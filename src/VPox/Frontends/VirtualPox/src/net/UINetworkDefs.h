/* $Id: UINetworkDefs.h $ */
/** @file
 * VPox Qt GUI - Network routine related declarations.
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

#ifndef FEQT_INCLUDED_SRC_net_UINetworkDefs_h
#define FEQT_INCLUDED_SRC_net_UINetworkDefs_h
#ifndef RT_WITHOUT_PRAGMA_ONCE
# pragma once
#endif

/* Qt includes: */
#include <QMap>

/** Network request types. */
enum UINetworkRequestType
{
    UINetworkRequestType_HEAD,
    UINetworkRequestType_GET
};

/** User dictionary. */
typedef QMap<QString, QString> UserDictionary;

#endif /* !FEQT_INCLUDED_SRC_net_UINetworkDefs_h */

