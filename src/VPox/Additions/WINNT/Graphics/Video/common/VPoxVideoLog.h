/* $Id: VPoxVideoLog.h $ */
/** @file
 * VPox Video drivers, logging helper
 */

/*
 * Copyright (C) 2011-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#ifndef GA_INCLUDED_SRC_WINNT_Graphics_Video_common_VPoxVideoLog_h
#define GA_INCLUDED_SRC_WINNT_Graphics_Video_common_VPoxVideoLog_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifndef VPOX_VIDEO_LOG_NAME
# error VPOX_VIDEO_LOG_NAME should be defined!
#endif

#ifndef VPOX_VIDEO_LOG_LOGGER
# define VPOX_VIDEO_LOG_LOGGER Log
#endif

#ifndef VPOX_VIDEO_LOGREL_LOGGER
# define VPOX_VIDEO_LOGREL_LOGGER LogRel
#endif

#ifndef VPOX_VIDEO_LOGFLOW_LOGGER
# define VPOX_VIDEO_LOGFLOW_LOGGER LogFlow
#endif

#ifndef VPOX_VIDEO_LOG_FN_FMT
# define VPOX_VIDEO_LOG_FN_FMT LOG_FN_FMT
#endif

#ifndef VPOX_VIDEO_LOG_FORMATTER
# define VPOX_VIDEO_LOG_FORMATTER(_logger, _severity, _a)                     \
    do                                                                      \
    {                                                                       \
        _logger((VPOX_VIDEO_LOG_PREFIX_FMT _severity, VPOX_VIDEO_LOG_PREFIX_PARMS));  \
        _logger(_a);                                                        \
        _logger((VPOX_VIDEO_LOG_SUFFIX_FMT  VPOX_VIDEO_LOG_SUFFIX_PARMS));  \
    } while (0)
#endif

/* Uncomment to show file/line info in the log */
/*#define VPOX_VIDEO_LOG_SHOWLINEINFO*/

#define VPOX_VIDEO_LOG_PREFIX_FMT VPOX_VIDEO_LOG_NAME"::"VPOX_VIDEO_LOG_FN_FMT": "
#define VPOX_VIDEO_LOG_PREFIX_PARMS __FUNCTION__

#ifdef VPOX_VIDEO_LOG_SHOWLINEINFO
# define VPOX_VIDEO_LOG_SUFFIX_FMT " (%s:%d)\n"
# define VPOX_VIDEO_LOG_SUFFIX_PARMS ,__FILE__, __LINE__
#else
# define VPOX_VIDEO_LOG_SUFFIX_FMT "\n"
# define VPOX_VIDEO_LOG_SUFFIX_PARMS
#endif

#ifdef DEBUG_sunlover
# define BP_WARN() AssertFailed()
#else
# define BP_WARN() do {} while(0)
#endif

#define _LOGMSG_EXACT(_logger, _a)                                          \
    do                                                                      \
    {                                                                       \
        _logger(_a);                                                        \
    } while (0)

#define _LOGMSG(_logger, _severity, _a)                                     \
    do                                                                      \
    {                                                                       \
        VPOX_VIDEO_LOG_FORMATTER(_logger, _severity, _a);                   \
    } while (0)

/* we can not print paged strings to RT logger, do it this way */
#define _LOGMSG_STR(_logger, _a, _f) do {\
        int _i = 0; \
        for (;(_a)[_i];++_i) { \
            _logger(("%"_f, (_a)[_i])); \
        }\
        _logger(("\n")); \
    } while (0)

#ifdef VPOX_WDDM_MINIPORT
# define _WARN_LOGGER VPOX_VIDEO_LOGREL_LOGGER
#else
# define _WARN_LOGGER VPOX_VIDEO_LOG_LOGGER
#endif

#define WARN_NOBP(_a) _LOGMSG(_WARN_LOGGER, "WARNING! :", _a)
#define WARN(_a)           \
    do                     \
    {                      \
        WARN_NOBP(_a);     \
        BP_WARN();         \
    } while (0)

#define ASSERT_WARN(_a, _w) do {\
        if(!(_a)) { \
            WARN(_w); \
        }\
    } while (0)

#define STOP_FATAL() do {      \
        AssertReleaseFailed(); \
    } while (0)
#define ERR(_a) do { \
        _LOGMSG(VPOX_VIDEO_LOGREL_LOGGER, "FATAL! :", _a); \
        STOP_FATAL();                             \
    } while (0)

#define _DBGOP_N_TIMES(_count, _op) do {    \
        static int fDoWarnCount = (_count); \
        if (fDoWarnCount) { \
            --fDoWarnCount; \
            _op; \
        } \
    } while (0)

#define WARN_ONCE(_a) do {    \
        _DBGOP_N_TIMES(1, WARN(_a)); \
    } while (0)


#define LOG(_a) _LOGMSG(VPOX_VIDEO_LOG_LOGGER, "", _a)
#define LOGREL(_a) _LOGMSG(VPOX_VIDEO_LOGREL_LOGGER, "", _a)
#define LOGF(_a) _LOGMSG(VPOX_VIDEO_LOGFLOW_LOGGER, "", _a)
#define LOGF_ENTER() LOGF(("ENTER"))
#define LOGF_LEAVE() LOGF(("LEAVE"))
#define LOG_EXACT(_a) _LOGMSG_EXACT(VPOX_VIDEO_LOG_LOGGER, _a)
#define LOGREL_EXACT(_a) _LOGMSG_EXACT(VPOX_VIDEO_LOGREL_LOGGER, _a)
#define LOGF_EXACT(_a) _LOGMSG_EXACT(VPOX_VIDEO_LOGFLOW_LOGGER, _a)
/* we can not print paged strings to RT logger, do it this way */
#define LOG_STRA(_a) do {\
        _LOGMSG_STR(VPOX_VIDEO_LOG_LOGGER, _a, "c"); \
    } while (0)
#define LOG_STRW(_a) do {\
        _LOGMSG_STR(VPOX_VIDEO_LOG_LOGGER, _a, "c"); \
    } while (0)
#define LOGREL_STRA(_a) do {\
        _LOGMSG_STR(VPOX_VIDEO_LOGREL_LOGGER, _a, "c"); \
    } while (0)
#define LOGREL_STRW(_a) do {\
        _LOGMSG_STR(VPOX_VIDEO_LOGREL_LOGGER, _a, "c"); \
    } while (0)


#endif /* !GA_INCLUDED_SRC_WINNT_Graphics_Video_common_VPoxVideoLog_h */
