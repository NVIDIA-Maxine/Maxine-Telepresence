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

#include "nvCVLogger.h"

#include <limits.h>
#include <stdarg.h>
#include <string.h>

/// @brief Append formatted string to an existing string
/// @param[in, out] str The string to be appended.
/// @param[in]      fmt The printf-style formatting string.
/// @param[in]      ap  The argument list.
/// @return         0   If successful
static int StringVAppendF(std::string& str, const char* fmt, va_list ap) {
  int n0, n1, res;
  va_list ap2;
  char* s;

  n0 = str.size();
  str.resize(n0 + 1ull);
  s = &(str[n0]);
  // First, get the required size
#ifndef _MSC_VER
  va_copy(ap2, ap);
  res = vsnprintf(s, 1, fmt, ap2);
#else                       /* _MSC_VER */
  ap2 = ap;
  res = _vscprintf(fmt, ap2);
#endif                      /* _MSC_VER */
  if (res < 0) return res;  // There was an error
  va_end(ap2);
  // Check for potential overflow before adding 2
  if (res > INT_MAX - 2) return -1;  // Prevent overflow
  n1 = res + 2;
  if (n1 > INT_MAX - n0) return -1;  // Prevent overflow
  str.resize(n0 + n1);
  s = &(str[n0]);
  // Then, perform format string
#ifndef _MSC_VER
  res = vsnprintf(s, n1, fmt, ap);
#else                                    /* _MSC_VER */
  res = vsnprintf_s(s, n1, _TRUNCATE, fmt, ap);
#endif                                   /* _MSC_VER */
  if (res < 0 || res >= n1) return res;  // There was an error or the size was too small
  n1 = res;
  if (n1 > INT_MAX - n0) return -1;  // Prevent overflow
  str.resize(n0 + n1);
  va_end(ap);
  return 0;
}

/// @brief Append formatted string to an existing string
/// @param[in, out]  str the string to be appended.
/// @param[in]       fmt the printf-style formatting string.
/// @param[in]       ... any number of arguments used by the fmt string.
/// @return          0   If successful
static int StringAppendF(std::string& str, const char* fmt, ...) {
  va_list ap;
  int res;
  va_start(ap, fmt);
  res = StringVAppendF(str, fmt, ap);
  va_end(ap);
  return res;
}

/// @brief Strip off the directory path, leaving only the file name.
/// @param str[in] str the string, which may or may not have a leading directory.
/// @return        The string containing only the file name, without the directory *path.
static const char* BaseName(const char* str) {
  if (str && *str) {
    for (const char* s = str + strlen(str) - 1; s != str; --s) {
      if (*s == '/'
#ifdef _WIN32
          || *s == '\\'
#endif  // _WIN32
      ) {
        return s + 1;
      }
    }
  }
  return str;
}

/// @brief Prepend each line with a given string.
/// @param[in] dst         The string to be modified.Assume that it has > 0 * bytes, assurred by the caller.
/// @param[in] prefix_len  The length of the prefix located in the first line.*The prefix_len *must be nonzero, but is
///                        guaranteed to *be >= 4 by the caller.
static void PrependLinesInString(std::string& dst, unsigned prefix_len) {
  const char *s, *p, *send, *pend, *prefix;
  char* d;
  size_t numLines, skipSize, oldSize;

  if (dst[dst.size() - 1] != '\n') dst += '\n';  // assure terminal newline
  numLines = 0;
  skipSize = 0;
  for (const char& c : dst) {
    if (c == '\n') {
      ++numLines;                                     // count newlines
      if (!skipSize) skipSize = &c - dst.data() + 1;  // offset of the second line
    }
  }
  if (numLines == 0) return;
  --numLines;             // don't need to prepend the first line
  if (!numLines) return;  // only one line, already with the prefix (most common)
  oldSize = dst.size();

  dst.resize(oldSize + numLines * prefix_len);  // make way for the prefixes
  prefix = dst.data();                          // ... copy from the first line
  pend = prefix - 1;                            // reverse prefix end
  prefix = pend + prefix_len;                   // reverse prefix
  send = dst.data() - 1;                        // reverse src end
  s = send + oldSize;                           // reverse src
  send += skipSize;                             // revised reverse src end
  d = &(dst[dst.size() - 1]);                   // reverse dst

  while (s != send) {
    do {
      *d-- = *s--;  // shift line content
    } while (s != send && *s != '\n');
    for (p = prefix; p != pend;) *d-- = *p--;  // insert prefix
  }
}

NvCVLogger::NvCVLogger() : m_verbosity(NVCV_LOG_ERROR), m_callback(nullptr), m_userData(nullptr), m_fd(nullptr) {
  m_loggerStr.reserve(2000);
}

NvCVLogger::~NvCVLogger() {
  flush();
  if (m_callback) (*m_callback)(m_userData, nullptr);  // NULL string signals to close
}

void NvCVLogger::flush() {
  std::lock_guard<std::mutex> lk(m_logMutex);
  if (m_callback && !m_loggerStr.empty()) {
    (*m_callback)(m_userData, m_loggerStr.c_str());
    m_loggerStr.clear();
  }
}

void NvCVLogger::FileLogger(void* user_data, const char* msg) {
  NvCVLogger* logger = (NvCVLogger*)user_data;
  if (msg) {
    fputs(msg, logger->m_fd);
  } else if (logger->m_fd) {
    fflush(logger->m_fd);
    if (logger->m_fd != stderr) {
      fclose(logger->m_fd);
      logger->m_fd = nullptr;
      logger->m_fileName.clear();
    }
  }
}

void NvCVLogger::setVerbosity(int verbosity) {
#ifndef NDEBUG
  if (verbosity > NVCV_LOG_VERBOSE) verbosity = NVCV_LOG_VERBOSE;
#else   // NDEBUG
  if (verbosity > NVCV_LOG_INFO) verbosity = NVCV_LOG_INFO;
#endif  // NDEBUG
  m_verbosity = verbosity;
}

int NvCVLogger::init(int verbosity, const char* file, CallbackProc callback, void* user_data) {
#ifndef NDEBUG
  if (verbosity > NVCV_LOG_VERBOSE) verbosity = NVCV_LOG_VERBOSE;
#else   // NDEBUG
  if (verbosity > NVCV_LOG_INFO) verbosity = NVCV_LOG_INFO;
#endif  // NDEBUG

  // We provide a means to change the verbosity level without changing the logger.
  if (file && !strcmp("same", file)) {   // Keep the same current logger -- just change the verbosity
    if (user_data)                       // Also, if requested, ...
      *((int*)user_data) = m_verbosity;  // ... return the current verbosity
    m_verbosity = verbosity;             // Change the verbosity level.
    return 0;
  }
  m_verbosity = verbosity;  // Change the verbosity level.

  if (file && *file) {
    // stderr logger
    if (!strcmp("stderr", file)) {  // stderr logger
      if (m_fd) {                   // If a file descriptor is already active, ...
        if (stderr == m_fd)         // ... and it is stderr, ...
          return 0;                 // .. we are done
        fflush(m_fd);               // It is currently set to file logging, so we flush before switching.
        fclose(m_fd);               // Close up the file that was being used for logging.
      }
      m_fd = stderr;  // Select stderr for logging
      m_callback = &FileLogger;
      m_userData = this;
      m_fileName.clear();
      return 0;
    }

    // file logger
    else {
      if (m_fd) {                                                 // If a file descriptor is already active, ...
        if (stderr != m_fd && !strcmp(file, m_fileName.c_str()))  // ... and we are already logging to this file, ...
          return 0;                                               // ... we are done
        flush();
        if (stderr != m_fd) fclose(m_fd);  // Close the file
      } else {
        flush();
      }
#ifndef _MSC_VER
      m_fd = fopen(file, (user_data ? (char*)user_data : "w"));
#else                         // _MSC_VER
      fopen_s(&m_fd, file, (user_data ? (char*)user_data : "w"));
#endif                        // _MSC_VER
      if (!m_fd) return -13;  // NVCV_ERR_FILE
      m_callback = &FileLogger;
      m_userData = this;
      m_fileName = file;
      return 0;
    }
  }

  // callback logger
  else {
    if (m_fd) {
      if (stderr != m_fd) fclose(m_fd);
      m_fd = nullptr;
    }
    if (callback == m_callback && user_data == m_userData) return 0;
    if (m_callback) (*m_callback)(m_userData, nullptr);
    m_callback = callback;
    m_userData = user_data;
    m_fileName.clear();
    return 0;
  }

  return -2;  // NVCV_ERR_UNIMPLEMENTED: It should never get here, but the compiler complains without it
}

int NvCVLogger::log(const char* file, int line, const char* func, int verbosity, const char* msg, ...) {
  static const char code[] = "FEWIDV";  // { FATAL, ERROR, WARNING, INFO, DEBUG, VERBOSE }
  va_list ap;
  int res;

  if (verbosity > m_verbosity || !m_callback) return -1;  // m_verbosity <= NVCV_LOG_VERBOSE has been enforced
  file = BaseName(file);

  {
    std::lock_guard<std::mutex> lk(m_logMutex);
    res = StringAppendF(m_loggerStr, "[%c] ", code[verbosity]);  // The msg is guaranteed to have these 4 characters
    if (res != 0) return res;
    if (file && file[0]) {
      if (!func) func = "";
      res = StringAppendF(m_loggerStr, "\"%s\", line %d, %s: ", file, line, func);
      if (res != 0) return res;
    }
    unsigned prefix_len = unsigned(m_loggerStr.size());
    va_start(ap, msg);
    res = StringVAppendF(m_loggerStr, msg, ap);
    if (res != 0) return res;
    va_end(ap);
    PrependLinesInString(m_loggerStr, prefix_len);  // This assures the terminal newline on a msg with at least 4 chars
    (*m_callback)(m_userData, m_loggerStr.c_str());
    m_loggerStr.clear();
  }
  return 0;
}
