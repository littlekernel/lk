/*
 * Copyright (c) 2017 - 2020, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NRFX_LOG_H__
#define NRFX_LOG_H__
#include<lk/debug.h>

// THIS IS A TEMPLATE FILE.
// It should be copied to a suitable location within the host environment into
// which nrfx is integrated, and the following macros should be provided with
// appropriate implementations.
// And this comment should be removed from the customized file.

#ifdef __cplusplus
extern "C" {
#endif


#ifndef NRFX_ENABLE_LOGGING
#define NRFX_ENABLE_LOGGING 0
#endif
/**
 * @defgroup nrfx_log nrfx_log.h
 * @{
 * @ingroup nrfx
 *
 * @brief This file contains macros that should be implemented according to
 *        the needs of the host environment into which @em nrfx is integrated.
 */


/**
 * @brief Macro for logging a message with the severity level ERROR.
 *
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#if NRFX_ENABLE_LOGGING
#define NRFX_LOG_ERROR(a...) dprintf(CRITICAL, "NRFX_ERROR:"); \
                             dprintf(CRITICAL, a); \
                             dprintf(CRITICAL,"\n")
#else
#define NRFX_LOG_ERROR(a...)
#endif

/**
 * @brief Macro for logging a message with the severity level WARNING.
 *
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#if NRFX_ENABLE_LOGGING
#define NRFX_LOG_WARNING(a...) dprintf(INFO, "NRFX_WARNING:"); \
                               dprintf(INFO, a); \
                               dprintf(INFO,"\n")
#else
#define NRFX_LOG_WARNING(a...)
#endif
/**
 * @brief Macro for logging a message with the severity level INFO.
 *
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#if NRFX_ENABLE_LOGGING
#define NRFX_LOG_INFO(a...) dprintf(INFO, "NRFX_INFO:"); \
                            dprintf(INFO, a); \
                            dprintf(INFO,"\n")
#else
#define NRFX_LOG_INFO(a...)
#endif
/**
 * @brief Macro for logging a message with the severity level DEBUG.
 *
 * @param format printf-style format string, optionally followed by arguments
 *               to be formatted and inserted in the resulting string.
 */
#if NRFX_ENABLE_LOGGING
#define NRFX_LOG_DEBUG(a...) dprintf(SPEW, "NRFX_DEBUG:"); \
                             dprintf(SPEW, a); \
                             dprintf(SPEW,"\n")
#else
#define NRFX_LOG_DEBUG(a...)
#endif

/**
 * @brief Macro for logging a memory dump with the severity level ERROR.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_ERROR(p_memory, length)

/**
 * @brief Macro for logging a memory dump with the severity level WARNING.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_WARNING(p_memory, length)

/**
 * @brief Macro for logging a memory dump with the severity level INFO.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_INFO(p_memory, length)

/**
 * @brief Macro for logging a memory dump with the severity level DEBUG.
 *
 * @param[in] p_memory Pointer to the memory region to be dumped.
 * @param[in] length   Length of the memory region in bytes.
 */
#define NRFX_LOG_HEXDUMP_DEBUG(p_memory, length)


/**
 * @brief Macro for getting the textual representation of a given error code.
 *
 * @param[in] error_code Error code.
 *
 * @return String containing the textual representation of the error code.
 */
#define NRFX_LOG_ERROR_STRING_GET(error_code) nrfx_get_err_str(error_code)

/** @} */
#define NRFX_ERR_STRING(x) \
  case NRFX_ERROR_##x: \
   return "NRFX_ERROR_"#x;

static inline const char* nrfx_get_err_str(nrfx_err_t code) {
  switch(code) {
    case NRFX_SUCCESS:
      return "NRFX_SUCCESS";
    case NRFX_ERROR_NULL:
      return "NRFX_ERROR_NULL";
    NRFX_ERR_STRING(INTERNAL)
    NRFX_ERR_STRING(NO_MEM)
    NRFX_ERR_STRING(NOT_SUPPORTED)
    NRFX_ERR_STRING(INVALID_PARAM)
    NRFX_ERR_STRING(INVALID_STATE)
    NRFX_ERR_STRING(INVALID_LENGTH)
    NRFX_ERR_STRING(TIMEOUT)
    NRFX_ERR_STRING(FORBIDDEN)
    NRFX_ERR_STRING(INVALID_ADDR)
    NRFX_ERR_STRING(BUSY)
    NRFX_ERR_STRING(ALREADY_INITIALIZED)
    NRFX_ERR_STRING(DRV_TWI_ERR_OVERRUN)
    NRFX_ERR_STRING(DRV_TWI_ERR_ANACK)
    NRFX_ERR_STRING(DRV_TWI_ERR_DNACK)
    default:
      return "UNKNOWN NRFX ERROR CODE";
  }
}

#ifdef __cplusplus
}
#endif

#endif // NRFX_LOG_H__
