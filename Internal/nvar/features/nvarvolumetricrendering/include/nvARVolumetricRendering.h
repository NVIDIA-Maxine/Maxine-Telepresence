/*###############################################################################
#
# Copyright(c) 2025 NVIDIA CORPORATION.All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################*/

#ifndef INCLUDE_NVARVOLUMETRICRENDERING_NVARVOLUMETRICRENDERING_H_
#define INCLUDE_NVARVOLUMETRICRENDERING_NVARVOLUMETRICRENDERING_H_

#define NVARVOLUMETRICRENDERING_VERSION "1.0.0.0"

#define NvAR_Feature_VolumetricRendering "VolumetricRendering"

#define NVAR_VOLUMETRICRENDERING_POSTPROCESS_VIGNETTE (1U << 0)  // 0x001
#define NVAR_VOLUMETRICRENDERING_CLEARBUFFER_COLOR (1U << 0)     // 0x001
#define NVAR_VOLUMETRICRENDERING_CLEARBUFFER_DEPTH (1U << 1)     // 0x002

#endif  // INCLUDE_NVARVOLUMETRICRENDERING_NVARVOLUMETRICRENDERING_H_
