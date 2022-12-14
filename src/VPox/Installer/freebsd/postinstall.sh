#!/bin/sh
## @file
#
# VirtualPox postinstall script for FreeBSD.
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

PATH_TMP="/tmp"
PATH_TMP_MODS="$PATH_TMP/vpox_mods"
PATH_INST="/usr/local/lib/virtualpox"
PATH_KERN_SRC="/usr/src/sys"
FILE_VPOXDRV="$PATH_INST/vpoxdrv.tar.gz"


if [ ! -f $PATH_KERN_SRC/Makefile ]; then
    echo "Kernel sources are not installed. Please install them and reinstall VirtualPox"
    exit 1
fi

echo "Compiling kernel modules, please wait..."

# Create temporary directory
mkdir -p $PATH_TMP_MODS

# Unpack archive
tar -C $PATH_TMP_MODS -xf  $FILE_VPOXDRV

# Compile
cd $PATH_TMP_MODS
make

# Check if we succeeded
if [ $? != 0 ]; then
    echo "Compiling kernel modules failed."
    cd ..
    rm -rf $PATH_TMP_MODS
    exit 1
fi

# Copy the modules to /boot/kernel
echo "Installing kernel modules to /boot/kernel, please wait..."
cp $PATH_TMP_MODS/vpoxdrv.ko /boot/kernel
cp $PATH_TMP_MODS/vpoxnetflt.ko /boot/kernel
cp $PATH_TMP_MODS/vpoxnetadp.ko /boot/kernel
kldxref -R /boot

# Load them now, unloading old modules
make load

if [ $? != 0 ]; then
    echo "Loading kernel modules failed"
    cd ..
    rm -rf $PATH_TMP_MODS
    exit 1
fi

echo "Kernel modules successfully installed."
echo "To load them on every boot put them into /boot/loader.conf"

# Cleanup
cd ..
rm -rf $PATH_TMP_MODS

exit 0

