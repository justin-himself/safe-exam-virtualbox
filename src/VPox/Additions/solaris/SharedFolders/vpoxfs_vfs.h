/* $Id: vpoxfs_vfs.h $ */
/** @file
 * VirtualPox File System for Solaris Guests, VFS header.
 */

/*
 * Copyright (C) 2009-2020 Oracle Corporation
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

#ifndef GA_INCLUDED_SRC_solaris_SharedFolders_vpoxfs_vfs_h
#define GA_INCLUDED_SRC_solaris_SharedFolders_vpoxfs_vfs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

#ifdef	__cplusplus
extern "C" {
#endif

/*
 * Shared Folders filesystem per-mount data structure.
 */
typedef struct sffs_data {
	vfs_t		*sf_vfsp;	/* filesystem's vfs struct */
	vnode_t		*sf_rootnode;	/* of vnode of the root directory */
	int  		sf_stat_ttl;	/* ttl for stat caches (in ms) */
	int  		sf_fsync;	/* whether to honor fsync or not */
	char		*sf_share_name;
	char 		*sf_mntpath;	/* name of mount point */
	sfp_mount_t	*sf_handle;
	uint64_t	sf_ino;		/* per FS ino generator */
} sffs_data_t;

/*
 * Workaround for older Solaris versions which called map_addr()/choose_addr()/
 * map_addr_proc() with an 'alignment' argument that was removed in Solaris
 * 11.4.
 */
typedef struct VPoxVFS_SolAddrMap
{
    union
    {
        void *(*pfnSol_map_addr)          (caddr_t *, size_t, offset_t, uint_t);
        void *(*pfnSol_map_addr_old)      (caddr_t *, size_t, offset_t, int, uint_t);
    } MapAddr;

    union
    {
        int (*pfnSol_choose_addr)       (struct as *, caddr_t *, size_t, offset_t, uint_t);
        int (*pfnSol_choose_addr_old)   (struct as *, caddr_t *, size_t, offset_t, int, uint_t);
    } ChooseAddr;
} VPoxVFS_SolAddrMap;
typedef VPoxVFS_SolAddrMap *pVPoxVFS_SolAddrMap;

extern bool                     g_fVPoxVFS_SolOldAddrMap;
extern VPoxVFS_SolAddrMap       g_VPoxVFS_SolAddrMap;

#ifdef	__cplusplus
}
#endif

#endif /* !GA_INCLUDED_SRC_solaris_SharedFolders_vpoxfs_vfs_h */

