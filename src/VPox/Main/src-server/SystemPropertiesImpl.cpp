/* $Id: SystemPropertiesImpl.cpp $ */
/** @file
 * VirtualPox COM class implementation
 */

/*
 * Copyright (C) 2006-2020 Oracle Corporation
 *
 * This file is part of VirtualPox Open Source Edition (OSE), as
 * available from http://www.virtualpox.org. This file is free software;
 * you can redistribute it and/or modify it under the terms of the GNU
 * General Public License (GPL) as published by the Free Software
 * Foundation, in version 2 as it comes in the "COPYING" file of the
 * VirtualPox OSE distribution. VirtualPox OSE is distributed in the
 * hope that it will be useful, but WITHOUT ANY WARRANTY of any kind.
 */

#define LOG_GROUP LOG_GROUP_MAIN_SYSTEMPROPERTIES
#include "SystemPropertiesImpl.h"
#include "VirtualPoxImpl.h"
#include "MachineImpl.h"
#ifdef VPOX_WITH_EXTPACK
# include "ExtPackManagerImpl.h"
#endif
#include "AutoCaller.h"
#include "Global.h"
#include "LoggingNew.h"
#include "AutostartDb.h"

// generated header
#include "SchemaDefs.h"

#include <iprt/dir.h>
#include <iprt/ldr.h>
#include <iprt/path.h>
#include <iprt/string.h>
#include <iprt/uri.h>
#include <iprt/cpp/utils.h>

#include <iprt/errcore.h>
#include <VPox/param.h>
#include <VPox/settings.h>
#include <VPox/vd.h>

// defines
/////////////////////////////////////////////////////////////////////////////

// constructor / destructor
/////////////////////////////////////////////////////////////////////////////

SystemProperties::SystemProperties()
    : mParent(NULL),
      m(new settings::SystemProperties)
{
}

SystemProperties::~SystemProperties()
{
    delete m;
}


HRESULT SystemProperties::FinalConstruct()
{
    return BaseFinalConstruct();
}

void SystemProperties::FinalRelease()
{
    uninit();
    BaseFinalRelease();
}

// public methods only for internal purposes
/////////////////////////////////////////////////////////////////////////////

/**
 * Initializes the system information object.
 *
 * @returns COM result indicator
 */
HRESULT SystemProperties::init(VirtualPox *aParent)
{
    LogFlowThisFunc(("aParent=%p\n", aParent));

    ComAssertRet(aParent, E_FAIL);

    /* Enclose the state transition NotReady->InInit->Ready */
    AutoInitSpan autoInitSpan(this);
    AssertReturn(autoInitSpan.isOk(), E_FAIL);

    unconst(mParent) = aParent;

    i_setDefaultMachineFolder(Utf8Str::Empty);
    i_setLoggingLevel(Utf8Str::Empty);
    i_setDefaultHardDiskFormat(Utf8Str::Empty);

    i_setVRDEAuthLibrary(Utf8Str::Empty);
    i_setDefaultVRDEExtPack(Utf8Str::Empty);

    m->uLogHistoryCount = 3;


    /* On Windows, OS X and Solaris, HW virtualization use isn't exclusive
     * by default so that VT-x or AMD-V can be shared with other
     * hypervisors without requiring user intervention.
     * NB: See also SystemProperties constructor in settings.h
     */
#if defined(RT_OS_DARWIN) || defined(RT_OS_WINDOWS) || defined(RT_OS_SOLARIS)
    m->fExclusiveHwVirt = false;
#else
    m->fExclusiveHwVirt = true;
#endif

    HRESULT rc = S_OK;

    /* Fetch info of all available hd backends. */

    /// @todo NEWMEDIA VDBackendInfo needs to be improved to let us enumerate
    /// any number of backends

    VDBACKENDINFO aVDInfo[100];
    unsigned cEntries;
    int vrc = VDBackendInfo(RT_ELEMENTS(aVDInfo), aVDInfo, &cEntries);
    AssertRC(vrc);
    if (RT_SUCCESS(vrc))
    {
        for (unsigned i = 0; i < cEntries; ++ i)
        {
            ComObjPtr<MediumFormat> hdf;
            rc = hdf.createObject();
            if (FAILED(rc)) break;

            rc = hdf->init(&aVDInfo[i]);
            if (FAILED(rc)) break;

            m_llMediumFormats.push_back(hdf);
        }
    }

    /* Confirm a successful initialization */
    if (SUCCEEDED(rc))
        autoInitSpan.setSucceeded();

    return rc;
}

/**
 *  Uninitializes the instance and sets the ready flag to FALSE.
 *  Called either from FinalRelease() or by the parent when it gets destroyed.
 */
void SystemProperties::uninit()
{
    LogFlowThisFunc(("\n"));

    /* Enclose the state transition Ready->InUninit->NotReady */
    AutoUninitSpan autoUninitSpan(this);
    if (autoUninitSpan.uninitDone())
        return;

    unconst(mParent) = NULL;
}

// wrapped ISystemProperties properties
/////////////////////////////////////////////////////////////////////////////

HRESULT SystemProperties::getMinGuestRAM(ULONG *minRAM)

{
    /* no need to lock, this is const */
    AssertCompile(MM_RAM_MIN_IN_MB >= SchemaDefs::MinGuestRAM);
    *minRAM = MM_RAM_MIN_IN_MB;

    return S_OK;
}

HRESULT SystemProperties::getMaxGuestRAM(ULONG *maxRAM)
{
    /* no need to lock, this is const */
    AssertCompile(MM_RAM_MAX_IN_MB <= SchemaDefs::MaxGuestRAM);
    ULONG maxRAMSys = MM_RAM_MAX_IN_MB;
    ULONG maxRAMArch = maxRAMSys;
    *maxRAM = RT_MIN(maxRAMSys, maxRAMArch);

    return S_OK;
}

HRESULT SystemProperties::getMinGuestVRAM(ULONG *minVRAM)
{
    /* no need to lock, this is const */
    *minVRAM = SchemaDefs::MinGuestVRAM;

    return S_OK;
}

HRESULT SystemProperties::getMaxGuestVRAM(ULONG *maxVRAM)
{
    /* no need to lock, this is const */
    *maxVRAM = SchemaDefs::MaxGuestVRAM;

    return S_OK;
}

HRESULT SystemProperties::getMinGuestCPUCount(ULONG *minCPUCount)
{
    /* no need to lock, this is const */
    *minCPUCount = SchemaDefs::MinCPUCount; // VMM_MIN_CPU_COUNT

    return S_OK;
}

HRESULT SystemProperties::getMaxGuestCPUCount(ULONG *maxCPUCount)
{
    /* no need to lock, this is const */
    *maxCPUCount = SchemaDefs::MaxCPUCount; // VMM_MAX_CPU_COUNT

    return S_OK;
}

HRESULT SystemProperties::getMaxGuestMonitors(ULONG *maxMonitors)
{

    /* no need to lock, this is const */
    *maxMonitors = SchemaDefs::MaxGuestMonitors;

    return S_OK;
}


HRESULT SystemProperties::getInfoVDSize(LONG64 *infoVDSize)
{
    /*
     * The BIOS supports currently 32 bit LBA numbers (implementing the full
     * 48 bit range is in theory trivial, but the crappy compiler makes things
     * more difficult). This translates to almost 2 TiBytes (to be on the safe
     * side, the reported limit is 1 MiByte less than that, as the total number
     * of sectors should fit in 32 bits, too), which should be enough for the
     * moment. Since the MBR partition tables support only 32bit sector numbers
     * and thus the BIOS can only boot from disks smaller than 2T this is a
     * rather hard limit.
     *
     * The virtual ATA/SATA disks support complete LBA48, and SCSI supports
     * LBA64 (almost, more like LBA55 in practice), so the theoretical maximum
     * disk size is 128 PiByte/16 EiByte. The GUI works nicely with 6 orders
     * of magnitude, but not with 11..13 orders of magnitude.
    */
    /* no need to lock, this is const */
    *infoVDSize = 2 * _1T - _1M;

    return S_OK;
}


HRESULT SystemProperties::getSerialPortCount(ULONG *count)
{
    /* no need to lock, this is const */
    *count = SchemaDefs::SerialPortCount;

    return S_OK;
}


HRESULT SystemProperties::getParallelPortCount(ULONG *count)
{
    /* no need to lock, this is const */
    *count = SchemaDefs::ParallelPortCount;

    return S_OK;
}


HRESULT SystemProperties::getMaxBootPosition(ULONG *aMaxBootPosition)
{
    /* no need to lock, this is const */
    *aMaxBootPosition = SchemaDefs::MaxBootPosition;

    return S_OK;
}


HRESULT SystemProperties::getRawModeSupported(BOOL *aRawModeSupported)
{
    *aRawModeSupported = FALSE;
    return S_OK;
}


HRESULT SystemProperties::getExclusiveHwVirt(BOOL *aExclusiveHwVirt)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aExclusiveHwVirt = m->fExclusiveHwVirt;

    return S_OK;
}

HRESULT SystemProperties::setExclusiveHwVirt(BOOL aExclusiveHwVirt)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->fExclusiveHwVirt = !!aExclusiveHwVirt;
    alock.release();

    // VirtualPox::i_saveSettings() needs vpox write lock
    AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = mParent->i_saveSettings();

    return rc;
}

HRESULT SystemProperties::getMaxNetworkAdapters(ChipsetType_T aChipset, ULONG *aMaxNetworkAdapters)
{
    /* no need for locking, no state */
    uint32_t uResult = Global::getMaxNetworkAdapters(aChipset);
    if (uResult == 0)
        AssertMsgFailed(("Invalid chipset type %d\n", aChipset));
    *aMaxNetworkAdapters = uResult;
    return S_OK;
}

HRESULT SystemProperties::getMaxNetworkAdaptersOfType(ChipsetType_T aChipset, NetworkAttachmentType_T aType, ULONG *count)
{
    /* no need for locking, no state */
    uint32_t uResult = Global::getMaxNetworkAdapters(aChipset);
    if (uResult == 0)
        AssertMsgFailed(("Invalid chipset type %d\n", aChipset));

    switch (aType)
    {
        case NetworkAttachmentType_NAT:
        case NetworkAttachmentType_Internal:
        case NetworkAttachmentType_NATNetwork:
            /* chipset default is OK */
            break;
        case NetworkAttachmentType_Bridged:
            /* Maybe use current host interface count here? */
            break;
        case NetworkAttachmentType_HostOnly:
            uResult = RT_MIN(uResult, 8);
            break;
        default:
            AssertMsgFailed(("Unhandled attachment type %d\n", aType));
    }

    *count = uResult;

    return S_OK;
}


HRESULT SystemProperties::getMaxDevicesPerPortForStorageBus(StorageBus_T aBus,
                                                            ULONG *aMaxDevicesPerPort)
{
    /* no need to lock, this is const */
    switch (aBus)
    {
        case StorageBus_SATA:
        case StorageBus_SCSI:
        case StorageBus_SAS:
        case StorageBus_USB:
        case StorageBus_PCIe:
        case StorageBus_VirtioSCSI:
        {
            /* SATA and both SCSI controllers only support one device per port. */
            *aMaxDevicesPerPort = 1;
            break;
        }
        case StorageBus_IDE:
        case StorageBus_Floppy:
        {
            /* The IDE and Floppy controllers support 2 devices. One as master
             * and one as slave (or floppy drive 0 and 1). */
            *aMaxDevicesPerPort = 2;
            break;
        }
        default:
            AssertMsgFailed(("Invalid bus type %d\n", aBus));
    }

    return S_OK;
}

HRESULT SystemProperties::getMinPortCountForStorageBus(StorageBus_T aBus,
                                                       ULONG *aMinPortCount)
{
    /* no need to lock, this is const */
    switch (aBus)
    {
        case StorageBus_SATA:
        case StorageBus_SAS:
        case StorageBus_PCIe:
        case StorageBus_VirtioSCSI:
        {
            *aMinPortCount = 1;
            break;
        }
        case StorageBus_SCSI:
        {
            *aMinPortCount = 16;
            break;
        }
        case StorageBus_IDE:
        {
            *aMinPortCount = 2;
            break;
        }
        case StorageBus_Floppy:
        {
            *aMinPortCount = 1;
            break;
        }
        case StorageBus_USB:
        {
            *aMinPortCount = 8;
            break;
        }
        default:
            AssertMsgFailed(("Invalid bus type %d\n", aBus));
    }

    return S_OK;
}

HRESULT SystemProperties::getMaxPortCountForStorageBus(StorageBus_T aBus,
                                                       ULONG *aMaxPortCount)
{
    /* no need to lock, this is const */
    switch (aBus)
    {
        case StorageBus_SATA:
        {
            *aMaxPortCount = 30;
            break;
        }
        case StorageBus_SCSI:
        {
            *aMaxPortCount = 16;
            break;
        }
        case StorageBus_IDE:
        {
            *aMaxPortCount = 2;
            break;
        }
        case StorageBus_Floppy:
        {
            *aMaxPortCount = 1;
            break;
        }
        case StorageBus_SAS:
        case StorageBus_PCIe:
        {
            *aMaxPortCount = 255;
            break;
        }
        case StorageBus_USB:
        {
            *aMaxPortCount = 8;
            break;
        }
        case StorageBus_VirtioSCSI:
        {
            *aMaxPortCount = 256;
            break;
        }
        default:
            AssertMsgFailed(("Invalid bus type %d\n", aBus));
    }

    return S_OK;
}

HRESULT SystemProperties::getMaxInstancesOfStorageBus(ChipsetType_T aChipset,
                                                      StorageBus_T  aBus,
                                                      ULONG *aMaxInstances)
{
    ULONG cCtrs = 0;

    /* no need to lock, this is const */
    switch (aBus)
    {
        case StorageBus_SATA:
        case StorageBus_SCSI:
        case StorageBus_SAS:
        case StorageBus_PCIe:
        case StorageBus_VirtioSCSI:
            cCtrs = aChipset == ChipsetType_ICH9 ? 8 : 1;
            break;
        case StorageBus_USB:
        case StorageBus_IDE:
        case StorageBus_Floppy:
        {
            cCtrs = 1;
            break;
        }
        default:
            AssertMsgFailed(("Invalid bus type %d\n", aBus));
    }

    *aMaxInstances = cCtrs;

    return S_OK;
}

HRESULT SystemProperties::getDeviceTypesForStorageBus(StorageBus_T aBus,
                                                      std::vector<DeviceType_T> &aDeviceTypes)
{
    aDeviceTypes.resize(0);

    /* no need to lock, this is const */
    switch (aBus)
    {
        case StorageBus_IDE:
        case StorageBus_SATA:
        case StorageBus_SCSI:
        case StorageBus_SAS:
        case StorageBus_USB:
        case StorageBus_VirtioSCSI:
        {
            aDeviceTypes.resize(2);
            aDeviceTypes[0] = DeviceType_DVD;
            aDeviceTypes[1] = DeviceType_HardDisk;
            break;
        }
        case StorageBus_Floppy:
        {
            aDeviceTypes.resize(1);
            aDeviceTypes[0] = DeviceType_Floppy;
            break;
        }
        case StorageBus_PCIe:
        {
            aDeviceTypes.resize(1);
            aDeviceTypes[0] = DeviceType_HardDisk;
            break;
        }
        default:
            AssertMsgFailed(("Invalid bus type %d\n", aBus));
    }

    return S_OK;
}

HRESULT SystemProperties::getStorageBusForStorageControllerType(StorageControllerType_T aStorageControllerType,
                                                                StorageBus_T *aStorageBus)
{
    /* no need to lock, this is const */
    switch (aStorageControllerType)
    {
        case StorageControllerType_LsiLogic:
        case StorageControllerType_BusLogic:
            *aStorageBus = StorageBus_SCSI;
            break;
        case StorageControllerType_IntelAhci:
            *aStorageBus = StorageBus_SATA;
            break;
        case StorageControllerType_PIIX3:
        case StorageControllerType_PIIX4:
        case StorageControllerType_ICH6:
            *aStorageBus = StorageBus_IDE;
            break;
        case StorageControllerType_I82078:
            *aStorageBus = StorageBus_Floppy;
            break;
        case StorageControllerType_LsiLogicSas:
            *aStorageBus = StorageBus_SAS;
            break;
        case StorageControllerType_USB:
            *aStorageBus = StorageBus_USB;
            break;
        case StorageControllerType_NVMe:
            *aStorageBus = StorageBus_PCIe;
            break;
        case StorageControllerType_VirtioSCSI:
            *aStorageBus = StorageBus_VirtioSCSI;
            break;
        default:
            return setError(E_FAIL, tr("Invalid storage controller type %d\n"), aStorageBus);
    }

    return S_OK;
}

HRESULT SystemProperties::getStorageControllerTypesForStorageBus(StorageBus_T aStorageBus,
                                                                 std::vector<StorageControllerType_T> &aStorageControllerTypes)
{
    aStorageControllerTypes.resize(0);

    /* no need to lock, this is const */
    switch (aStorageBus)
    {
        case StorageBus_IDE:
            aStorageControllerTypes.resize(3);
            aStorageControllerTypes[0] = StorageControllerType_PIIX4;
            aStorageControllerTypes[1] = StorageControllerType_PIIX3;
            aStorageControllerTypes[2] = StorageControllerType_ICH6;
            break;
        case StorageBus_SATA:
            aStorageControllerTypes.resize(1);
            aStorageControllerTypes[0] = StorageControllerType_IntelAhci;
            break;
        case StorageBus_SCSI:
            aStorageControllerTypes.resize(2);
            aStorageControllerTypes[0] = StorageControllerType_LsiLogic;
            aStorageControllerTypes[1] = StorageControllerType_BusLogic;
            break;
        case StorageBus_Floppy:
            aStorageControllerTypes.resize(1);
            aStorageControllerTypes[0] = StorageControllerType_I82078;
            break;
        case StorageBus_SAS:
            aStorageControllerTypes.resize(1);
            aStorageControllerTypes[0] = StorageControllerType_LsiLogicSas;
            break;
        case StorageBus_USB:
            aStorageControllerTypes.resize(1);
            aStorageControllerTypes[0] = StorageControllerType_USB;
            break;
        case StorageBus_PCIe:
            aStorageControllerTypes.resize(1);
            aStorageControllerTypes[0] = StorageControllerType_NVMe;
            break;
        case StorageBus_VirtioSCSI:
            aStorageControllerTypes.resize(1);
            aStorageControllerTypes[0] = StorageControllerType_VirtioSCSI;
            break;
        default:
            return setError(E_FAIL, tr("Invalid storage bus %d\n"), aStorageBus);
    }

    return S_OK;
}

HRESULT SystemProperties::getDefaultIoCacheSettingForStorageController(StorageControllerType_T aControllerType,
                                                                       BOOL *aEnabled)
{
    /* no need to lock, this is const */
    switch (aControllerType)
    {
        case StorageControllerType_LsiLogic:
        case StorageControllerType_BusLogic:
        case StorageControllerType_IntelAhci:
        case StorageControllerType_LsiLogicSas:
        case StorageControllerType_USB:
        case StorageControllerType_NVMe:
        case StorageControllerType_VirtioSCSI:
            *aEnabled = false;
            break;
        case StorageControllerType_PIIX3:
        case StorageControllerType_PIIX4:
        case StorageControllerType_ICH6:
        case StorageControllerType_I82078:
            *aEnabled = true;
            break;
        default:
            AssertMsgFailed(("Invalid controller type %d\n", aControllerType));
    }
    return S_OK;
}

HRESULT SystemProperties::getStorageControllerHotplugCapable(StorageControllerType_T aControllerType,
                                                             BOOL *aHotplugCapable)
{
    switch (aControllerType)
    {
        case StorageControllerType_IntelAhci:
        case StorageControllerType_USB:
            *aHotplugCapable = true;
            break;
        case StorageControllerType_LsiLogic:
        case StorageControllerType_LsiLogicSas:
        case StorageControllerType_BusLogic:
        case StorageControllerType_NVMe:
        case StorageControllerType_VirtioSCSI:
        case StorageControllerType_PIIX3:
        case StorageControllerType_PIIX4:
        case StorageControllerType_ICH6:
        case StorageControllerType_I82078:
            *aHotplugCapable = false;
            break;
        default:
            AssertMsgFailedReturn(("Invalid controller type %d\n", aControllerType), E_FAIL);
    }

    return S_OK;
}

HRESULT SystemProperties::getMaxInstancesOfUSBControllerType(ChipsetType_T aChipset,
                                                             USBControllerType_T aType,
                                                             ULONG *aMaxInstances)
{
    NOREF(aChipset);
    ULONG cCtrs = 0;

    /* no need to lock, this is const */
    switch (aType)
    {
        case USBControllerType_OHCI:
        case USBControllerType_EHCI:
        case USBControllerType_XHCI:
        {
            cCtrs = 1;
            break;
        }
        default:
            AssertMsgFailed(("Invalid bus type %d\n", aType));
    }

    *aMaxInstances = cCtrs;

    return S_OK;
}

HRESULT SystemProperties::getDefaultMachineFolder(com::Utf8Str &aDefaultMachineFolder)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aDefaultMachineFolder = m->strDefaultMachineFolder;
    return S_OK;
}

HRESULT SystemProperties::setDefaultMachineFolder(const com::Utf8Str &aDefaultMachineFolder)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setDefaultMachineFolder(aDefaultMachineFolder);
    alock.release();
    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}

HRESULT SystemProperties::getLoggingLevel(com::Utf8Str &aLoggingLevel)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aLoggingLevel = m->strLoggingLevel;

    if (aLoggingLevel.isEmpty())
        aLoggingLevel = VPOXSVC_LOG_DEFAULT;

    return S_OK;
}


HRESULT SystemProperties::setLoggingLevel(const com::Utf8Str &aLoggingLevel)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setLoggingLevel(aLoggingLevel);
    alock.release();

    if (SUCCEEDED(rc))
    {
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }
    else
        LogRel(("Cannot set passed logging level=%s, or the default one - Error=%Rhrc \n", aLoggingLevel.c_str(), rc));

    return rc;
}

HRESULT SystemProperties::getMediumFormats(std::vector<ComPtr<IMediumFormat> > &aMediumFormats)
{
    MediumFormatList mediumFormats(m_llMediumFormats);
    aMediumFormats.resize(mediumFormats.size());
    size_t i = 0;
    for (MediumFormatList::const_iterator it = mediumFormats.begin(); it != mediumFormats.end(); ++it, ++i)
        (*it).queryInterfaceTo(aMediumFormats[i].asOutParam());
    return S_OK;
}

HRESULT SystemProperties::getDefaultHardDiskFormat(com::Utf8Str &aDefaultHardDiskFormat)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aDefaultHardDiskFormat = m->strDefaultHardDiskFormat;
    return S_OK;
}


HRESULT SystemProperties::setDefaultHardDiskFormat(const com::Utf8Str &aDefaultHardDiskFormat)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setDefaultHardDiskFormat(aDefaultHardDiskFormat);
    alock.release();
    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}

HRESULT SystemProperties::getFreeDiskSpaceWarning(LONG64 *aFreeSpace)
{
    NOREF(aFreeSpace);
    ReturnComNotImplemented();
}

HRESULT SystemProperties::setFreeDiskSpaceWarning(LONG64 /* aFreeSpace */)
{
    ReturnComNotImplemented();
}

HRESULT SystemProperties::getFreeDiskSpacePercentWarning(ULONG *aFreeSpacePercent)
{
    NOREF(aFreeSpacePercent);
    ReturnComNotImplemented();
}

HRESULT SystemProperties::setFreeDiskSpacePercentWarning(ULONG /* aFreeSpacePercent */)
{
    ReturnComNotImplemented();
}

HRESULT SystemProperties::getFreeDiskSpaceError(LONG64 *aFreeSpace)
{
    NOREF(aFreeSpace);
    ReturnComNotImplemented();
}

HRESULT SystemProperties::setFreeDiskSpaceError(LONG64 /* aFreeSpace */)
{
    ReturnComNotImplemented();
}

HRESULT SystemProperties::getFreeDiskSpacePercentError(ULONG *aFreeSpacePercent)
{
    NOREF(aFreeSpacePercent);
    ReturnComNotImplemented();
}

HRESULT SystemProperties::setFreeDiskSpacePercentError(ULONG /* aFreeSpacePercent */)
{
    ReturnComNotImplemented();
}

HRESULT SystemProperties::getVRDEAuthLibrary(com::Utf8Str &aVRDEAuthLibrary)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aVRDEAuthLibrary = m->strVRDEAuthLibrary;

    return S_OK;
}

HRESULT SystemProperties::setVRDEAuthLibrary(const com::Utf8Str &aVRDEAuthLibrary)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setVRDEAuthLibrary(aVRDEAuthLibrary);
    alock.release();
    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}

HRESULT SystemProperties::getWebServiceAuthLibrary(com::Utf8Str &aWebServiceAuthLibrary)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aWebServiceAuthLibrary = m->strWebServiceAuthLibrary;

    return S_OK;
}

HRESULT SystemProperties::setWebServiceAuthLibrary(const com::Utf8Str &aWebServiceAuthLibrary)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setWebServiceAuthLibrary(aWebServiceAuthLibrary);
    alock.release();

    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}

HRESULT SystemProperties::getDefaultVRDEExtPack(com::Utf8Str &aExtPack)
{
    HRESULT hrc = S_OK;
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    Utf8Str strExtPack(m->strDefaultVRDEExtPack);
    if (strExtPack.isNotEmpty())
    {
        if (strExtPack.equals(VPOXVRDP_KLUDGE_EXTPACK_NAME))
            hrc = S_OK;
        else
#ifdef VPOX_WITH_EXTPACK
            hrc = mParent->i_getExtPackManager()->i_checkVrdeExtPack(&strExtPack);
#else
            hrc = setError(E_FAIL, tr("The extension pack '%s' does not exist"), strExtPack.c_str());
#endif
    }
    else
    {
#ifdef VPOX_WITH_EXTPACK
        hrc = mParent->i_getExtPackManager()->i_getDefaultVrdeExtPack(&strExtPack);
#endif
        if (strExtPack.isEmpty())
        {
            /*
            * Klugde - check if VPoxVRDP.dll/.so/.dylib is installed.
            * This is hardcoded uglyness, sorry.
            */
            char szPath[RTPATH_MAX];
            int vrc = RTPathAppPrivateArch(szPath, sizeof(szPath));
            if (RT_SUCCESS(vrc))
                vrc = RTPathAppend(szPath, sizeof(szPath), "VPoxVRDP");
            if (RT_SUCCESS(vrc))
                vrc = RTStrCat(szPath, sizeof(szPath), RTLdrGetSuff());
            if (RT_SUCCESS(vrc) && RTFileExists(szPath))
            {
                /* Illegal extpack name, so no conflict. */
                strExtPack = VPOXVRDP_KLUDGE_EXTPACK_NAME;
            }
        }
    }

    if (SUCCEEDED(hrc))
          aExtPack = strExtPack;

    return S_OK;
}


HRESULT SystemProperties::setDefaultVRDEExtPack(const com::Utf8Str &aExtPack)
{
    HRESULT hrc = S_OK;
    if (aExtPack.isNotEmpty())
    {
        if (aExtPack.equals(VPOXVRDP_KLUDGE_EXTPACK_NAME))
            hrc = S_OK;
        else
#ifdef VPOX_WITH_EXTPACK
            hrc = mParent->i_getExtPackManager()->i_checkVrdeExtPack(&aExtPack);
#else
            hrc = setError(E_FAIL, tr("The extension pack '%s' does not exist"), aExtPack.c_str());
#endif
    }
    if (SUCCEEDED(hrc))
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        hrc = i_setDefaultVRDEExtPack(aExtPack);
        if (SUCCEEDED(hrc))
        {
            /* VirtualPox::i_saveSettings() needs the VirtualPox write lock. */
            alock.release();
            AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
            hrc = mParent->i_saveSettings();
        }
    }

    return hrc;
}


HRESULT SystemProperties::getLogHistoryCount(ULONG *count)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *count = m->uLogHistoryCount;

    return S_OK;
}


HRESULT SystemProperties::setLogHistoryCount(ULONG count)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    m->uLogHistoryCount = count;
    alock.release();

    // VirtualPox::i_saveSettings() needs vpox write lock
    AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = mParent->i_saveSettings();

    return rc;
}

HRESULT SystemProperties::getDefaultAudioDriver(AudioDriverType_T *aAudioDriver)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    *aAudioDriver = settings::MachineConfigFile::getHostDefaultAudioDriver();

    return S_OK;
}

HRESULT SystemProperties::getAutostartDatabasePath(com::Utf8Str &aAutostartDbPath)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    aAutostartDbPath = m->strAutostartDatabasePath;

    return S_OK;
}

HRESULT SystemProperties::setAutostartDatabasePath(const com::Utf8Str &aAutostartDbPath)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setAutostartDatabasePath(aAutostartDbPath);
    alock.release();

    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}

HRESULT SystemProperties::getDefaultAdditionsISO(com::Utf8Str &aDefaultAdditionsISO)
{
    return i_getDefaultAdditionsISO(aDefaultAdditionsISO);
}

HRESULT SystemProperties::setDefaultAdditionsISO(const com::Utf8Str &aDefaultAdditionsISO)
{
    RT_NOREF(aDefaultAdditionsISO);
    /** @todo not yet implemented, settings handling is missing */
    ReturnComNotImplemented();
#if 0 /* not implemented */
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = i_setDefaultAdditionsISO(aDefaultAdditionsISO);
    alock.release();

    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
#endif
}

HRESULT SystemProperties::getDefaultFrontend(com::Utf8Str &aDefaultFrontend)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aDefaultFrontend = m->strDefaultFrontend;
    return S_OK;
}

HRESULT SystemProperties::setDefaultFrontend(const com::Utf8Str &aDefaultFrontend)
{
    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (m->strDefaultFrontend == Utf8Str(aDefaultFrontend))
        return S_OK;
    HRESULT rc = i_setDefaultFrontend(aDefaultFrontend);
    alock.release();

    if (SUCCEEDED(rc))
    {
        // VirtualPox::i_saveSettings() needs vpox write lock
        AutoWriteLock vpoxLock(mParent COMMA_LOCKVAL_SRC_POS);
        rc = mParent->i_saveSettings();
    }

    return rc;
}

HRESULT SystemProperties::getScreenShotFormats(std::vector<BitmapFormat_T> &aBitmapFormats)
{
    aBitmapFormats.push_back(BitmapFormat_BGR0);
    aBitmapFormats.push_back(BitmapFormat_BGRA);
    aBitmapFormats.push_back(BitmapFormat_RGBA);
    aBitmapFormats.push_back(BitmapFormat_PNG);
    return S_OK;
}

HRESULT SystemProperties::getProxyMode(ProxyMode_T *pProxyMode)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    ProxyMode_T enmMode = *pProxyMode = (ProxyMode_T)m->uProxyMode;
    AssertMsgReturn(enmMode == ProxyMode_System || enmMode == ProxyMode_NoProxy || enmMode == ProxyMode_Manual,
                    ("enmMode=%d\n", enmMode), E_UNEXPECTED);
    return S_OK;
}

HRESULT SystemProperties::setProxyMode(ProxyMode_T aProxyMode)
{
    /* Validate input. */
    switch (aProxyMode)
    {
        case ProxyMode_System:
        case ProxyMode_NoProxy:
        case ProxyMode_Manual:
            break;
        default:
            return setError(E_INVALIDARG, tr("Invalid ProxyMode value: %d"), (int)aProxyMode);
    }

    /* Set and write out settings. */
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        m->uProxyMode = aProxyMode;
    }
    AutoWriteLock alock(mParent COMMA_LOCKVAL_SRC_POS); /* required for saving. */
    return mParent->i_saveSettings();
}

HRESULT SystemProperties::getProxyURL(com::Utf8Str &aProxyURL)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    aProxyURL = m->strProxyUrl;
    return S_OK;
}

HRESULT SystemProperties::setProxyURL(const com::Utf8Str &aProxyURL)
{
    /*
     * Validate input.
     */
    Utf8Str const *pStrProxyUrl = &aProxyURL;
    Utf8Str strTmp;
    if (pStrProxyUrl->isNotEmpty())
    {
        /* RTUriParse requires a scheme, so append 'http://' if none seems present: */
        if (pStrProxyUrl->find("://") == RTCString::npos)
        {
            strTmp.printf("http://%s", aProxyURL.c_str());
            pStrProxyUrl = &strTmp;
        }

        /* Use RTUriParse to check the format.  There must be a hostname, but nothing
           can follow it and the port. */
        RTURIPARSED Parsed;
        int vrc = RTUriParse(pStrProxyUrl->c_str(), &Parsed);
        if (RT_FAILURE(vrc))
            return setErrorBoth(E_INVALIDARG, vrc, tr("Failed to parse proxy URL: %Rrc"), vrc);
        if (   Parsed.cchAuthorityHost == 0
            && !RTUriIsSchemeMatch(pStrProxyUrl->c_str(), "direct"))
            return setError(E_INVALIDARG, tr("Proxy URL must include a hostname"));
        if (Parsed.cchPath > 0)
            return setError(E_INVALIDARG, tr("Proxy URL must not include a path component (%.*s)"),
                            Parsed.cchPath, pStrProxyUrl->c_str() + Parsed.offPath);
        if (Parsed.cchQuery > 0)
            return setError(E_INVALIDARG, tr("Proxy URL must not include a query component (?%.*s)"),
                            Parsed.cchQuery, pStrProxyUrl->c_str() + Parsed.offQuery);
        if (Parsed.cchFragment > 0)
            return setError(E_INVALIDARG, tr("Proxy URL must not include a fragment component (#%.*s)"),
                            Parsed.cchFragment, pStrProxyUrl->c_str() + Parsed.offFragment);
    }

    /*
     * Set and write out settings.
     */
    {
        AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
        m->strProxyUrl = *pStrProxyUrl;
    }
    AutoWriteLock alock(mParent COMMA_LOCKVAL_SRC_POS); /* required for saving. */
    return mParent->i_saveSettings();
}

HRESULT SystemProperties::getSupportedParavirtProviders(std::vector<ParavirtProvider_T> &aSupportedParavirtProviders)
{
    static const ParavirtProvider_T aParavirtProviders[] =
    {
        ParavirtProvider_None,
        ParavirtProvider_Default,
        ParavirtProvider_Legacy,
        ParavirtProvider_Minimal,
        ParavirtProvider_HyperV,
        ParavirtProvider_KVM,
    };
    aSupportedParavirtProviders.assign(aParavirtProviders,
                                       aParavirtProviders + RT_ELEMENTS(aParavirtProviders));
    return S_OK;
}

HRESULT SystemProperties::getSupportedClipboardModes(std::vector<ClipboardMode_T> &aSupportedClipboardModes)
{
    static const ClipboardMode_T aClipboardModes[] =
    {
        ClipboardMode_Disabled,
        ClipboardMode_HostToGuest,
        ClipboardMode_GuestToHost,
        ClipboardMode_Bidirectional,
    };
    aSupportedClipboardModes.assign(aClipboardModes,
                                    aClipboardModes + RT_ELEMENTS(aClipboardModes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedDnDModes(std::vector<DnDMode_T> &aSupportedDnDModes)
{
    static const DnDMode_T aDnDModes[] =
    {
        DnDMode_Disabled,
        DnDMode_HostToGuest,
        DnDMode_GuestToHost,
        DnDMode_Bidirectional,
    };
    aSupportedDnDModes.assign(aDnDModes,
                              aDnDModes + RT_ELEMENTS(aDnDModes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedFirmwareTypes(std::vector<FirmwareType_T> &aSupportedFirmwareTypes)
{
    static const FirmwareType_T aFirmwareTypes[] =
    {
        FirmwareType_BIOS,
        FirmwareType_EFI,
        FirmwareType_EFI32,
        FirmwareType_EFI64,
        FirmwareType_EFIDUAL,
    };
    aSupportedFirmwareTypes.assign(aFirmwareTypes,
                                   aFirmwareTypes + RT_ELEMENTS(aFirmwareTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedPointingHIDTypes(std::vector<PointingHIDType_T> &aSupportedPointingHIDTypes)
{
    static const PointingHIDType_T aPointingHIDTypes[] =
    {
        PointingHIDType_PS2Mouse,
#ifdef DEBUG
        PointingHIDType_USBMouse,
#endif
        PointingHIDType_USBTablet,
#ifdef DEBUG
        PointingHIDType_ComboMouse,
#endif
        PointingHIDType_USBMultiTouch,
    };
    aSupportedPointingHIDTypes.assign(aPointingHIDTypes,
                                      aPointingHIDTypes + RT_ELEMENTS(aPointingHIDTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedKeyboardHIDTypes(std::vector<KeyboardHIDType_T> &aSupportedKeyboardHIDTypes)
{
    static const KeyboardHIDType_T aKeyboardHIDTypes[] =
    {
        KeyboardHIDType_PS2Keyboard,
        KeyboardHIDType_USBKeyboard,
#ifdef DEBUG
        KeyboardHIDType_ComboKeyboard,
#endif
    };
    aSupportedKeyboardHIDTypes.assign(aKeyboardHIDTypes,
                                      aKeyboardHIDTypes + RT_ELEMENTS(aKeyboardHIDTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedVFSTypes(std::vector<VFSType_T> &aSupportedVFSTypes)
{
    static const VFSType_T aVFSTypes[] =
    {
        VFSType_File,
        VFSType_Cloud,
        VFSType_S3,
#ifdef DEBUG
        VFSType_WebDav,
#endif
    };
    aSupportedVFSTypes.assign(aVFSTypes,
                              aVFSTypes + RT_ELEMENTS(aVFSTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedImportOptions(std::vector<ImportOptions_T> &aSupportedImportOptions)
{
    static const ImportOptions_T aImportOptions[] =
    {
        ImportOptions_KeepAllMACs,
        ImportOptions_KeepNATMACs,
        ImportOptions_ImportToVDI,
    };
    aSupportedImportOptions.assign(aImportOptions,
                                   aImportOptions + RT_ELEMENTS(aImportOptions));
    return S_OK;
}

HRESULT SystemProperties::getSupportedExportOptions(std::vector<ExportOptions_T> &aSupportedExportOptions)
{
    static const ExportOptions_T aExportOptions[] =
    {
        ExportOptions_CreateManifest,
        ExportOptions_ExportDVDImages,
        ExportOptions_StripAllMACs,
        ExportOptions_StripAllNonNATMACs,
    };
    aSupportedExportOptions.assign(aExportOptions,
                                   aExportOptions + RT_ELEMENTS(aExportOptions));
    return S_OK;
}

HRESULT SystemProperties::getSupportedRecordingAudioCodecs(std::vector<RecordingAudioCodec_T> &aSupportedRecordingAudioCodecs)
{
    static const RecordingAudioCodec_T aRecordingAudioCodecs[] =
    {
#ifdef DEBUG
        RecordingAudioCodec_WavPCM,
#endif
        RecordingAudioCodec_Opus,
    };
    aSupportedRecordingAudioCodecs.assign(aRecordingAudioCodecs,
                                          aRecordingAudioCodecs + RT_ELEMENTS(aRecordingAudioCodecs));
    return S_OK;
}

HRESULT SystemProperties::getSupportedRecordingVideoCodecs(std::vector<RecordingVideoCodec_T> &aSupportedRecordingVideoCodecs)
{
    static const RecordingVideoCodec_T aRecordingVideoCodecs[] =
    {
        RecordingVideoCodec_VP8,
#ifdef DEBUG
        RecordingVideoCodec_VP9,
        RecordingVideoCodec_AV1,
#endif
    };
    aSupportedRecordingVideoCodecs.assign(aRecordingVideoCodecs,
                                          aRecordingVideoCodecs + RT_ELEMENTS(aRecordingVideoCodecs));
    return S_OK;
}

HRESULT SystemProperties::getSupportedRecordingVSMethods(std::vector<RecordingVideoScalingMethod_T> &aSupportedRecordingVideoScalingMethods)
{
    static const RecordingVideoScalingMethod_T aRecordingVideoScalingMethods[] =
    {
        RecordingVideoScalingMethod_None,
#ifdef DEBUG
        RecordingVideoScalingMethod_NearestNeighbor,
        RecordingVideoScalingMethod_Bilinear,
        RecordingVideoScalingMethod_Bicubic,
#endif
    };
    aSupportedRecordingVideoScalingMethods.assign(aRecordingVideoScalingMethods,
                                                  aRecordingVideoScalingMethods + RT_ELEMENTS(aRecordingVideoScalingMethods));
    return S_OK;
}

HRESULT SystemProperties::getSupportedRecordingVRCModes(std::vector<RecordingVideoRateControlMode_T> &aSupportedRecordingVideoRateControlModes)
{
    static const RecordingVideoRateControlMode_T aRecordingVideoRateControlModes[] =
    {
        RecordingVideoRateControlMode_CBR,
#ifdef DEBUG
        RecordingVideoRateControlMode_VBR,
#endif
    };
    aSupportedRecordingVideoRateControlModes.assign(aRecordingVideoRateControlModes,
                                                    aRecordingVideoRateControlModes + RT_ELEMENTS(aRecordingVideoRateControlModes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedGraphicsControllerTypes(std::vector<GraphicsControllerType_T> &aSupportedGraphicsControllerTypes)
{
    static const GraphicsControllerType_T aGraphicsControllerTypes[] =
    {
        GraphicsControllerType_VPoxVGA,
        GraphicsControllerType_VMSVGA,
        GraphicsControllerType_VPoxSVGA,
        GraphicsControllerType_Null,
    };
    aSupportedGraphicsControllerTypes.assign(aGraphicsControllerTypes,
                                             aGraphicsControllerTypes + RT_ELEMENTS(aGraphicsControllerTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedCloneOptions(std::vector<CloneOptions_T> &aSupportedCloneOptions)
{
    static const CloneOptions_T aCloneOptions[] =
    {
        CloneOptions_Link,
        CloneOptions_KeepAllMACs,
        CloneOptions_KeepNATMACs,
        CloneOptions_KeepDiskNames,
        CloneOptions_KeepHwUUIDs,
    };
    aSupportedCloneOptions.assign(aCloneOptions,
                                  aCloneOptions + RT_ELEMENTS(aCloneOptions));
    return S_OK;
}

HRESULT SystemProperties::getSupportedAutostopTypes(std::vector<AutostopType_T> &aSupportedAutostopTypes)
{
    static const AutostopType_T aAutostopTypes[] =
    {
        AutostopType_Disabled,
        AutostopType_SaveState,
        AutostopType_PowerOff,
        AutostopType_AcpiShutdown,
    };
    aSupportedAutostopTypes.assign(aAutostopTypes,
                                   aAutostopTypes + RT_ELEMENTS(aAutostopTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedVMProcPriorities(std::vector<VMProcPriority_T> &aSupportedVMProcPriorities)
{
    static const VMProcPriority_T aVMProcPriorities[] =
    {
        VMProcPriority_Default,
        VMProcPriority_Flat,
        VMProcPriority_Low,
        VMProcPriority_Normal,
        VMProcPriority_High,
    };
    aSupportedVMProcPriorities.assign(aVMProcPriorities,
                                      aVMProcPriorities + RT_ELEMENTS(aVMProcPriorities));
    return S_OK;
}

HRESULT SystemProperties::getSupportedNetworkAttachmentTypes(std::vector<NetworkAttachmentType_T> &aSupportedNetworkAttachmentTypes)
{
    static const NetworkAttachmentType_T aNetworkAttachmentTypes[] =
    {
        NetworkAttachmentType_NAT,
        NetworkAttachmentType_Bridged,
        NetworkAttachmentType_Internal,
        NetworkAttachmentType_HostOnly,
        NetworkAttachmentType_Generic,
        NetworkAttachmentType_NATNetwork,
#ifdef VPOX_WITH_CLOUD_NET
        NetworkAttachmentType_Cloud,
#endif
        NetworkAttachmentType_Null,
    };
    aSupportedNetworkAttachmentTypes.assign(aNetworkAttachmentTypes,
                                            aNetworkAttachmentTypes + RT_ELEMENTS(aNetworkAttachmentTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedNetworkAdapterTypes(std::vector<NetworkAdapterType_T> &aSupportedNetworkAdapterTypes)
{
    static const NetworkAdapterType_T aNetworkAdapterTypes[] =
    {
        NetworkAdapterType_Am79C970A,
        NetworkAdapterType_Am79C973,
        NetworkAdapterType_I82540EM,
        NetworkAdapterType_I82543GC,
        NetworkAdapterType_I82545EM,
        NetworkAdapterType_Virtio,
#ifdef VPOX_WITH_VIRTIO_NET_1_0
        NetworkAdapterType_Virtio_1_0,
#endif
    };
    aSupportedNetworkAdapterTypes.assign(aNetworkAdapterTypes,
                                         aNetworkAdapterTypes + RT_ELEMENTS(aNetworkAdapterTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedPortModes(std::vector<PortMode_T> &aSupportedPortModes)
{
    static const PortMode_T aPortModes[] =
    {
        PortMode_Disconnected,
        PortMode_HostPipe,
        PortMode_HostDevice,
        PortMode_RawFile,
        PortMode_TCP,
    };
    aSupportedPortModes.assign(aPortModes,
                               aPortModes + RT_ELEMENTS(aPortModes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedUartTypes(std::vector<UartType_T> &aSupportedUartTypes)
{
    static const UartType_T aUartTypes[] =
    {
        UartType_U16450,
        UartType_U16550A,
        UartType_U16750,
    };
    aSupportedUartTypes.assign(aUartTypes,
                               aUartTypes + RT_ELEMENTS(aUartTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedUSBControllerTypes(std::vector<USBControllerType_T> &aSupportedUSBControllerTypes)
{
    static const USBControllerType_T aUSBControllerTypesWithoutExtPack[] =
    {
        USBControllerType_OHCI,
    };
    static const USBControllerType_T aUSBControllerTypesWithExtPack[] =
    {
        USBControllerType_OHCI,
        USBControllerType_EHCI,
        USBControllerType_XHCI,
    };
    bool fExtPack = false;
# ifdef VPOX_WITH_EXTPACK
    static const char *s_pszUsbExtPackName = "Oracle VM VirtualPox Extension Pack";
    if (mParent->i_getExtPackManager()->i_isExtPackUsable(s_pszUsbExtPackName))
# endif
    {
        fExtPack = true;
    }

    if (fExtPack)
        aSupportedUSBControllerTypes.assign(aUSBControllerTypesWithExtPack,
                                            aUSBControllerTypesWithExtPack + RT_ELEMENTS(aUSBControllerTypesWithExtPack));
    else
        aSupportedUSBControllerTypes.assign(aUSBControllerTypesWithoutExtPack,
                                            aUSBControllerTypesWithoutExtPack + RT_ELEMENTS(aUSBControllerTypesWithoutExtPack));
    return S_OK;
}

HRESULT SystemProperties::getSupportedAudioDriverTypes(std::vector<AudioDriverType_T> &aSupportedAudioDriverTypes)
{
    static const AudioDriverType_T aAudioDriverTypes[] =
    {
#ifdef RT_OS_WINDOWS
# if 0 /* deprecated for many years now */
        AudioDriverType_WinMM,
# endif
        AudioDriverType_DirectSound,
#endif
#ifdef RT_OS_DARWIN
        AudioDriverType_CoreAudio,
#endif
#ifdef RT_OS_OS2
        AudioDriverType_MMPM,
#endif
#ifdef RT_OS_SOLARIS
# if 0 /* deprecated for many years now */
        AudioDriverType_SolAudio,
# endif
#endif
#ifdef VPOX_WITH_AUDIO_ALSA
        AudioDriverType_ALSA,
#endif
#ifdef VPOX_WITH_AUDIO_OSS
        AudioDriverType_OSS,
#endif
#ifdef VPOX_WITH_AUDIO_PULSE
        AudioDriverType_Pulse,
#endif
        AudioDriverType_Null,
    };
    aSupportedAudioDriverTypes.assign(aAudioDriverTypes,
                                      aAudioDriverTypes + RT_ELEMENTS(aAudioDriverTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedAudioControllerTypes(std::vector<AudioControllerType_T> &aSupportedAudioControllerTypes)
{
    static const AudioControllerType_T aAudioControllerTypes[] =
    {
        AudioControllerType_AC97,
        AudioControllerType_SB16,
        AudioControllerType_HDA,
    };
    aSupportedAudioControllerTypes.assign(aAudioControllerTypes,
                                          aAudioControllerTypes + RT_ELEMENTS(aAudioControllerTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedStorageBuses(std::vector<StorageBus_T> &aSupportedStorageBuses)
{
    static const StorageBus_T aStorageBuses[] =
    {
        StorageBus_SATA,
        StorageBus_IDE,
        StorageBus_SCSI,
        StorageBus_Floppy,
        StorageBus_SAS,
        StorageBus_USB,
        StorageBus_PCIe,
        StorageBus_VirtioSCSI,
    };
    aSupportedStorageBuses.assign(aStorageBuses,
                                  aStorageBuses + RT_ELEMENTS(aStorageBuses));
    return S_OK;
}

HRESULT SystemProperties::getSupportedStorageControllerTypes(std::vector<StorageControllerType_T> &aSupportedStorageControllerTypes)
{
    static const StorageControllerType_T aStorageControllerTypes[] =
    {
        StorageControllerType_IntelAhci,
        StorageControllerType_PIIX4,
        StorageControllerType_PIIX3,
        StorageControllerType_ICH6,
        StorageControllerType_LsiLogic,
        StorageControllerType_BusLogic,
        StorageControllerType_I82078,
        StorageControllerType_LsiLogicSas,
        StorageControllerType_USB,
        StorageControllerType_NVMe,
        StorageControllerType_VirtioSCSI,
    };
    aSupportedStorageControllerTypes.assign(aStorageControllerTypes,
                                            aStorageControllerTypes + RT_ELEMENTS(aStorageControllerTypes));
    return S_OK;
}

HRESULT SystemProperties::getSupportedChipsetTypes(std::vector<ChipsetType_T> &aSupportedChipsetTypes)
{
    static const ChipsetType_T aChipsetTypes[] =
    {
        ChipsetType_PIIX3,
        ChipsetType_ICH9,
    };
    aSupportedChipsetTypes.assign(aChipsetTypes,
                                  aChipsetTypes + RT_ELEMENTS(aChipsetTypes));
    return S_OK;
}


// public methods only for internal purposes
/////////////////////////////////////////////////////////////////////////////

HRESULT SystemProperties::i_loadSettings(const settings::SystemProperties &data)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoWriteLock alock(this COMMA_LOCKVAL_SRC_POS);
    HRESULT rc = S_OK;
    rc = i_setDefaultMachineFolder(data.strDefaultMachineFolder);
    if (FAILED(rc)) return rc;

    rc = i_setLoggingLevel(data.strLoggingLevel);
    if (FAILED(rc)) return rc;

    rc = i_setDefaultHardDiskFormat(data.strDefaultHardDiskFormat);
    if (FAILED(rc)) return rc;

    rc = i_setVRDEAuthLibrary(data.strVRDEAuthLibrary);
    if (FAILED(rc)) return rc;

    rc = i_setWebServiceAuthLibrary(data.strWebServiceAuthLibrary);
    if (FAILED(rc)) return rc;

    rc = i_setDefaultVRDEExtPack(data.strDefaultVRDEExtPack);
    if (FAILED(rc)) return rc;

    m->uLogHistoryCount  = data.uLogHistoryCount;
    m->fExclusiveHwVirt  = data.fExclusiveHwVirt;
    m->uProxyMode        = data.uProxyMode;
    m->strProxyUrl       = data.strProxyUrl;

    rc = i_setAutostartDatabasePath(data.strAutostartDatabasePath);
    if (FAILED(rc)) return rc;

    {
        /* must ignore errors signalled here, because the guest additions
         * file may not exist, and in this case keep the empty string */
        ErrorInfoKeeper eik;
        (void)i_setDefaultAdditionsISO(data.strDefaultAdditionsISO);
    }

    rc = i_setDefaultFrontend(data.strDefaultFrontend);
    if (FAILED(rc)) return rc;

    return S_OK;
}

HRESULT SystemProperties::i_saveSettings(settings::SystemProperties &data)
{
    AutoCaller autoCaller(this);
    if (FAILED(autoCaller.rc())) return autoCaller.rc();

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    data = *m;

    return S_OK;
}

/**
 * Returns a medium format object corresponding to the given format
 * identifier or null if no such format.
 *
 * @param aFormat   Format identifier.
 *
 * @return ComObjPtr<MediumFormat>
 */
ComObjPtr<MediumFormat> SystemProperties::i_mediumFormat(const Utf8Str &aFormat)
{
    ComObjPtr<MediumFormat> format;

    AutoCaller autoCaller(this);
    AssertComRCReturn (autoCaller.rc(), format);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    for (MediumFormatList::const_iterator it = m_llMediumFormats.begin();
         it != m_llMediumFormats.end();
         ++ it)
    {
        /* MediumFormat is all const, no need to lock */

        if ((*it)->i_getId().compare(aFormat, Utf8Str::CaseInsensitive) == 0)
        {
            format = *it;
            break;
        }
    }

    return format;
}

/**
 * Returns a medium format object corresponding to the given file extension or
 * null if no such format.
 *
 * @param aExt   File extension.
 *
 * @return ComObjPtr<MediumFormat>
 */
ComObjPtr<MediumFormat> SystemProperties::i_mediumFormatFromExtension(const Utf8Str &aExt)
{
    ComObjPtr<MediumFormat> format;

    AutoCaller autoCaller(this);
    AssertComRCReturn (autoCaller.rc(), format);

    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);

    bool fFound = false;
    for (MediumFormatList::const_iterator it = m_llMediumFormats.begin();
         it != m_llMediumFormats.end() && !fFound;
         ++it)
    {
        /* MediumFormat is all const, no need to lock */
        MediumFormat::StrArray aFileList = (*it)->i_getFileExtensions();
        for (MediumFormat::StrArray::const_iterator it1 = aFileList.begin();
             it1 != aFileList.end();
             ++it1)
        {
            if ((*it1).compare(aExt, Utf8Str::CaseInsensitive) == 0)
            {
                format = *it;
                fFound = true;
                break;
            }
        }
    }

    return format;
}


/**
 * VD plugin load
 */
int SystemProperties::i_loadVDPlugin(const char *pszPluginLibrary)
{
    return VDPluginLoadFromFilename(pszPluginLibrary);
}

/**
 * VD plugin unload
 */
int SystemProperties::i_unloadVDPlugin(const char *pszPluginLibrary)
{
    return VDPluginUnloadFromFilename(pszPluginLibrary);
}

/**
 * Internally usable version of getDefaultAdditionsISO.
 */
HRESULT SystemProperties::i_getDefaultAdditionsISO(com::Utf8Str &aDefaultAdditionsISO)
{
    AutoReadLock alock(this COMMA_LOCKVAL_SRC_POS);
    if (m->strDefaultAdditionsISO.isNotEmpty())
        aDefaultAdditionsISO = m->strDefaultAdditionsISO;
    else
    {
        /* no guest additions, check if it showed up in the mean time */
        alock.release();
        AutoWriteLock wlock(this COMMA_LOCKVAL_SRC_POS);
        if (m->strDefaultAdditionsISO.isEmpty())
        {
            ErrorInfoKeeper eik;
            (void)i_setDefaultAdditionsISO("");
        }
        aDefaultAdditionsISO = m->strDefaultAdditionsISO;
    }
    return S_OK;
}

// private methods
/////////////////////////////////////////////////////////////////////////////

/**
 * Returns the user's home directory. Wrapper around RTPathUserHome().
 * @param strPath
 * @return
 */
HRESULT SystemProperties::i_getUserHomeDirectory(Utf8Str &strPath)
{
    char szHome[RTPATH_MAX];
    int vrc = RTPathUserHome(szHome, sizeof(szHome));
    if (RT_FAILURE(vrc))
        return setErrorBoth(E_FAIL, vrc,
                            tr("Cannot determine user home directory (%Rrc)"),
                            vrc);
    strPath = szHome;
    return S_OK;
}

/**
 * Internal implementation to set the default machine folder. Gets called
 * from the public attribute setter as well as loadSettings(). With 4.0,
 * the "default default" machine folder has changed, and we now require
 * a full path always.
 * @param   strPath
 * @return
 */
HRESULT SystemProperties::i_setDefaultMachineFolder(const Utf8Str &strPath)
{
    Utf8Str path(strPath);      // make modifiable
    if (    path.isEmpty()          // used by API calls to reset the default
         || path == "Machines"      // this value (exactly like this, without path) is stored
                                    // in VirtualPox.xml if user upgrades from before 4.0 and
                                    // has not changed the default machine folder
       )
    {
        // new default with VirtualPox 4.0: "$HOME/VirtualPox VMs"
        HRESULT rc = i_getUserHomeDirectory(path);
        if (FAILED(rc)) return rc;
        path += RTPATH_SLASH_STR "VirtualPox VMs";
    }

    if (!RTPathStartsWithRoot(path.c_str()))
        return setError(E_INVALIDARG,
                        tr("Given default machine folder '%s' is not fully qualified"),
                        path.c_str());

    m->strDefaultMachineFolder = path;

    return S_OK;
}

HRESULT SystemProperties::i_setLoggingLevel(const com::Utf8Str &aLoggingLevel)
{
    Utf8Str useLoggingLevel(aLoggingLevel);
    if (useLoggingLevel.isEmpty())
        useLoggingLevel = VPOXSVC_LOG_DEFAULT;
    int rc = RTLogGroupSettings(RTLogRelGetDefaultInstance(), useLoggingLevel.c_str());
    //  If failed and not the default logging level - try to use the default logging level.
    if (RT_FAILURE(rc))
    {
        // If failed write message to the release log.
        LogRel(("Cannot set passed logging level=%s Error=%Rrc \n", useLoggingLevel.c_str(), rc));
        //  If attempted logging level not the default one then try the default one.
        if (!useLoggingLevel.equals(VPOXSVC_LOG_DEFAULT))
        {
            rc = RTLogGroupSettings(RTLogRelGetDefaultInstance(), VPOXSVC_LOG_DEFAULT);
            // If failed report this to the release log.
            if (RT_FAILURE(rc))
                LogRel(("Cannot set default logging level Error=%Rrc \n", rc));
        }
        // On any failure - set default level as the one to be stored.
        useLoggingLevel = VPOXSVC_LOG_DEFAULT;
    }
    //  Set to passed value or if default used/attempted (even if error condition) use empty string.
    m->strLoggingLevel = (useLoggingLevel.equals(VPOXSVC_LOG_DEFAULT) ? "" : useLoggingLevel);
    return RT_SUCCESS(rc) ? S_OK : E_FAIL;
}

HRESULT SystemProperties::i_setDefaultHardDiskFormat(const com::Utf8Str &aFormat)
{
    if (!aFormat.isEmpty())
        m->strDefaultHardDiskFormat = aFormat;
    else
        m->strDefaultHardDiskFormat = "VDI";

    return S_OK;
}

HRESULT SystemProperties::i_setVRDEAuthLibrary(const com::Utf8Str &aPath)
{
    if (!aPath.isEmpty())
        m->strVRDEAuthLibrary = aPath;
    else
        m->strVRDEAuthLibrary = "VPoxAuth";

    return S_OK;
}

HRESULT SystemProperties::i_setWebServiceAuthLibrary(const com::Utf8Str &aPath)
{
    if (!aPath.isEmpty())
        m->strWebServiceAuthLibrary = aPath;
    else
        m->strWebServiceAuthLibrary = "VPoxAuth";

    return S_OK;
}

HRESULT SystemProperties::i_setDefaultVRDEExtPack(const com::Utf8Str &aExtPack)
{
    m->strDefaultVRDEExtPack = aExtPack;

    return S_OK;
}

HRESULT SystemProperties::i_setAutostartDatabasePath(const com::Utf8Str &aPath)
{
    HRESULT rc = S_OK;
    AutostartDb *autostartDb = this->mParent->i_getAutostartDb();

    if (!aPath.isEmpty())
    {
        /* Update path in the autostart database. */
        int vrc = autostartDb->setAutostartDbPath(aPath.c_str());
        if (RT_SUCCESS(vrc))
            m->strAutostartDatabasePath = aPath;
        else
            rc = setErrorBoth(E_FAIL, vrc,
                              tr("Cannot set the autostart database path (%Rrc)"),
                              vrc);
    }
    else
    {
        int vrc = autostartDb->setAutostartDbPath(NULL);
        if (RT_SUCCESS(vrc) || vrc == VERR_NOT_SUPPORTED)
            m->strAutostartDatabasePath = "";
        else
            rc = setErrorBoth(E_FAIL, vrc,
                              tr("Deleting the autostart database path failed (%Rrc)"),
                              vrc);
    }

    return rc;
}

HRESULT SystemProperties::i_setDefaultAdditionsISO(const com::Utf8Str &aPath)
{
    com::Utf8Str path(aPath);
    if (path.isEmpty())
    {
        char strTemp[RTPATH_MAX];
        int vrc = RTPathAppPrivateNoArch(strTemp, sizeof(strTemp));
        AssertRC(vrc);
        Utf8Str strSrc1 = Utf8Str(strTemp).append("/VPoxGuestAdditions.iso");

        vrc = RTPathExecDir(strTemp, sizeof(strTemp));
        AssertRC(vrc);
        Utf8Str strSrc2 = Utf8Str(strTemp).append("/additions/VPoxGuestAdditions.iso");

        vrc = RTPathUserHome(strTemp, sizeof(strTemp));
        AssertRC(vrc);
        Utf8Str strSrc3 = Utf8StrFmt("%s/VPoxGuestAdditions_%s.iso", strTemp, VirtualPox::i_getVersionNormalized().c_str());

        /* Check the standard image locations */
        if (RTFileExists(strSrc1.c_str()))
            path = strSrc1;
        else if (RTFileExists(strSrc2.c_str()))
            path = strSrc2;
        else if (RTFileExists(strSrc3.c_str()))
            path = strSrc3;
        else
            return setError(E_FAIL,
                            tr("Cannot determine default Guest Additions ISO location. Most likely they are not available"));
    }

    if (!RTPathStartsWithRoot(path.c_str()))
        return setError(E_INVALIDARG,
                        tr("Given default machine Guest Additions ISO file '%s' is not fully qualified"),
                        path.c_str());

    if (!RTFileExists(path.c_str()))
        return setError(E_INVALIDARG,
                        tr("Given default machine Guest Additions ISO file '%s' does not exist"),
                        path.c_str());

    m->strDefaultAdditionsISO = path;

    return S_OK;
}

HRESULT SystemProperties::i_setDefaultFrontend(const com::Utf8Str &aDefaultFrontend)
{
    m->strDefaultFrontend = aDefaultFrontend;

    return S_OK;
}

