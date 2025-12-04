/*###############################################################################
#
# Copyright 2020 NVIDIA Corporation
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
#ifndef NvAR_DEFS_H
#define NvAR_DEFS_H

#include <nvCVImage.h>
#include <nvCVStatus.h>
#include <stddef.h>
#include <stdint.h>

#ifdef _WIN32
#ifdef NVAR_API_EXPORT
#define NvAR_API __declspec(dllexport) __cdecl
#else
#define NvAR_API
#endif
#else
#ifdef NVAR_API_EXPORT
#define NvAR_API __attribute__((visibility("default")))
#else
#define NvAR_API
#endif
#endif  // OS dependencies

typedef struct NvAR_Vector3f {
  float vec[3];
} NvAR_Vector3f;

typedef struct NvAR_Vector3u16 {
  unsigned short vec[3];
} NvAR_Vector3u16;

typedef struct NvAR_Rect {
  float x, y, width, height;
} NvAR_Rect;

typedef struct NvAR_BBoxes {
  NvAR_Rect* boxes;
  uint8_t num_boxes;
  uint8_t max_boxes;
} NvAR_BBoxes;

typedef struct NvAR_TrackingBBox {
  NvAR_Rect bbox;
  uint16_t tracking_id;
} NvAR_TrackingBBox;

typedef struct NvAR_TrackingBBoxes {
  NvAR_TrackingBBox* boxes;
  uint8_t num_boxes;
  uint8_t max_boxes;
} NvAR_TrackingBBoxes;

typedef struct NvAR_FaceMesh {
  NvAR_Vector3f* vertices;  ///< Mesh 3D vertex positions.
  size_t num_vertices;
  NvAR_Vector3u16* tvi;  ///< Mesh triangle's vertex indices
  size_t num_triangles;  ///< The number of triangles (previously num_tri_idx)
} NvAR_FaceMesh;
#define num_tri_idx num_triangles  ///< num_tri_idx is confusing and deprecated

typedef struct NvAR_Frustum {
  float left;
  float right;
  float bottom;
  float top;
} NvAR_Frustum;

typedef struct NvAR_Quaternion {
  float x, y, z, w;
} NvAR_Quaternion;

typedef struct NvAR_Point2f {
  float x, y;
} NvAR_Point2f;

typedef struct NvAR_Point3f {
  float x, y, z;
} NvAR_Point3f;

typedef struct NvAR_Vector2f {
  float x, y;
} NvAR_Vector2f;

typedef struct NvAR_RenderingParams {
  NvAR_Frustum frustum;
  NvAR_Quaternion rotation;
  NvAR_Vector3f translation;
} NvAR_RenderingParams;

typedef struct NvAR_SpeakerData {
  const float* audio_frame_data;  ///< Buffer containing driving audio.
  size_t audio_frame_size;        ///< Number of samples in audio buffer.
  NvAR_Rect region;               ///< Region that contains the speaker's face.
  uint8_t region_type;            ///< 0 = ROI, perform face detection within ROI. 1 = face box, skip face detection.
  float bypass;                   ///< Value in [0, 1] that can reduce the output opacity.
} NvAR_SpeakerData;

// Parameters provided by client application
typedef const char* NvAR_FeatureID;
// clang-format off

typedef struct NvAR_RenderCameraIntrinsicParams {
  float fx;  // Horizontal focal length
  float fy;  // Vertical focal length
  float cx;  // Horizontal principal point coordinate
  float cy;  // vertical principal point coordinate
} NvAR_RenderCameraIntrinsicParams;

#define NvAR_Parameter_Input(Name) "NvAR_Parameter_Input_" #Name
#define NvAR_Parameter_Output(Name) "NvAR_Parameter_Output_" #Name
#define NvAR_Parameter_Config(Name) "NvAR_Parameter_Config_" #Name
#define NvAR_Parameter_InOut(Name) "NvAR_Parameter_InOut_" #Name

#ifndef   NVCV_LOG_FATAL
  #define NVCV_LOG_FATAL    0   //!< Message to be printed right before aborting due to an unrecoverable error.
  #define NVCV_LOG_ERROR    1   //!< An operation has failed, but it is not fatal.
  #define NVCV_LOG_WARNING  2   //!< Something was not quite right, but we fixed it up, perhaps at a loss in performance.
  #define NVCV_LOG_INFO     3   //!< Nothing is wrong, but this information might be of interest.
#endif // NVCV_LOG_FATAL

#define NVAR_TEMPORAL_FILTER_FACE_BOX                 (1U << 0)  // 0x001
#define NVAR_TEMPORAL_FILTER_FACIAL_LANDMARKS         (1U << 1)  // 0x002
#define NVAR_TEMPORAL_FILTER_FACE_ROTATIONAL_POSE     (1U << 2)  // 0x004
#define NVAR_TEMPORAL_FILTER_FACIAL_EXPRESSIONS       (1U << 4)  // 0x010
#define NVAR_TEMPORAL_FILTER_FACIAL_GAZE              (1U << 5)  // 0x020
#define NVAR_TEMPORAL_FILTER_ENHANCE_EXPRESSIONS      (1U << 8)  // 0x100

#endif  // NvAR_DEFS_H
