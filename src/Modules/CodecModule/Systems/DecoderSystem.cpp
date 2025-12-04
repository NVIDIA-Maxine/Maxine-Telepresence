/*
 * SPDX-FileCopyrightText: Copyright (c) 2024 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: LicenseRef-NvidiaProprietary
 *
 * NVIDIA CORPORATION, its affiliates and licensors retain all intellectual
 * property and proprietary rights in and to this material, related
 * documentation and any modifications thereto. Any use, reproduction,
 * disclosure or distribution of this material and related documentation
 * without an express license agreement from NVIDIA CORPORATION or
 * its affiliates is strictly prohibited.
 */

#include "DecoderSystem.h"

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>

#include "Core/Engine/Engine.h"
#include "Core/EntityComponentSystem/Entity.h"
#include "Core/Error.h"
#include "Core/Util/Logger.h"
#include "Modules/AudioModule/Components/AudioSourceComponent.h"
#include "Modules/CodecModule/Components/AudioDecoderComponent.h"
#include "Modules/CodecModule/Components/FrameDecoderComponent.h"
#include "Modules/CommonModule/Components/CallbackComponent.h"
#include "Modules/CommonModule/Components/VideoFrameComponent.h"
#include "Modules/TriplaneModule/Components/TriplaneBufferComponent.h"
#include "nvCVImage.h"

namespace nv3dvc {
namespace modules {
namespace codecmodule {
namespace systems {

static core::Error HandleDecodedTriplaneFrame(NvCVImage* decoded_frame,
                                              const triplanemodule::TriplanePackage& triplane_package,
                                              triplanemodule::components::TriplaneBufferComponent* triplane_buffer,
                                              cudaStream_t cu_stream, NvCVImage* tmp_image, float dt) {
  core::Error err = core::Error::SUCCESS;
  NvCVTriplaneVolume source_triplane_volume;
  // Decode to triplane frame.
  triplanemodule::TriplaneFrame* triplane_frame = triplane_buffer->GetWriteableTriplaneFrame();
  if (!triplane_frame) BAIL(err, core::Error::ERR_DATA_UNAVAILABLE);

  CHECK_TRUE(decoded_frame->width == 2560, static_cast<nv3dvc::core::Error>(NVCV_ERR_RESOLUTION));
  CHECK_TRUE(decoded_frame->height == 2560, static_cast<nv3dvc::core::Error>(NVCV_ERR_RESOLUTION));
  CHECK_TRUE(decoded_frame->numComponents == 1, static_cast<nv3dvc::core::Error>(NVCV_ERR_PIXELFORMAT));
  CHECK_TRUE(decoded_frame->componentType == NVCV_U8, static_cast<nv3dvc::core::Error>(NVCV_ERR_PIXELFORMAT));
  CHECK_TRUE(triplane_package.mins.size() == 96, static_cast<nv3dvc::core::Error>(NVCV_ERR_MISMATCH),
             "Triplane package quantization mins size (%d) needs to be 96", triplane_package.mins.size());
  CHECK_TRUE(triplane_package.maxs.size() == 96, static_cast<nv3dvc::core::Error>(NVCV_ERR_MISMATCH),
             "Triplane package quantization maxs size (%d) needs to be 96", triplane_package.maxs.size());

  // Create a triplane view of the source
  source_triplane_volume.config_params = {
      96,           // Number of effective triplane channels within the structure
      10,           // Used to splay out the triplane channels in a grid
      10,           // Used to splay out the triplane channels in a grid
      256,          // Width of one triplane
      256,          // Width of one triplane
      NVCV_Y,       // The format of the pixels in the triplanes
      NVCV_U8,      // The type of the components of the pixels.
      NVCV_PLANAR,  // NVCV_CHUNKY, or NVCV_PLANAR
      NVCV_GPU,     // Location of the triplane buffer: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
      NVCV_CPU,     // Location of the mins, maxs buffers: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
      1,            // Row byte alignment.
  };
  source_triplane_volume.quantization_mins = triplane_frame->triplane_package.mins.data();
  source_triplane_volume.quantization_maxs = triplane_frame->triplane_package.maxs.data();
  triplane_frame->triplane_package = triplane_package;
  NvCVImage_InitView(&source_triplane_volume.triplanes, decoded_frame, 0, 0, decoded_frame->width,
                     decoded_frame->height);
  CHECK_NVCV_SUCCESS(
      NvCVVolume_TransferTriplaneVolume(&source_triplane_volume, &triplane_frame->triplane_volume, cu_stream));
  // Write confidence
  triplane_frame->triplane_volume.confidence =
      triplane_buffer->FilterConfidence(triplane_frame->triplane_package.confidence, dt);
  triplane_buffer->SignalProvide();

bail:
  return err;
}

static core::Error HandleDecodedVideoFrame(NvCVImage* decoded_frame,
                                           commonmodule::components::VideoFrameComponent* video_frame_component,
                                           cudaStream_t cu_stream, NvCVImage* tmp_image, float dt) {
  core::Error err = core::Error::SUCCESS;
  // Ensure VideoFrameComponent has the right size.
  if (NvCVImage* ptr = video_frame_component->GetImagePtr();
      ptr->width != decoded_frame->width || ptr->height != decoded_frame->height) {
    LOG_DEBUG("Re-allocating VideoFrameComponent from %d x %d to %d x %d", ptr->width, ptr->height,
              decoded_frame->width, decoded_frame->height);
    // Initialize the video frame component.
    CHECK_SUCCESS(video_frame_component->AllocateImageBuffer(
        decoded_frame->width, decoded_frame->height, NvCVImage_PixelFormat::NVCV_BGR, NvCVImage_ComponentType::NVCV_U8,
        NVCV_CHUNKY, NVCV_CUDA, 1));
  }
  // Write the decoded frame output
  CHECK_NVCV_SUCCESS(
      NvCVImage_Transfer(decoded_frame, video_frame_component->GetImagePtr(), 1.0f, cu_stream, tmp_image));
  CHECK_CU_SUCCESS(cuStreamSynchronize(cu_stream));
bail:
  return err;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// Member function definitions                                                                                      ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

DecoderSystem::DecoderSystem(core::engine::Engine* engine) : m_engine(engine) {}

core::Error DecoderSystem::Initialize() {
  core::Error err = core::Error::SUCCESS;
  CHECK_NONNULL(m_engine, core::Error::ERR_NULL_POINTER, "DecoderSystem requires Engine");
bail:
  return err;
}

core::Error DecoderSystem::Uninitialize() { return core::Error::SUCCESS; }

core::Error DecoderSystem::OnLoadScene(core::ecs::registry::EntityRegistry* reg) {
  core::Error err = core::Error::SUCCESS;
  for (auto& entity : reg->view<triplanemodule::components::TriplaneBufferComponent>()) {
    entity.GetComponent<triplanemodule::components::TriplaneBufferComponent>().Initialize();
  }

  for (auto& entity :
       reg->view<components::FrameDecoderComponent, commonmodule::components::EncodedVideoCallbackComponent>()) {
    // The EncodedVideoCallbackComponent can be fired by the ReceiverSystem, when it receives new packets.
    auto& frame_decoder = entity.GetComponent<components::FrameDecoderComponent>();
    auto& callback_component = entity.GetComponent<commonmodule::components::EncodedVideoCallbackComponent>();
    // Initialize decoder
    cudaVideoCodec codec = cudaVideoCodec::cudaVideoCodec_H264;
    // Choose decoder target format based on whether it's a triplane video or a regular 2D video.
    VideoDecoder::FrameOutputFormat frame_output_format = VideoDecoder::FrameOutputFormat::NV12;
    triplanemodule::components::TriplaneBufferComponent* triplane_buffer = nullptr;
    commonmodule::components::VideoFrameComponent* video_frame_component = nullptr;
    if (entity.HasComponent<triplanemodule::components::TriplaneBufferComponent>()) {
      triplane_buffer = &entity.GetComponent<triplanemodule::components::TriplaneBufferComponent>();
      frame_output_format = VideoDecoder::FrameOutputFormat::Y8Planar;
    } else if (entity.HasComponent<commonmodule::components::VideoFrameComponent>()) {
      video_frame_component = &entity.GetComponent<commonmodule::components::VideoFrameComponent>();
      frame_output_format = VideoDecoder::FrameOutputFormat::NV12;
    }
    CHECK_SUCCESS(frame_decoder.Initialize(m_engine->GetCudaContext(), codec, frame_output_format));
    // Set callback for decoding incoming data.
    callback_component.SetOnFiredCallback([this, &frame_decoder, triplane_buffer, video_frame_component](
                                              const void* data_ptr, size_t data_size, size_t* decoded_size,
                                              int /*data_type*/) {
      core::Error err = core::SUCCESS;

      auto buffers = static_cast<const commonmodule::components::EncodedVideoCallbackComponent::Buffers*>(data_ptr);
      // Decode metadata packet if present.
      if (buffers->metadata_buffer) {
        try {
          triplanemodule::TriplanePackage triplane_package;
          nlohmann::json mdata = nlohmann::json::from_cbor<uint8_t>(
              buffers->metadata_buffer, buffers->metadata_size, true, true, nlohmann::json::cbor_tag_handler_t::error);
          if (mdata.contains("TriplanePackage")) {
            mdata.at("TriplanePackage").get_to(triplane_package);
            frame_decoder.SetLastDecodedTriplanePackage(triplane_package);
          }
        } catch (const nlohmann::json::parse_error& e) {
          LOG_ERROR("%s: Error parsing json metadata, %s", NAME, e.what());
        }
      }

      // Pass video packet to the decoder.
      int flags = CUVID_PKT_ENDOFPICTURE;
      cudaStream_t cu_stream = GetStream();
      frame_decoder.DecodeFrame(buffers->package_buffer, buffers->package_size, cu_stream, flags);
      // We must retrieve all decoded frames before we can call frame_decoder.DecodeFrame again.
      while (frame_decoder.NumDecodedFramesAvailable() > 0) {
        NvCVImage decoded_frame;
        CHECK_SUCCESS(frame_decoder.GetDecodedFrame(&decoded_frame));
        CHECK_NONNULL(decoded_frame.pixels, core::Error::ERR_DATA_UNAVAILABLE);
        if (triplane_buffer) {
          CHECK_SUCCESS(HandleDecodedTriplaneFrame(&decoded_frame, frame_decoder.GetLastDecodedTriplanePackage(),
                                                   triplane_buffer, cu_stream, &m_tmpImage, buffers->dt));
        }
        if (video_frame_component) {
          CHECK_SUCCESS(
              HandleDecodedVideoFrame(&decoded_frame, video_frame_component, cu_stream, &m_tmpImage, buffers->dt));
        }
      }
    bail:
      if (decoded_size) *decoded_size = (err == core::SUCCESS) ? data_size : 0;
      return err;
    });
  }

  // Audio decoder ==> audio source.
  for (auto& entity : reg->view<codecmodule::components::AudioDecoderComponent,  //
                                audiomodule::components::AudioSourceComponent>()) {
    auto& decoder_component = entity.GetComponent<codecmodule::components::AudioDecoderComponent>();
    auto& source_component = entity.GetComponent<audiomodule::components::AudioSourceComponent>();
    if (source_component.IsInitialized()) {
      LOG_ERROR("Source component already initialized");
      BAIL(err, core::ERR_GENERAL);
    }
    int sample_rate = 48'000;
    int num_channels = 1;
    CHECK_SUCCESS(decoder_component.Initialize(sample_rate, num_channels));
    source_component.Initialize(sample_rate, num_channels);
    decoder_component.SetDecodeCallback(std::bind(&audiomodule::components::AudioSourceComponent::PushSourceAudio,
                                                  &source_component, std::placeholders::_1, std::placeholders::_2));
  }

bail:
  return err;
}

core::Error DecoderSystem::Run(core::ecs::registry::EntityRegistry* reg, float dt) { return core::Error::SUCCESS; }

}  // namespace systems
}  // namespace codecmodule
}  // namespace modules
}  // namespace nv3dvc
