#!/bin/sh
# $Id: postinstall.sh $
## @file
# VirtualPox postinstall script for Solaris.
#

#
# Copyright (C) 2007-2020 Oracle Corporation
#
# This file is part of VirtualPox Open Source Edition (OSE), as
# available from http://www.virtualpox.org. This file is free software;
# you can redistribute it and/or modify it under the terms of the GNU
# General Public License (GPL) as published by the Free Software
# Foundation, in version 2 as it comes in the "COPYING" file of the
# VirtualPox OSE distribution. VirtualPox OSE is distributed in the
# hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
#

rc=0
currentzone=`zonename`
if test "$currentzone" = "global"; then
    DEBUGOPT=`set -o 2>/dev/null | sed -ne 's/^xtrace *on$/--sh-trace/p'` # propagate pkgadd -v
    ${PKG_INSTALL_ROOT:=/}/opt/VirtualPox/pkginstall.sh --srv4 ${DEBUGOPT}
    rc=$?
fi

# installf inherits ${PKG_INSTALL_ROOT} from pkgadd, no need to explicitly specify
/usr/sbin/installf -f $PKGINST

# return 20 = requires reboot, 2 = partial failure, 0  = success
exit "$rc"

