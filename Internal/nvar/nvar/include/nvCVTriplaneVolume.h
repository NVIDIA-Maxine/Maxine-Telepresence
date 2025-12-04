/*###############################################################################
#
# Copyright 2023 NVIDIA Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a copy of
# this software and associated documentation files (the "Software"), to deal in
# the Software without restriction, including without limitation the rights to
# use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
# the Software, and to permit persons to whom the Software is furnished to do so,
# subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
# FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
# COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
# IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#
###############################################################################*/

#ifndef __NVCV_TRIPLANE_VOLUME_H__
#define __NVCV_TRIPLANE_VOLUME_H__

#include "nvCVImage.h"
#include "nvCVStatus.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//! Triplane descriptor.
typedef struct
#ifdef _MSC_VER
    __declspec(dllexport)
#endif  // _MSC_VER
        NvCVTriplaneVolume {
  struct ConfigParams {
    unsigned int num_triplanes;        // Number of effective triplane channels within the structure
    unsigned int grid_width;           // Used to splay out the triplane channels in a grid
    unsigned int grid_height;          // Used to splay out the triplane channels in a grid
    unsigned int width;                // Width of one triplane
    unsigned int height;               // Width of one triplane
    NvCVImage_PixelFormat format;      // The format of the pixels in the triplanes
    NvCVImage_ComponentType type;      // The type of the components of the pixels.
    unsigned int layout;               //  NVCV_CHUNKY, or NVCV_PLANAR
    unsigned int mem_space;            // Location of the triplane buffer: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
    unsigned int mem_space_mins_maxs;  // Location of the mins, maxs buffers: one of { NVCV_CPU, NVCV_GPU, NVCV_CUDA }
    unsigned int alignment;            // Row byte alignment.
  } config_params;
  NvCVImage triplanes;                             // The triplane image buffer and descriptor
  float* quantization_mins;                        // Quantization mins buffer pointer. One value per triplane
  void* quantization_mins_delete_ptr;              // Quantization mins delete pointer.
  void (*quantization_mins_delete_proc)(void* p);  // Quantization mins delete procedure.
  float* quantization_maxs;                        // Quantization maxs buffer pointer. One value per triplane
  void* quantization_maxs_delete_ptr;              // Quantization maxs delete pointer.
  void (*quantization_maxs_delete_proc)(void* p);  // Quantization maxs delete procedure.
  float confidence;                                // Confidence used by triplane renderer. [0, 1]

#ifdef __cplusplus

  /// @brief Create a triplane volume object, but do not allocate buffers
  /// To allocate buffers, set configuration parameters and call NvCVVolume_AllocateTriplaneVolume
  /// This constructor will initialize default values for the configuration parameters to create a tiled 10x10 triplane
  /// volume object
  inline NvCVTriplaneVolume();

  /// @brief Destructor. Deallocates any previously allocated buffers
  inline ~NvCVTriplaneVolume();

#endif  // __cplusplus
} NvCVTriplaneVolume;

/// @brief Allocate memory for a triplane volume object
/// Assuming all triplane volume config parameters have been set
/// @param[in,out] triplane_volume   The triplane volume to allocate buffers for.
/// @return NVCV_SUCCESS             If the operation was successful.
/// @return NVCV_ERR_PIXELFORMAT     If the pixel format is not accommodated.
/// @return NVCV_ERR_MEMORY          If there is not enough memory to allocate the buffer.
/// @return NVCV_ERR_PARAMETER       If the combination of configuration parameters is not supported.
NvCV_Status NvCV_API NvCVVolume_AllocateTriplaneVolume(NvCVTriplaneVolume *triplane_volume);

/// @brief Allocate memory for a batch of triplane volume objects sharing the same configuration
/// @param[in,out] triplane_volume   The triplane volume to allocate buffers for.
/// @param[in] batch_size            The number of triplane volume objects in the batch
/// @return NVCV_SUCCESS             If the operation was successful.
/// @return NVCV_ERR_PIXELFORMAT     If the pixel format is not accommodated.
/// @return NVCV_ERR_MEMORY          If there is not enough memory to allocate the buffer.
/// @return NVCV_ERR_PARAMETER       If the combination of configuration parameters is not supported.
NvCV_Status NvCV_API NvCVVolume_AllocateTriplaneVolumeBatch(NvCVTriplaneVolume *triplane_volume,
                                                            unsigned int batch_size);

/// @brief Deallocate the buffers from the triplane volume. The triplane volume object itself is not deallocated.
/// Configuration parameters are left unchanged
/// @param[in,out] triplane_volume The triplane volume object to be deallocted.
void NvCV_API NvCVVolume_DeallocateTriplaneVolume(NvCVTriplaneVolume *triplane_volume);

/// @brief Transfer triplane volume object between different formats
///
/// A simple transfer is applied if the source and destination triplane volume objects use the same format descriptors
/// in their configuration parameters. These are grid_width, grid_height, width, height, num_triplanes, and format. in
/// such cases transferring between different types and memory spaces are allowed. Special cases handles conversion
/// between different representations of the triplane volume object. Currently, the different supported representations
/// are:
///
/// Tensor representation:
/// * format == NVCV_FORMAT_UNKNOWN
/// * grid_width == 1
/// * grid_height == 1
/// * layout == NVCV_PLANAR
///
/// 10X10 single channel grid representation:
/// * format == NVCV_Y
/// * grid_width == 10
/// * grid_height == 10
/// * num_triplanes == 96
/// * width == 256
/// * height == 256 &&
///
/// 6x6 three channel grid representation:
/// * format == NVCV_RGBA
/// * grid_width == 6
/// * grid_height == 6
/// * num_triplanes == 96
/// * width == 256
/// * height == 256
/// * layout == NVCV_CHUNKY
/// 
/// 4x6 three channel grid representation:
/// * format == NVCV_RGBA
/// * grid_width == 4
/// * grid_height == 6
/// * num_triplanes == 96
/// * width == 256
/// * height == 256
/// * layout == NVCV_CHUNKY
///
/// @param[in]     src Source triplane volume
/// @param[in,out] dst Destination triplane volume
/// @param[in]     stream Stream on which to perform the copy.
/// @return        NVCV_ERR_PIXELFORMAT If the provided format is not supported
/// @return        NVCV_SUCCESS         If successful
NvCV_Status NvCV_API NvCVVolume_TransferTriplaneVolume(const NvCVTriplaneVolume *src, NvCVTriplaneVolume *dst,
                                                       CUstream_st *stream);

/// @brief Initialize a triplane volume descriptor for the Nth triplane volume in a batch.
/// @param[in] n     The index of the desired triplane volume in the batch.
/// @param[in] full  The triplane volume batch, or the 0th triplane volume in the batch.
/// @param[in] nth   The triplane volume descriptor to be initialized to a view of the nth triplane volume in the batch.
void NvCV_API NvCVVolume_NthTriplaneVolume(unsigned int n, NvCVTriplaneVolume *full, NvCVTriplaneVolume *nth);

#ifdef __cplusplus
}  // extern "C"

NvCVTriplaneVolume::NvCVTriplaneVolume()
    : config_params({
          96,                   // num_triplanes
          1,                    // grid_width
          1,                    // grid_height
          256,                  // width
          256,                  // height
          NVCV_FORMAT_UNKNOWN,  // format
          NVCV_F32,             // type
          NVCV_PLANAR,          // layout
          NVCV_CUDA,            // mem_space
          NVCV_CUDA,            // mem_space_mins_maxs
          1                     // alignment
      }),
      quantization_mins(nullptr),
      quantization_mins_delete_ptr(nullptr),
      quantization_mins_delete_proc(nullptr),
      quantization_maxs(nullptr),
      quantization_maxs_delete_ptr(nullptr),
      quantization_maxs_delete_proc(nullptr),
      confidence(1.0f) {}

NvCVTriplaneVolume::~NvCVTriplaneVolume() { NvCVVolume_DeallocateTriplaneVolume(this); }

#endif  // __cplusplus

#endif  // __NVCV_TRIPLANE_VOLUME_H__
