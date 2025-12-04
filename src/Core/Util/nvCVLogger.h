/*###############################################################################
#
# Copyright(c) 2018-2023 NVIDIA CORPORATION.All Rights Reserved.
#
# NVIDIA CORPORATION and its licensors retain all intellectual property
# and proprietary rights in and to this software, related documentation
# and any modifications thereto.Any use, reproduction, disclosure or
# distribution of this software and related documentation without an express
# license agreement from NVIDIA CORPORATION is strictly prohibited.
#
###############################################################################*/

#ifndef __NVCVLOGGER__
#define __NVCVLOGGER__

#include <stdio.h>

#include <mutex>
#include <string>

/// Verbosity level
#ifndef   NVCV_LOG_FATAL
  #define NVCV_LOG_FATAL    0   //!< Message to be printed right before aborting due to an unrecoverable error.
  #define NVCV_LOG_ERROR    1   //!< An operation has failed, but it is not fatal.
  #define NVCV_LOG_WARNING  2   //!< Something was not quite right, but we fixed it up, perhaps at a loss in performance.
  #define NVCV_LOG_INFO     3   //!< Nothing is wrong, but this information might be of interest.
#endif // NVCV_LOG_FATAL
#ifndef   NVCV_LOG_DEBUG
  #define NVCV_LOG_DEBUG    4   //!< This is targeted for developers, but is not available in release builds.
  #define NVCV_LOG_VERBOSE  5   //!< Not sure. Maybe this is the same as DEBUG.
#endif // NVCV_LOG_DEBUG


class NvCVLogger {
public:

  /// Typedef for the callback function.
  /// @param[in,out]  user_data   a pointer to data needed by the specific logger.
  /// @param[in]      msg         a C-string to add to the log.
  typedef void (*CallbackProc)(void *user_data, const char *msg);

  /// Constructor.
  NvCVLogger();

  /// Destructor.
  ~NvCVLogger();

  /// Set the level of verbosity.
  /// @param[in] level the maximum desired level of verbosity.
  void  setVerbosity(int level);

  /// Query the current level of verbosity.
  /// @return the current level of verbosity.
  int   getVerbosity() const      { return m_verbosity; }

  /// Initialize or change the logger.
  /// @param[in] verbosity  the maximum desired verbosity of logging.
  /// @param[in] file       the file to be logged to, if logging to a file. There are some special file names:
  ///                       "stderr"  logs to stderr; the other parameters are ignored;
  ///                       "same"    keep the same logger, just change the verbosity level; if user_data is not NULL,
  ///                                 the current verbosity is returned in its location.
  /// @param[in] callback   the callback function.
  /// @param[in] user_data  pointer to data used by the callback function.
  /// @return 0  if successful.
  int init(int verbosity, const char *file, CallbackProc callback, void *user_data);

  /// Query as to whether the logger has been inited yet. 
  /// @return 1 if the logger has been inited, 0 otherwise.
  int isInited() const { return m_callback != nullptr; }

  /// Append an entry to the log.
  /// @param[in] file       the source file from which the log entry was generated.
  /// @param[in] line       the line number in the source file from which the log entry was generated.
  /// @param[in] func       the function name that generated the log entry.
  /// @param[in] verbosity  the verbosity level of the log entry.
  /// @param[in] msg        the message (possibly with additional VARARGS) to be logged.
  /// @param[in] ...        additional arguments that might be needed by the printf-style msg string. 
  int log(const char *file, int line, const char *func, int verbosity, const char *msg, ...)
      #if defined(__GNUC__)
        __attribute__ ((__format__ (__printf__, 6, 7)));
      #endif // __GNUC__
  ;


private:
  int                     m_verbosity;    ///< The maximum level of verbosity desired in the log.
  CallbackProc            m_callback;     ///< The callback function.
  void                    *m_userData;    ///< Pointer to data for the callback function.
  FILE                    *m_fd;          ///< The file descriptor.
  std::string             m_loggerStr;    ///< The string used in the logger.
  std::string             m_fileName;     ///< The name of the file used for logging.
  std::mutex              m_logMutex;     ///< The mutex used for logging.

  /// Flush the current logger.
  void        flush();

  /// Callback used for logging to a file.
  /// @param[in] user_data  pointer to this, used by the file logger.
  /// @param[in] msg        the message to be printed to the file log.
  static void FileLogger(void *user_data, const char *msg);
};


#endif // __NVCVLOGGER__
