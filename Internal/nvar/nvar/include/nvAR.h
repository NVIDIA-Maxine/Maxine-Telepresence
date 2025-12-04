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

#ifndef NvAR_H
#define NvAR_H

#include "nvAR_defs.h"

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

// Forward declaration for CUDA API
// CUstream and cudaStream_t are CUstream_st*
struct CUstream_st;
typedef struct CUstream_st *CUstream;

typedef struct nvAR_Feature nvAR_Feature;
typedef struct nvAR_Feature *NvAR_FeatureHandle;
typedef struct NvAR_StateObjectBase NvAR_StateObjectBase;
typedef struct NvAR_StateObjectBase *NvAR_StateHandle;

//! Get the SDK version
//! \param[in,out]  version    Pointer to an unsigned int set to 
//!                            (major << 24) | (minor << 16) | (build << 8) | 0
//! \return         NVCV_SUCCESS  if the version was set
//! \return         NVCV_ERR_PARAMETER  if version was NULL
NvCV_Status NvAR_API NvAR_GetVersion(unsigned int *version);

//! Create a new feature instantiation.
//! \param[in]  InFeatureID The selector code for the desired feature.
//! \param[out] handle      Handle to the feature instance.
NvCV_Status NvAR_API NvAR_Create(NvAR_FeatureID featureID, NvAR_FeatureHandle *handle);

//! Load the model based on the set params.
//! \param[in] handle Handle to the feature instance.
NvCV_Status NvAR_API NvAR_Load(NvAR_FeatureHandle handle);

//! Run the selected feature instance.
//! \param[in] handle Handle to the feature instance.
NvCV_Status NvAR_API NvAR_Run(NvAR_FeatureHandle handle);

//! Delete a previously created feature instance.
//! \param[in] handle Handle to the feature instance.
NvCV_Status NvAR_API NvAR_Destroy(NvAR_FeatureHandle handle);

//! Wrapper for cudaStreamCreate(), if it is desired to avoid linking with the cuda lib.
//! \param[out] stream  A place to store the newly allocated stream.
NvCV_Status NvAR_API NvAR_CudaStreamCreate(CUstream *stream);

//! Wrapper for cudaStreamDestroy(), if it is desired to avoid linking with the cuda lib.
//! \param[in]  stream  The stream to destroy.
NvCV_Status NvAR_API NvAR_CudaStreamDestroy(CUstream stream);

//! Wrapper for cudaStreamSynchronize(), if it is desired to avoid linking with the cuda lib.
//! \param[in]  stream  The stream to destroy.
NvCV_Status NvAR_API NvAR_CudaStreamSynchronize(CUstream stream);

//! Set the value of the selected parameter.
//! \param[in] handle Handle to the feature instance.
//! \param[in] name   The selector of the feature parameter to configure.
//! \param[in] val    The value to be assigned to the selected feature parameter.
NvCV_Status NvAR_API NvAR_SetU32(NvAR_FeatureHandle handle, const char *name, unsigned int val);
NvCV_Status NvAR_API NvAR_SetS32(NvAR_FeatureHandle handle, const char *name, int val);
NvCV_Status NvAR_API NvAR_SetF32(NvAR_FeatureHandle handle, const char *name, float val);
NvCV_Status NvAR_API NvAR_SetF64(NvAR_FeatureHandle handle, const char *name, double val);
NvCV_Status NvAR_API NvAR_SetU64(NvAR_FeatureHandle handle, const char *name, unsigned long long val);
NvCV_Status NvAR_API NvAR_SetObject(NvAR_FeatureHandle handle, const char *name, void *ptr, unsigned long typeSize);
NvCV_Status NvAR_API NvAR_SetString(NvAR_FeatureHandle handle, const char *name, const char *str);
NvCV_Status NvAR_API NvAR_SetCudaStream(NvAR_FeatureHandle handle, const char *name, CUstream stream);
NvCV_Status NvAR_API NvAR_SetU32Array(NvAR_FeatureHandle handle, const char *name, unsigned int *vals, int count);
NvCV_Status NvAR_API NvAR_SetF32Array(NvAR_FeatureHandle handle, const char *name, float *vals, int count);

//! Get the value of the selected parameter.
//! \param[in]  handle Handle to the feature instance.
//! \param[in]  name   The selector of the feature parameter to retrieve.
//! \param[out] val    Place to store the retrieved parameter.
NvCV_Status NvAR_API NvAR_GetU32(NvAR_FeatureHandle handle, const char *name, unsigned int *val);
NvCV_Status NvAR_API NvAR_GetS32(NvAR_FeatureHandle handle, const char *name, int *val);
NvCV_Status NvAR_API NvAR_GetF32(NvAR_FeatureHandle handle, const char *name, float *val);
NvCV_Status NvAR_API NvAR_GetF64(NvAR_FeatureHandle handle, const char *name, double *val);
NvCV_Status NvAR_API NvAR_GetU64(NvAR_FeatureHandle handle, const char *name, unsigned long long *val);
NvCV_Status NvAR_API NvAR_GetObject(NvAR_FeatureHandle handle, const char *name, const void **ptr,
                                    unsigned long typeSize);
NvCV_Status NvAR_API NvAR_GetString(NvAR_FeatureHandle handle, const char *name, const char **str);
NvCV_Status NvAR_API NvAR_GetCudaStream(NvAR_FeatureHandle handle, const char *name, const CUstream *stream);
NvCV_Status NvAR_API NvAR_GetU32Array(NvAR_FeatureHandle handle, const char *name, const unsigned int **vals, int *count);
NvCV_Status NvAR_API NvAR_GetF32Array(NvAR_FeatureHandle handle, const char *name, const float **vals, int *count);

//! Configure the logger. Not all fields are used.
//! There are several ways to use this API:
//! (1) log to stderr:      (verbosity, "stderr",  NULL, NULL).
//! (2) log to a new file:  (verbosity, file_name, NULL, NULL) or (verbosity, file_name, NULL, "w").
//! (3) append to a file:   (verbosity, file_name, NULL, "a").
//! (4) log to a  function: (verbosity, NULL,      func, func_data)
//! (5) stop logging        (verbosity, NULL,      NULL, NULL)
//!
//! \param[in]  verbosity the highest level of verbosity desired in the log.
//! \param[in]  file      the file name, for logging to a file; NULL otherwise.
//!                       The special file "stderr" will cause logging to go to stderr.
//!                       The special file "same" will allow you to change the verbosity level without changing the logger,
//!                       in which case the current verbosity is returned to the int at cb_data, if not NULL.
//! \param[in]  cb        the callback function, if file == NULL, of type
//!                       typedef void (*NvCVCallbackProc)(void *user_data, const char *msg);
//! \param[in]  cb_data   pointer to data used by the callback function. This can also used to alter the mode used
//!                       when opening a file (e.g. "a"), which is "w" by default.
NvCV_Status NvAR_API NvAR_ConfigureLogger(int verbosity, const char *file, void (*cb)(void*, const char*), void *cb_data);


//! Reset the state object for a stateful feature.
//! \param[in] handle Handle to the feature instance.
//! \param[in] state  Handle to the state object.
NvCV_Status NvAR_API NvAR_ResetState(NvAR_FeatureHandle handle, NvAR_StateHandle state);

#ifdef __cplusplus
}
#endif  // __cplusplus

#endif  // #define NvAR_H
