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

#ifndef NVARVOLUMETRICENCODING_H_
#define NVARVOLUMETRICENCODING_H_

#define NVARVOLUMETRICENCODING_VERSION "1.0.0.0"

#define NvAR_Feature_VolumetricEncoding "VolumetricEncoding"

#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_NONE 0u
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_1 (1U << 0)
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_2 (1U << 1)
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_4 (1U << 2)
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_8 (1U << 3)
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_16 (1U << 4)
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_32 (1U << 5)
#define NVAR_VOLUMETRIC_ENCODING_QUANTIZATION_FILTER_INTERVAL_64 (1U << 6)

#endif  // NVARVOLUMETRICENCODING_H_
