/** @file
 * libvdeplug header and dynamic symbol loader.
 */

/*
 * Copyright (C) 2008-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 *
 * The contents of this file may alternatively be used under the terms
 * of the Common Development and Distribution License Version 1.0
 * (CDDL) only, as it comes in the "COPYING.CDDL" file of the
 * VirtualPox OSE distribution, in which case the provisions of the
 * CDDL are applicable instead of those of the GPL.
 *
 * You may elect to license modified versions of this file under the
 * terms and conditions of either the GPL or the CDDL or both.
 */

#ifndef VPOX_INCLUDED_VDEPlug_h
#define VPOX_INCLUDED_VDEPlug_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#include <stddef.h>
#include <sys/types.h>

#define LIBVDEPLUG_INTERFACE_VERSION 1

#define vde_open(vde_switch, descr, open_args) \
    vde_open_real((vde_switch), (descr), LIBVDEPLUG_INTERFACE_VERSION, (open_args))

/** Opaque connection type */
struct vdeconn;
typedef struct vdeconn VDECONN;

/** Structure to be passed to the open function describing the connection.
 * All members can be left zero to use the default values. */
struct vde_open_args
{
    /** The port of the switch to connect to. */
    int port;
    /** The group to set ownership of the port socket to. */
    char *group;
    /** The file mode to set the port socket to. */
    mode_t mode;
};

/* Declarations of the functions that we need from the library */
#define VDEPLUG_GENERATE_HEADER

#include <VPox/VDEPlugSymDefs.h>

#undef VDEPLUG_GENERATE_HEADER

#endif /* !VPOX_INCLUDED_VDEPlug_h */
/* vi: set tabstop=4 shiftwidth=4 expandtab: */
