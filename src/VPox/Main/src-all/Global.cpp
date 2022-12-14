/* $Id: Global.cpp $ */
/** @file
 * VirtualPox COM global definitions
 *
 * NOTE: This file is part of both VPoxC.dll and VPoxSVC.exe.
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
 */

#include "Global.h"

#include <iprt/assert.h>
#include <iprt/string.h>
#include <iprt/errcore.h>

/* static */
const Global::OSType Global::sOSTypes[] =
{
    /* NOTE1: we assume that unknown is always the first two entries!
     * NOTE2: please use powers of 2 when specifying the size of harddisks since
     *        '2GB' looks better than '1.95GB' (= 2000MB)
     * NOTE3: if you add new guest OS types please check if the code in
     *        Machine::getEffectiveParavirtProvider and Console::i_configConstructorInner
     *        are still covering the relevant cases. */
    { "Other",   "Other",             "Other",              "Other/Unknown",
      VPOXOSTYPE_Unknown,         VPOXOSHINT_NONE,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700 },

    { "Other",   "Other",             "Other_64",           "Other/Unknown (64-bit)",
      VPOXOSTYPE_Unknown_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_PAE | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700 },

    { "Windows", "Microsoft Windows", "Windows31",          "Windows 3.1",
      VPOXOSTYPE_Win31,           VPOXOSHINT_FLOPPY,
      1,   32,   4,  1 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "Windows95",          "Windows 95",
      VPOXOSTYPE_Win95,           VPOXOSHINT_FLOPPY,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "Windows98",          "Windows 98",
      VPOXOSTYPE_Win98,           VPOXOSHINT_FLOPPY,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "WindowsMe",          "Windows ME",
      VPOXOSTYPE_WinMe,           VPOXOSHINT_FLOPPY | VPOXOSHINT_USBTABLET,
      1,  128,   4,  4 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsNT3x",        "Windows NT 3.x",
      VPOXOSTYPE_WinNT3x,       VPOXOSHINT_NOUSB | VPOXOSHINT_FLOPPY,
      1,   64,   8,  _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_BusLogic, StorageBus_SCSI,
      StorageControllerType_BusLogic, StorageBus_SCSI, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "WindowsNT4",         "Windows NT 4",
      VPOXOSTYPE_WinNT4,          VPOXOSHINT_NOUSB,
      1,  128,  16,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Windows", "Microsoft Windows", "Windows2000",        "Windows 2000",
      VPOXOSTYPE_Win2k,           VPOXOSHINT_USBTABLET,
      1,  168,  16,  4 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsXP",          "Windows XP (32-bit)",
      VPOXOSTYPE_WinXP,           VPOXOSHINT_USBTABLET,
      1,  192,  16, 10 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82543GC, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsXP_64",       "Windows XP (64-bit)",
      VPOXOSTYPE_WinXP_x64,       VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1,  512,  16, 10 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "Windows2003",        "Windows 2003 (32-bit)",
      VPOXOSTYPE_Win2k3,          VPOXOSHINT_USBTABLET,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82543GC, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "Windows2003_64",     "Windows 2003 (64-bit)",
      VPOXOSTYPE_Win2k3_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "WindowsVista",       "Windows Vista (32-bit)",
      VPOXOSTYPE_WinVista,        VPOXOSHINT_USBTABLET,
      1,  512,  16, 25 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "WindowsVista_64",    "Windows Vista (64-bit)",
      VPOXOSTYPE_WinVista_x64,    VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1,  512,  16, 25 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2008",        "Windows 2008 (32-bit)",
      VPOXOSTYPE_Win2k8,          VPOXOSHINT_USBTABLET,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2008_64",     "Windows 2008 (64-bit)",
      VPOXOSTYPE_Win2k8_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows7",           "Windows 7 (32-bit)",
      VPOXOSTYPE_Win7,            VPOXOSHINT_USBTABLET,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows7_64",        "Windows 7 (64-bit)",
      VPOXOSTYPE_Win7_x64,        VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows8",           "Windows 8 (32-bit)",
      VPOXOSTYPE_Win8,            VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_PAE | VPOXOSHINT_USB3,
      1, 1024, 128, 40 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows8_64",        "Windows 8 (64-bit)",
      VPOXOSTYPE_Win8_x64,        VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      1, 2048, 128, 40 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows81",          "Windows 8.1 (32-bit)",
      VPOXOSTYPE_Win81,           VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_PAE | VPOXOSHINT_USB3,
      1, 1024, 128, 40 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows81_64",       "Windows 8.1 (64-bit)",
      VPOXOSTYPE_Win81_x64,       VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      1, 2048, 128, 40 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2012_64",     "Windows 2012 (64-bit)",
      VPOXOSTYPE_Win2k12_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows10",          "Windows 10 (32-bit)",
      VPOXOSTYPE_Win10,           VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_PAE | VPOXOSHINT_USB3,
      1, 1024, 128, 50 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows10_64",       "Windows 10 (64-bit)",
      VPOXOSTYPE_Win10_x64,       VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2016_64",     "Windows 2016 (64-bit)",
      VPOXOSTYPE_Win2k16_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows2019_64",     "Windows 2019 (64-bit)",
      VPOXOSTYPE_Win2k19_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      1, 2048, 128, 50 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "Windows11_64",       "Windows 11 (64-bit)",
      VPOXOSTYPE_Win11_x64,       VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_USBTABLET | VPOXOSHINT_USB3,
      2, 4096, 128, 80 * _1G64, GraphicsControllerType_VPoxSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Windows", "Microsoft Windows", "WindowsNT",          "Other Windows (32-bit)",
      VPOXOSTYPE_WinNT,           VPOXOSHINT_NONE,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Windows", "Microsoft Windows", "WindowsNT_64",       "Other Windows (64-bit)",
      VPOXOSTYPE_WinNT_x64,       VPOXOSHINT_64BIT | VPOXOSHINT_PAE | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1,  512,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Linux",   "Linux",             "Linux22",            "Linux 2.2",
      VPOXOSTYPE_Linux22,         VPOXOSHINT_RTCUTC,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Linux24",            "Linux 2.4 (32-bit)",
      VPOXOSTYPE_Linux24,         VPOXOSHINT_RTCUTC,
      1,  128,  16,  4 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Linux24_64",         "Linux 2.4 (64-bit)",
      VPOXOSTYPE_Linux24_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC,
      1,  128,  16,  4 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Linux26",            "Linux 2.6 / 3.x / 4.x (32-bit)",
      VPOXOSTYPE_Linux26,         VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1,  512,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Linux26_64",         "Linux 2.6 / 3.x / 4.x (64-bit)",
      VPOXOSTYPE_Linux26_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "ArchLinux",          "Arch Linux (32-bit)",
      VPOXOSTYPE_ArchLinux,       VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "ArchLinux_64",       "Arch Linux (64-bit)",
      VPOXOSTYPE_ArchLinux_x64,   VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Debian",             "Debian (32-bit)",
      VPOXOSTYPE_Debian,          VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Debian_64",          "Debian (64-bit)",
      VPOXOSTYPE_Debian_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980},

    { "Linux",   "Linux",             "Fedora",             "Fedora (32-bit)",
      VPOXOSTYPE_FedoraCore,      VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Fedora_64",          "Fedora (64-bit)",
      VPOXOSTYPE_FedoraCore_x64,  VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Gentoo",             "Gentoo (32-bit)",
      VPOXOSTYPE_Gentoo,          VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Gentoo_64",          "Gentoo (64-bit)",
      VPOXOSTYPE_Gentoo_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Mandriva",           "Mandriva (32-bit)",
      VPOXOSTYPE_Mandriva,        VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Mandriva_64",        "Mandriva (64-bit)",
      VPOXOSTYPE_Mandriva_x64,    VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Oracle",             "Oracle (32-bit)",
      VPOXOSTYPE_Oracle,          VPOXOSHINT_RTCUTC | VPOXOSHINT_PAE | VPOXOSHINT_X2APIC,
      1, 1024,  16, 12 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Oracle_64",          "Oracle (64-bit)",
      VPOXOSTYPE_Oracle_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_PAE | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_X2APIC,
      1, 1024,  16, 12 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "RedHat",             "Red Hat (32-bit)",
      VPOXOSTYPE_RedHat,          VPOXOSHINT_RTCUTC | VPOXOSHINT_PAE | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "RedHat_64",          "Red Hat (64-bit)",
      VPOXOSTYPE_RedHat_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_PAE | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "OpenSUSE",           "openSUSE (32-bit)",
      VPOXOSTYPE_OpenSUSE,        VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "OpenSUSE_64",        "openSUSE (64-bit)",
      VPOXOSTYPE_OpenSUSE_x64,    VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Turbolinux",         "Turbolinux (32-bit)",
      VPOXOSTYPE_Turbolinux,      VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1,  384,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Turbolinux_64",      "Turbolinux (64-bit)",
      VPOXOSTYPE_Turbolinux_x64,  VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1,  384,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Ubuntu",             "Ubuntu (32-bit)",
      VPOXOSTYPE_Ubuntu,          VPOXOSHINT_RTCUTC | VPOXOSHINT_PAE | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16, 10 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Ubuntu_64",          "Ubuntu (64-bit)",
      VPOXOSTYPE_Ubuntu_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC
                                | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1, 1024,  16, 10 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Xandros",            "Xandros (32-bit)",
      VPOXOSTYPE_Xandros,         VPOXOSHINT_RTCUTC | VPOXOSHINT_X2APIC,
      1,  256,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Xandros_64",         "Xandros (64-bit)",
      VPOXOSTYPE_Xandros_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC | VPOXOSHINT_X2APIC,
      1,  256,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Linux",              "Other Linux (32-bit)",
      VPOXOSTYPE_Linux,           VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1,  256,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_AD1980  },

    { "Linux",   "Linux",             "Linux_64",           "Other Linux (64-bit)",
      VPOXOSTYPE_Linux_x64,       VPOXOSHINT_64BIT | VPOXOSHINT_PAE | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC
                                | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET | VPOXOSHINT_X2APIC,
      1,  512,  16,  8 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris",            "Oracle Solaris 10 5/09 and earlier (32-bit)",
      VPOXOSTYPE_Solaris,         VPOXOSHINT_NONE,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris_64",         "Oracle Solaris 10 5/09 and earlier (64-bit)",
      VPOXOSTYPE_Solaris_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris10U8_or_later",        "Oracle Solaris 10 10/09 and later (32-bit)",
      VPOXOSTYPE_Solaris10U8_or_later,     VPOXOSHINT_USBTABLET,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris10U8_or_later_64",     "Oracle Solaris 10 10/09 and later (64-bit)",
      VPOXOSTYPE_Solaris10U8_or_later_x64, VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "Solaris11_64",       "Oracle Solaris 11 (64-bit)",
      VPOXOSTYPE_Solaris11_x64,   VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET | VPOXOSHINT_RTCUTC,
      1, 4096,  16, 32 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "OpenSolaris",        "OpenSolaris / Illumos / OpenIndiana (32-bit)",
      VPOXOSTYPE_OpenSolaris,     VPOXOSHINT_USBTABLET,
      1, 1024,  16, 32 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Solaris", "Solaris",           "OpenSolaris_64",     "OpenSolaris / Illumos / OpenIndiana (64-bit)",
      VPOXOSTYPE_OpenSolaris_x64, VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 32 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "FreeBSD",            "FreeBSD (32-bit)",
      VPOXOSTYPE_FreeBSD,         VPOXOSHINT_NONE,
      1, 1024,  16,  2 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "FreeBSD_64",         "FreeBSD (64-bit)",
      VPOXOSTYPE_FreeBSD_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "OpenBSD",            "OpenBSD (32-bit)",
      VPOXOSTYPE_OpenBSD,         VPOXOSHINT_HWVIRTEX,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "OpenBSD_64",         "OpenBSD (64-bit)",
      VPOXOSTYPE_OpenBSD_x64,     VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "NetBSD",             "NetBSD (32-bit)",
      VPOXOSTYPE_NetBSD,          VPOXOSHINT_RTCUTC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "BSD",     "BSD",               "NetBSD_64",          "NetBSD (64-bit)",
      VPOXOSTYPE_NetBSD_x64,      VPOXOSHINT_64BIT | VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_RTCUTC,
      1, 1024,  16, 16 * _1G64, GraphicsControllerType_VMSVGA, NetworkAdapterType_I82540EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "OS2",     "IBM OS/2",          "OS21x",              "OS/2 1.x",
      VPOXOSTYPE_OS21x,           VPOXOSHINT_FLOPPY | VPOXOSHINT_NOUSB | VPOXOSHINT_TFRESET,
      1,    8,   4, 500 * _1M, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2Warp3",           "OS/2 Warp 3",
      VPOXOSTYPE_OS2Warp3,        VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY,
      1,   48,   4,  1 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2Warp4",           "OS/2 Warp 4",
      VPOXOSTYPE_OS2Warp4,        VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2Warp45",          "OS/2 Warp 4.5",
      VPOXOSTYPE_OS2Warp45,       VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY,
      1,  128,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "OS2",     "IBM OS/2",          "OS2eCS",             "eComStation",
      VPOXOSTYPE_ECS,             VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY,
      1,  256,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "OS2",     "IBM OS/2",          "OS2ArcaOS",          "ArcaOS",
      VPOXOSTYPE_ArcaOS,          VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY,
      1, 1024,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82540EM, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700 },

    { "OS2",     "IBM OS/2",          "OS2",                "Other OS/2",
      VPOXOSTYPE_OS2,             VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY | VPOXOSHINT_NOUSB,
      1,   96,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "MacOS",   "Mac OS X",          "MacOS",              "Mac OS X (32-bit)",
      VPOXOSTYPE_MacOS,           VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS_64",           "Mac OS X (64-bit)",
      VPOXOSTYPE_MacOS_x64,       VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS106",           "Mac OS X 10.6 Snow Leopard (32-bit)",
      VPOXOSTYPE_MacOS106,        VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS106_64",        "Mac OS X 10.6 Snow Leopard (64-bit)",
      VPOXOSTYPE_MacOS106_x64,    VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS107_64",        "Mac OS X 10.7 Lion (64-bit)",
      VPOXOSTYPE_MacOS107_x64,    VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS108_64",        "Mac OS X 10.8 Mountain Lion (64-bit)",  /* Aka "Mountain Kitten". */
      VPOXOSTYPE_MacOS108_x64,    VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 20 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS109_64",        "Mac OS X 10.9 Mavericks (64-bit)", /* Not to be confused with McCain. */
      VPOXOSTYPE_MacOS109_x64,    VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 25 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1010_64",       "Mac OS X 10.10 Yosemite (64-bit)",
      VPOXOSTYPE_MacOS1010_x64,   VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 25 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1011_64",       "Mac OS X 10.11 El Capitan (64-bit)",
      VPOXOSTYPE_MacOS1011_x64,   VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 30 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1012_64",       "macOS 10.12 Sierra (64-bit)",
      VPOXOSTYPE_MacOS1012_x64,   VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 30 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "MacOS",   "Mac OS X",          "MacOS1013_64",       "macOS 10.13 High Sierra (64-bit)",
      VPOXOSTYPE_MacOS1013_x64,   VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_EFI | VPOXOSHINT_PAE |  VPOXOSHINT_64BIT
                                | VPOXOSHINT_USBHID | VPOXOSHINT_HPET | VPOXOSHINT_RTCUTC | VPOXOSHINT_USBTABLET,
      1, 2048,  16, 30 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_IntelAhci, StorageBus_SATA,
      StorageControllerType_IntelAhci, StorageBus_SATA, ChipsetType_ICH9, AudioControllerType_HDA, AudioCodecType_STAC9221  },

    { "Other",   "Other",             "DOS",                "DOS",
      VPOXOSTYPE_DOS,             VPOXOSHINT_FLOPPY | VPOXOSHINT_NOUSB,
      1,   32,   4,  500 * _1M, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 1, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_SB16, AudioCodecType_SB16  },

    { "Other",   "Other",             "Netware",            "Netware",
      VPOXOSTYPE_Netware,         VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY | VPOXOSHINT_NOUSB,
      1,  512,   4,  4 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "L4",                 "L4",
      VPOXOSTYPE_L4,              VPOXOSHINT_NONE,
      1,   64,   4,  2 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "QNX",                "QNX",
      VPOXOSTYPE_QNX,             VPOXOSHINT_HWVIRTEX,
      1,  512,   4,  4 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_Am79C973, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "JRockitVE",          "JRockitVE",
      VPOXOSTYPE_JRockitVE,     VPOXOSHINT_HWVIRTEX | VPOXOSHINT_IOAPIC | VPOXOSHINT_PAE,
      1, 1024,   4,  8 * _1G64, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_BusLogic, StorageBus_SCSI, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },

    { "Other",   "Other",             "VPoxBS_64",          "VirtualPox Bootsector Test (64-bit)",
      VPOXOSTYPE_VPoxBS_x64,    VPOXOSHINT_HWVIRTEX | VPOXOSHINT_FLOPPY | VPOXOSHINT_IOAPIC | VPOXOSHINT_PAE | VPOXOSHINT_64BIT,
      1,  128,   4,  0, GraphicsControllerType_VPoxVGA, NetworkAdapterType_I82545EM, 0, StorageControllerType_PIIX4, StorageBus_IDE,
      StorageControllerType_PIIX4, StorageBus_IDE, ChipsetType_PIIX3, AudioControllerType_AC97, AudioCodecType_STAC9700  },
};

size_t Global::cOSTypes = RT_ELEMENTS(Global::sOSTypes);

/**
 * Returns an OS Type ID for the given VPOXOSTYPE value.
 *
 * The returned ID will correspond to the IGuestOSType::id value of one of the
 * objects stored in the IVirtualPox::guestOSTypes
 * (VirtualPoxImpl::COMGETTER(GuestOSTypes)) collection.
 */
/* static */
const char *Global::OSTypeId(VPOXOSTYPE aOSType)
{
    for (size_t i = 0; i < RT_ELEMENTS(sOSTypes); ++i)
    {
        if (sOSTypes[i].osType == aOSType)
            return sOSTypes[i].id;
    }

    return sOSTypes[0].id;
}

/**
 * Maps an OS type ID string to index into sOSTypes.
 *
 * @returns index on success, UINT32_MAX if not found.
 * @param   pszId       The OS type ID string.
 */
/* static */ uint32_t Global::getOSTypeIndexFromId(const char *pszId)
{
    for (size_t i = 0; i < RT_ELEMENTS(sOSTypes); ++i)
        if (!RTStrICmp(pszId, Global::sOSTypes[i].id))
            return (uint32_t)i;
    return UINT32_MAX;
}

/*static*/ uint32_t Global::getMaxNetworkAdapters(ChipsetType_T aChipsetType)
{
    switch (aChipsetType)
    {
        case ChipsetType_PIIX3:
            return 8;
        case ChipsetType_ICH9:
            return 36;
        default:
            return 0;
    }
}

/*static*/ const char *
Global::stringifyMachineState(MachineState_T aState)
{
    switch (aState)
    {
        case MachineState_Null:                 return "Null";
        case MachineState_PoweredOff:           return "PoweredOff";
        case MachineState_Saved:                return "Saved";
        case MachineState_Teleported:           return "Teleported";
        case MachineState_Aborted:              return "Aborted";
        case MachineState_Running:              return "Running";
        case MachineState_Paused:               return "Paused";
        case MachineState_Stuck:                return "GuruMeditation";
        case MachineState_Teleporting:          return "Teleporting";
        case MachineState_LiveSnapshotting:     return "LiveSnapshotting";
        case MachineState_Starting:             return "Starting";
        case MachineState_Stopping:             return "Stopping";
        case MachineState_Saving:               return "Saving";
        case MachineState_Restoring:            return "Restoring";
        case MachineState_TeleportingPausedVM:  return "TeleportingPausedVM";
        case MachineState_TeleportingIn:        return "TeleportingIn";
        case MachineState_DeletingSnapshotOnline: return "DeletingSnapshotOnline";
        case MachineState_DeletingSnapshotPaused: return "DeletingSnapshotPaused";
        case MachineState_OnlineSnapshotting:   return "OnlineSnapshotting";
        case MachineState_RestoringSnapshot:    return "RestoringSnapshot";
        case MachineState_DeletingSnapshot:     return "DeletingSnapshot";
        case MachineState_SettingUp:            return "SettingUp";
        case MachineState_Snapshotting:         return "Snapshotting";
        default:
        {
            AssertMsgFailed(("%d (%#x)\n", aState, aState));
            static char s_szMsg[48];
            RTStrPrintf(s_szMsg, sizeof(s_szMsg), "InvalidState-0x%08x\n", aState);
            return s_szMsg;
        }
    }
}

/*static*/ const char *
Global::stringifySessionState(SessionState_T aState)
{
    switch (aState)
    {
        case SessionState_Null:         return "Null";
        case SessionState_Unlocked:     return "Unlocked";
        case SessionState_Locked:       return "Locked";
        case SessionState_Spawning:     return "Spawning";
        case SessionState_Unlocking:    return "Unlocking";
        default:
        {
            AssertMsgFailed(("%d (%#x)\n", aState, aState));
            static char s_szMsg[48];
            RTStrPrintf(s_szMsg, sizeof(s_szMsg), "InvalidState-0x%08x\n", aState);
            return s_szMsg;
        }
    }
}

/*static*/ const char *
Global::stringifyDeviceType(DeviceType_T aType)
{
    switch (aType)
    {
        case DeviceType_Null:         return "Null";
        case DeviceType_Floppy:       return "Floppy";
        case DeviceType_DVD:          return "DVD";
        case DeviceType_HardDisk:     return "HardDisk";
        case DeviceType_Network:      return "Network";
        case DeviceType_USB:          return "USB";
        case DeviceType_SharedFolder: return "ShardFolder";
        default:
        {
            AssertMsgFailed(("%d (%#x)\n", aType, aType));
            static char s_szMsg[48];
            RTStrPrintf(s_szMsg, sizeof(s_szMsg), "InvalidType-0x%08x\n", aType);
            return s_szMsg;
        }
    }
}

/*static*/ const char *
Global::stringifyReason(Reason_T aReason)
{
    switch (aReason)
    {
        case Reason_Unspecified:      return "unspecified";
        case Reason_HostSuspend:      return "host suspend";
        case Reason_HostResume:       return "host resume";
        case Reason_HostBatteryLow:   return "host battery low";
        case Reason_Snapshot:         return "snapshot";
        default:
        {
            AssertMsgFailed(("%d (%#x)\n", aReason, aReason));
            static char s_szMsg[48];
            RTStrPrintf(s_szMsg, sizeof(s_szMsg), "invalid reason %#010x\n", aReason);
            return s_szMsg;
        }
    }
}

/*static*/ const char *
Global::stringifyStorageControllerType(StorageControllerType_T aType)
{
    switch (aType)
    {
        case StorageControllerType_Null:        return "Null";
        case StorageControllerType_LsiLogic:    return "LsiLogic";
        case StorageControllerType_BusLogic:    return "BusLogic";
        case StorageControllerType_IntelAhci:   return "AHCI";
        case StorageControllerType_PIIX3:       return "PIIX3";
        case StorageControllerType_PIIX4 :      return "PIIX4";
        case StorageControllerType_ICH6:        return "ICH6";
        case StorageControllerType_I82078:      return "I82078";
        case StorageControllerType_LsiLogicSas: return "LsiLogicSas";
        case StorageControllerType_USB:         return "USB";
        case StorageControllerType_NVMe:        return "NVMe";
        case StorageControllerType_VirtioSCSI:  return "VirtioSCSI";
        default:
        {
            AssertMsgFailed(("%d (%#x)\n", aType, aType));
            static char s_szMsg[48];
            RTStrPrintf(s_szMsg, sizeof(s_szMsg), "Invalid storage controller type: 0x%08x\n", aType);
            return s_szMsg;
        }
    }
}

/* vi: set tabstop=4 shiftwidth=4 expandtab: */
