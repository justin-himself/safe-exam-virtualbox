/**@file
  Platform PEI driver

  Copyright (c) 2006 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2011, Andrei Warkentin <andreiw@motorola.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

//
// The package level header files this module uses
//
#include <PiPei.h>

//
// The Library classes this module consumes
//
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/IoLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/PciLib.h>
#include <Library/PeimEntryPoint.h>
#include <Library/PeiServicesLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Library/QemuFwCfgS3Lib.h>
#include <Library/ResourcePublicationLib.h>
#include <Guid/MemoryTypeInformation.h>
#include <Ppi/MasterBootMode.h>
#include <IndustryStandard/Pci22.h>
#include <OvmfPlatforms.h>

#include "Platform.h"
#include "Cmos.h"

#ifdef VPOX
# include "VPoxPkg.h"
# include "DevEFI.h"
# include "iprt/asm.h"
#endif

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIMemoryNVS,       0x004 },
  { EfiACPIReclaimMemory,   0x008 },
  { EfiReservedMemoryType,  0x004 },
  { EfiRuntimeServicesData, 0x024 },
  { EfiRuntimeServicesCode, 0x030 },
  { EfiBootServicesCode,    0x180 },
  { EfiBootServicesData,    0xF00 },
  { EfiMaxMemoryType,       0x000 }
};


EFI_PEI_PPI_DESCRIPTOR   mPpiBootMode[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
    &gEfiPeiMasterBootModePpiGuid,
    NULL
  }
};


UINT16 mHostBridgeDevId;

EFI_BOOT_MODE mBootMode = BOOT_WITH_FULL_CONFIGURATION;

BOOLEAN mS3Supported = FALSE;

UINT32 mMaxCpuCount;
#ifdef VPOX
static UINT32
GetVmVariable(UINT32 Variable, CHAR8 *pbBuf, UINT32 cbBuf)
{
  UINT32 cbVar, offBuf;

  ASMOutU32(EFI_INFO_PORT, Variable);
  cbVar = ASMInU32(EFI_INFO_PORT);

  for (offBuf = 0; offBuf < cbVar && offBuf < cbBuf; offBuf++)
    pbBuf[offBuf] = ASMInU8(EFI_INFO_PORT);

  return cbVar;
}
#endif


VOID
AddIoMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_MAPPED_IO,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

VOID
AddReservedMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize,
  BOOLEAN                     Cacheable
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_MEMORY_RESERVED,
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      (Cacheable ?
       EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
       EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE :
       0
       ) |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}

#ifdef VPOX
VOID
AddRomMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  STATIC EFI_RESOURCE_ATTRIBUTE_TYPE Attributes =
    (
      EFI_RESOURCE_ATTRIBUTE_PRESENT     |
      EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTED |
      EFI_RESOURCE_ATTRIBUTE_WRITE_PROTECTABLE |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE
    );

  BuildResourceDescriptorHob (
    EFI_RESOURCE_FIRMWARE_DEVICE,
    Attributes,
    MemoryBase,
    MemorySize
    );

  DEBUG ((DEBUG_INFO, "ROM HOB: at 0x%llx size 0x%llx\n", MemoryBase, MemorySize));
}

static VOID *
FindAcpiRsdPtr (
  VOID
  )
{
#define ACPI_RSD_PTR      SIGNATURE_64('R', 'S', 'D', ' ', 'P', 'T', 'R', ' ')
  UINTN                           Address;

  //
  // First Search 0x0e0000 - 0x0fffff for RSD Ptr
  //
  for (Address = 0xe0000; Address < 0xfffff; Address += 0x10) {
    if (*(UINT64 *)(Address) == ACPI_RSD_PTR) {
      return (VOID *)Address;
    }
  }
  return NULL;
}
#undef ACPI_RSD_PTR
#endif

VOID
AddIoMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddIoMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
AddMemoryBaseSizeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  UINT64                      MemorySize
  )
{
  BuildResourceDescriptorHob (
    EFI_RESOURCE_SYSTEM_MEMORY,
      EFI_RESOURCE_ATTRIBUTE_PRESENT |
      EFI_RESOURCE_ATTRIBUTE_INITIALIZED |
      EFI_RESOURCE_ATTRIBUTE_UNCACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_COMBINEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_THROUGH_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_WRITE_BACK_CACHEABLE |
      EFI_RESOURCE_ATTRIBUTE_TESTED,
    MemoryBase,
    MemorySize
    );
}


VOID
AddMemoryRangeHob (
  EFI_PHYSICAL_ADDRESS        MemoryBase,
  EFI_PHYSICAL_ADDRESS        MemoryLimit
  )
{
  AddMemoryBaseSizeHob (MemoryBase, (UINT64)(MemoryLimit - MemoryBase));
}


VOID
MemMapInitialization (
  VOID
  )
{
  UINT64        PciIoBase;
  UINT64        PciIoSize;
  RETURN_STATUS PcdStatus;

#ifdef VPOX
  EFI_PHYSICAL_ADDRESS RsdPtr;
  EFI_PHYSICAL_ADDRESS AcpiTables;
  UINT64 McfgBase = 0;
  UINT64 McfgSize = 0;
#endif
  PciIoBase = 0xC000;
  PciIoSize = 0x4000;

  //
  // Create Memory Type Information HOB
  //
  BuildGuidDataHob (
    &gEfiMemoryTypeInformationGuid,
    mDefaultMemoryTypeInformation,
    sizeof(mDefaultMemoryTypeInformation)
    );

  //
  // Video memory + Legacy BIOS region
#ifdef VPOX
  // This includes ACPI floating pointer region.
#endif
  //
  AddIoMemoryRangeHob (0x0A0000, BASE_1MB);

  if (!mXen) {
    UINT32  TopOfLowRam;
#ifndef VPOX
    UINT64  PciExBarBase;
#endif
    UINT32  PciBase;
    UINT32  PciSize;

    TopOfLowRam = GetSystemMemorySizeBelow4gb ();
#ifndef VPOX
    PciExBarBase = 0;

    if (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
      //
      // The MMCONFIG area is expected to fall between the top of low RAM and
      // the base of the 32-bit PCI host aperture.
      //
      PciExBarBase = PcdGet64 (PcdPciExpressBaseAddress);
      ASSERT (TopOfLowRam <= PciExBarBase);
      ASSERT (PciExBarBase <= MAX_UINT32 - SIZE_256MB);
      PciBase = (UINT32)(PciExBarBase + SIZE_256MB);
    } else {
      ASSERT (TopOfLowRam <= mQemuUc32Base);
      PciBase = mQemuUc32Base;
    }

    //
    // address       purpose   size
    // ------------  --------  -------------------------
    // max(top, 2g)  PCI MMIO  0xFC000000 - max(top, 2g)
    // 0xFC000000    gap                           44 MB
    // 0xFEC00000    IO-APIC                        4 KB
    // 0xFEC01000    gap                         1020 KB
    // 0xFED00000    HPET                           1 KB
    // 0xFED00400    gap                          111 KB
    // 0xFED1C000    gap (PIIX4) / RCRB (ICH9)     16 KB
    // 0xFED20000    gap                          896 KB
    // 0xFEE00000    LAPIC                          1 MB
    //
    PciSize = 0xFC000000 - PciBase;
    AddIoMemoryBaseSizeHob (PciBase, PciSize);
    PcdStatus = PcdSet64S (PcdPciMmio32Base, PciBase);
    ASSERT_RETURN_ERROR (PcdStatus);
    PcdStatus = PcdSet64S (PcdPciMmio32Size, PciSize);
    ASSERT_RETURN_ERROR (PcdStatus);

    AddIoMemoryRangeHob (TopOfLowRam < BASE_2GB ?
                         BASE_2GB : TopOfLowRam, 0xFC000000);
#else
    GetVmVariable(EFI_INFO_INDEX_MCFG_BASE, (CHAR8 *)&McfgBase, sizeof(McfgBase));
    GetVmVariable(EFI_INFO_INDEX_MCFG_SIZE, (CHAR8 *)&McfgSize, sizeof(McfgSize));
    if (TopOfLowRam < BASE_2GB)
      TopOfLowRam = BASE_2GB;
    if (McfgBase == 0)
      McfgBase = TopOfLowRam;   // backward compatibilit with old DevEFI
    if (TopOfLowRam < McfgBase)
      AddIoMemoryRangeHob (TopOfLowRam, McfgBase);
    AddIoMemoryRangeHob (McfgBase + McfgSize, 0xFC000000);

    PcdSet64S (PcdPciExpressBaseAddress, McfgBase);

    ASSERT (McfgBase == (UINT32)McfgBase);
    ASSERT (McfgBase + McfgSize < 0xFC000000);

    PciBase = (UINT32)(McfgBase + McfgSize);
    PciSize = 0xFC000000 - PciBase;
    PcdStatus = PcdSet64S (PcdPciMmio32Base, PciBase);
    ASSERT_RETURN_ERROR (PcdStatus);
    PcdStatus = PcdSet64S (PcdPciMmio32Size, PciSize);
    ASSERT_RETURN_ERROR (PcdStatus);
#endif

    AddIoMemoryBaseSizeHob (0xFEC00000, SIZE_4KB);
    AddIoMemoryBaseSizeHob (0xFED00000, SIZE_1KB);
#ifndef VPOX
    if (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
      AddIoMemoryBaseSizeHob (ICH9_ROOT_COMPLEX_BASE, SIZE_16KB);
      //
      // Note: there should be an
      //
      //   AddIoMemoryBaseSizeHob (PciExBarBase, SIZE_256MB);
      //
      // call below, just like the one above for RCBA. However, Linux insists
      // that the MMCONFIG area be marked in the E820 or UEFI memory map as
      // "reserved memory" -- Linux does not content itself with a simple gap
      // in the memory map wherever the MCFG ACPI table points to.
      //
      // This appears to be a safety measure. The PCI Firmware Specification
      // (rev 3.1) says in 4.1.2. "MCFG Table Description": "The resources can
      // *optionally* be returned in [...] EFIGetMemoryMap as reserved memory
      // [...]". (Emphasis added here.)
      //
      // Normally we add memory resource descriptor HOBs in
      // QemuInitializeRam(), and pre-allocate from those with memory
      // allocation HOBs in InitializeRamRegions(). However, the MMCONFIG area
      // is most definitely not RAM; so, as an exception, cover it with
      // uncacheable reserved memory right here.
      //
      AddReservedMemoryBaseSizeHob (PciExBarBase, SIZE_256MB, FALSE);
      BuildMemoryAllocationHob (PciExBarBase, SIZE_256MB,
        EfiReservedMemoryType);
    }
#endif
    AddIoMemoryBaseSizeHob (PcdGet32(PcdCpuLocalApicBaseAddress), SIZE_1MB);

    //
    // On Q35, the IO Port space is available for PCI resource allocations from
    // 0x6000 up.
    //
    if (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
      PciIoBase = 0x6000;
      PciIoSize = 0xA000;
      ASSERT ((ICH9_PMBASE_VALUE & 0xF000) < PciIoBase);
    }
  }

  //
  // Add PCI IO Port space available for PCI resource allocations.
  //
  BuildResourceDescriptorHob (
    EFI_RESOURCE_IO,
    EFI_RESOURCE_ATTRIBUTE_PRESENT     |
    EFI_RESOURCE_ATTRIBUTE_INITIALIZED,
    PciIoBase,
    PciIoSize
    );
  PcdStatus = PcdSet64S (PcdPciIoBase, PciIoBase);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet64S (PcdPciIoSize, PciIoSize);
  ASSERT_RETURN_ERROR (PcdStatus);

#ifdef VPOX
  //
  // Add ACPI memory, provided by VPox
  //
  RsdPtr = (EFI_PHYSICAL_ADDRESS)(UINTN)FindAcpiRsdPtr();
  ASSERT(RsdPtr != 0);
  AcpiTables = (EFI_PHYSICAL_ADDRESS)*(UINT32*)((UINTN)RsdPtr + 16) & ~0xfff;
  ASSERT(AcpiTables != 0);

  // ACPI tables 64 K
  AddRomMemoryBaseSizeHob(AcpiTables, 0x10000);
#endif
}

EFI_STATUS
GetNamedFwCfgBoolean (
  IN  CHAR8   *FwCfgFileName,
  OUT BOOLEAN *Setting
  )
{
  EFI_STATUS           Status;
  FIRMWARE_CONFIG_ITEM FwCfgItem;
  UINTN                FwCfgSize;
  UINT8                Value[3];

  Status = QemuFwCfgFindFile (FwCfgFileName, &FwCfgItem, &FwCfgSize);
  if (EFI_ERROR (Status)) {
    return Status;
  }
  if (FwCfgSize > sizeof Value) {
    return EFI_BAD_BUFFER_SIZE;
  }
  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (FwCfgSize, Value);

  if ((FwCfgSize == 1) ||
      (FwCfgSize == 2 && Value[1] == '\n') ||
      (FwCfgSize == 3 && Value[1] == '\r' && Value[2] == '\n')) {
    switch (Value[0]) {
      case '0':
      case 'n':
      case 'N':
        *Setting = FALSE;
        return EFI_SUCCESS;

      case '1':
      case 'y':
      case 'Y':
        *Setting = TRUE;
        return EFI_SUCCESS;

      default:
        break;
    }
  }
  return EFI_PROTOCOL_ERROR;
}

#define UPDATE_BOOLEAN_PCD_FROM_FW_CFG(TokenName)                   \
          do {                                                      \
            BOOLEAN       Setting;                                  \
            RETURN_STATUS PcdStatus;                                \
                                                                    \
            if (!EFI_ERROR (GetNamedFwCfgBoolean (                  \
                              "opt/ovmf/" #TokenName, &Setting))) { \
              PcdStatus = PcdSetBoolS (TokenName, Setting);         \
              ASSERT_RETURN_ERROR (PcdStatus);                      \
            }                                                       \
          } while (0)

VOID
NoexecDxeInitialization (
  VOID
  )
{
  UPDATE_BOOLEAN_PCD_FROM_FW_CFG (PcdPropertiesTableEnable);
  UPDATE_BOOLEAN_PCD_FROM_FW_CFG (PcdSetNxForStack);
}

VOID
PciExBarInitialization (
  VOID
  )
{
  union {
    UINT64 Uint64;
    UINT32 Uint32[2];
  } PciExBarBase;

  //
  // We only support the 256MB size for the MMCONFIG area:
  // 256 buses * 32 devices * 8 functions * 4096 bytes config space.
  //
  // The masks used below enforce the Q35 requirements that the MMCONFIG area
  // be (a) correctly aligned -- here at 256 MB --, (b) located under 64 GB.
  //
  // Note that (b) also ensures that the minimum address width we have
  // determined in AddressWidthInitialization(), i.e., 36 bits, will suffice
  // for DXE's page tables to cover the MMCONFIG area.
  //
  PciExBarBase.Uint64 = PcdGet64 (PcdPciExpressBaseAddress);
  ASSERT ((PciExBarBase.Uint32[1] & MCH_PCIEXBAR_HIGHMASK) == 0);
  ASSERT ((PciExBarBase.Uint32[0] & MCH_PCIEXBAR_LOWMASK) == 0);

  //
  // Clear the PCIEXBAREN bit first, before programming the high register.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW), 0);

  //
  // Program the high register. Then program the low register, setting the
  // MMCONFIG area size and enabling decoding at once.
  //
  PciWrite32 (DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_HIGH), PciExBarBase.Uint32[1]);
  PciWrite32 (
    DRAMC_REGISTER_Q35 (MCH_PCIEXBAR_LOW),
    PciExBarBase.Uint32[0] | MCH_PCIEXBAR_BUS_FF | MCH_PCIEXBAR_EN
    );
}

VOID
MiscInitialization (
  VOID
  )
{
  UINTN         PmCmd;
  UINTN         Pmba;
  UINT32        PmbaAndVal;
  UINT32        PmbaOrVal;
  UINTN         AcpiCtlReg;
  UINT8         AcpiEnBit;
  RETURN_STATUS PcdStatus;

  //
  // Disable A20 Mask
  //
  IoOr8 (0x92, BIT1);

  //
  // Build the CPU HOB with guest RAM size dependent address width and 16-bits
  // of IO space. (Side note: unlike other HOBs, the CPU HOB is needed during
  // S3 resume as well, so we build it unconditionally.)
  //
  BuildCpuHob (mPhysMemAddressWidth, 16);

  //
  // Determine platform type and save Host Bridge DID to PCD
  //
  switch (mHostBridgeDevId) {
#ifdef VPOX
    // This is really hacky but so it goes. The PCIe chipset might have nothing at 0:0.0,
    // or it might be some random device. But it's not going to be the 440FX host bridge.
    default:
#endif
    case INTEL_82441_DEVICE_ID:
      PmCmd      = POWER_MGMT_REGISTER_PIIX4 (PCI_COMMAND_OFFSET);
      Pmba       = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMBA);
      PmbaAndVal = ~(UINT32)PIIX4_PMBA_MASK;
      PmbaOrVal  = PIIX4_PMBA_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_PIIX4 (PIIX4_PMREGMISC);
      AcpiEnBit  = PIIX4_PMREGMISC_PMIOSE;
      break;
#ifndef VPOX
    case INTEL_Q35_MCH_DEVICE_ID:
      PmCmd      = POWER_MGMT_REGISTER_Q35 (PCI_COMMAND_OFFSET);
      Pmba       = POWER_MGMT_REGISTER_Q35 (ICH9_PMBASE);
      PmbaAndVal = ~(UINT32)ICH9_PMBASE_MASK;
      PmbaOrVal  = ICH9_PMBASE_VALUE;
      AcpiCtlReg = POWER_MGMT_REGISTER_Q35 (ICH9_ACPI_CNTL);
      AcpiEnBit  = ICH9_ACPI_CNTL_ACPI_EN;
      break;
    default:
      DEBUG ((EFI_D_ERROR, "%a: Unknown Host Bridge Device ID: 0x%04x\n",
        __FUNCTION__, mHostBridgeDevId));
      ASSERT (FALSE);
      return;
#endif
  }

#ifdef VPOX
  // If it's not 440FX, it must be the PCIe chipset.
  if (mHostBridgeDevId != INTEL_82441_DEVICE_ID)
      mHostBridgeDevId = INTEL_Q35_MCH_DEVICE_ID;

#endif
  PcdStatus = PcdSet16S (PcdOvmfHostBridgePciDevId, mHostBridgeDevId);
  ASSERT_RETURN_ERROR (PcdStatus);

  //
  // If the appropriate IOspace enable bit is set, assume the ACPI PMBA
  // has been configured (e.g., by Xen) and skip the setup here.
  // This matches the logic in AcpiTimerLibConstructor ().
  //
  if ((PciRead8 (AcpiCtlReg) & AcpiEnBit) == 0) {
    //
    // The PEI phase should be exited with fully accessibe ACPI PM IO space:
    // 1. set PMBA
    //
    PciAndThenOr32 (Pmba, PmbaAndVal, PmbaOrVal);

    //
    // 2. set PCICMD/IOSE
    //
    PciOr8 (PmCmd, EFI_PCI_COMMAND_IO_SPACE);

    //
    // 3. set ACPI PM IO enable bit (PMREGMISC:PMIOSE or ACPI_CNTL:ACPI_EN)
    //
    PciOr8 (AcpiCtlReg, AcpiEnBit);
  }

#ifndef VPOX    // The RCBA is not really there, and MCFG is already in place
  if (mHostBridgeDevId == INTEL_Q35_MCH_DEVICE_ID) {
    //
    // Set Root Complex Register Block BAR
    //
    PciWrite32 (
      POWER_MGMT_REGISTER_Q35 (ICH9_RCBA),
      ICH9_ROOT_COMPLEX_BASE | ICH9_RCBA_EN
      );

    //
    // Set PCI Express Register Range Base Address
    //
    PciExBarInitialization ();
  }
#endif
}


VOID
BootModeInitialization (
  VOID
  )
{
  EFI_STATUS    Status;

  if (CmosRead8 (0xF) == 0xFE) {
    mBootMode = BOOT_ON_S3_RESUME;
  }
  CmosWrite8 (0xF, 0x00);

  Status = PeiServicesSetBootMode (mBootMode);
  ASSERT_EFI_ERROR (Status);

  Status = PeiServicesInstallPpi (mPpiBootMode);
  ASSERT_EFI_ERROR (Status);
}


VOID
ReserveEmuVariableNvStore (
  )
{
  EFI_PHYSICAL_ADDRESS VariableStore;
  RETURN_STATUS        PcdStatus;

  //
  // Allocate storage for NV variables early on so it will be
  // at a consistent address.  Since VM memory is preserved
  // across reboots, this allows the NV variable storage to survive
  // a VM reboot.
  //
  VariableStore =
    (EFI_PHYSICAL_ADDRESS)(UINTN)
      AllocateRuntimePages (
        EFI_SIZE_TO_PAGES (2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize))
        );
  DEBUG ((EFI_D_INFO,
          "Reserved variable store memory: 0x%lX; size: %dkb\n",
          VariableStore,
          (2 * PcdGet32 (PcdFlashNvStorageFtwSpareSize)) / 1024
        ));
  PcdStatus = PcdSet64S (PcdEmuVariableNvStoreReserved, VariableStore);
  ASSERT_RETURN_ERROR (PcdStatus);
}


VOID
DebugDumpCmos (
  VOID
  )
{
  UINT32 Loop;

  DEBUG ((EFI_D_INFO, "CMOS:\n"));

  for (Loop = 0; Loop < 0x80; Loop++) {
    if ((Loop % 0x10) == 0) {
      DEBUG ((EFI_D_INFO, "%02x:", Loop));
    }
    DEBUG ((EFI_D_INFO, " %02x", CmosRead8 (Loop)));
    if ((Loop % 0x10) == 0xf) {
      DEBUG ((EFI_D_INFO, "\n"));
    }
  }
}


VOID
S3Verification (
  VOID
  )
{
#if defined (MDE_CPU_X64)
  if (FeaturePcdGet (PcdSmmSmramRequire) && mS3Supported) {
    DEBUG ((EFI_D_ERROR,
      "%a: S3Resume2Pei doesn't support X64 PEI + SMM yet.\n", __FUNCTION__));
    DEBUG ((EFI_D_ERROR,
      "%a: Please disable S3 on the QEMU command line (see the README),\n",
      __FUNCTION__));
    DEBUG ((EFI_D_ERROR,
      "%a: or build OVMF with \"OvmfPkgIa32X64.dsc\".\n", __FUNCTION__));
    ASSERT (FALSE);
    CpuDeadLoop ();
  }
#endif
}


/**
  Fetch the number of boot CPUs from QEMU and expose it to UefiCpuPkg modules.
  Set the mMaxCpuCount variable.
**/
VOID
MaxCpuCountInitialization (
  VOID
  )
{
  UINT16        ProcessorCount;
  RETURN_STATUS PcdStatus;

  QemuFwCfgSelectItem (QemuFwCfgItemSmpCpuCount);
  ProcessorCount = QemuFwCfgRead16 ();
  //
  // If the fw_cfg key or fw_cfg entirely is unavailable, load mMaxCpuCount
  // from the PCD default. No change to PCDs.
  //
  if (ProcessorCount == 0) {
    mMaxCpuCount = PcdGet32 (PcdCpuMaxLogicalProcessorNumber);
    return;
  }
  //
  // Otherwise, set mMaxCpuCount to the value reported by QEMU.
  //
  mMaxCpuCount = ProcessorCount;
  //
  // Additionally, tell UefiCpuPkg modules (a) the exact number of VCPUs, (b)
  // to wait, in the initial AP bringup, exactly as long as it takes for all of
  // the APs to report in. For this, we set the longest representable timeout
  // (approx. 71 minutes).
  //
  PcdStatus = PcdSet32S (PcdCpuMaxLogicalProcessorNumber, ProcessorCount);
  ASSERT_RETURN_ERROR (PcdStatus);
  PcdStatus = PcdSet32S (PcdCpuApInitTimeOutInMicroSeconds, MAX_UINT32);
  ASSERT_RETURN_ERROR (PcdStatus);
  DEBUG ((DEBUG_INFO, "%a: QEMU reports %d processor(s)\n", __FUNCTION__,
    ProcessorCount));
}


/**
  Perform Platform PEI initialization.

  @param  FileHandle      Handle of the file being invoked.
  @param  PeiServices     Describes the list of possible PEI Services.

  @return EFI_SUCCESS     The PEIM initialized successfully.

**/
EFI_STATUS
EFIAPI
InitializePlatform (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS    Status;
#ifdef VPOX
  EFI_PHYSICAL_ADDRESS Memory;
#endif

  DEBUG ((DEBUG_INFO, "Platform PEIM Loaded\n"));

  DebugDumpCmos ();

  XenDetect ();

  if (QemuFwCfgS3Enabled ()) {
    DEBUG ((EFI_D_INFO, "S3 support was detected on QEMU\n"));
    mS3Supported = TRUE;
    Status = PcdSetBoolS (PcdAcpiS3Enable, TRUE);
    ASSERT_EFI_ERROR (Status);
  }

  S3Verification ();
  BootModeInitialization ();
  AddressWidthInitialization ();
  MaxCpuCountInitialization ();

  //
  // Query Host Bridge DID
  //
  mHostBridgeDevId = PciRead16 (OVMF_HOSTBRIDGE_DID);
#ifdef VPOX
  // HACK ALERT! There is no host bridge device in the PCIe chipset, but we pretend it's a 3 Series chip.
  // There may or not be some device at 0:0.0 so anything not 440FX must be PCIe.
  if (mHostBridgeDevId != INTEL_82441_DEVICE_ID)
        mHostBridgeDevId = INTEL_Q35_MCH_DEVICE_ID;
#endif

  if (FeaturePcdGet (PcdSmmSmramRequire)) {
    Q35TsegMbytesInitialization ();
  }

  PublishPeiMemory ();

  QemuUc32BaseInitialization ();

  InitializeRamRegions ();

  if (mXen) {
    DEBUG ((EFI_D_INFO, "Xen was detected\n"));
    InitializeXen ();
  }

#ifdef VPOX
  /*
   * This seemingly useless allocation is required to protect the memory against
   * a bug present in Apples boot.efi bootloader for OS X Tiger, Leopard and Snow Leopard
   * causing a triple fault before the kernel is started because the stack got trashed.
   *
   * Before handing control to the kernel it goes over the memory map acquired with gRT->GetMemoryMap()
   * and relocates all EfiRuntimeServicesData and EfiRuntimeServicesCode to another memory location.
   * Every entry not having the EfiRuntimeServicesData/EfiRuntimeServicesCode type gets removed and the
   * memory location is zeroed. However the size of the region is not taken from the memory descriptor
   * but calculated before by just using the last EfiRuntimeServices* regions size (which is the bug).
   *
   * In our case this is the variable store memory allocated in ReserveEmuVariableNvStore() which spans
   * 0x84 pages or 528KB which causes the stack to get trashed when boot.efi comes to the zero out the
   * EfiBootServicesData range covering the stack.
   * To prevent merging adjacent memory regions with the same properties in CoreGetMemoryMap() a
   * EfiRuntimeServicesCode region with exactly one page gets allocated as the first region here so it
   * ends up last in the memory map. This prevents boot.efi from zeroing too much memory.
   *
   * This worked with 6.0 and earlier firmware because the variable store was much smaller (only 128KB)
   * which happened to work by accident.
   */
  PeiServicesAllocatePages (EfiRuntimeServicesCode, 1, &Memory);
#endif

  if (mBootMode != BOOT_ON_S3_RESUME) {
    if (!FeaturePcdGet (PcdSmmSmramRequire)) {
      ReserveEmuVariableNvStore ();
    }
    PeiFvInitialization ();
    MemMapInitialization ();
    NoexecDxeInitialization ();
  }

  InstallClearCacheCallback ();
  AmdSevInitialize ();
  MiscInitialization ();
  InstallFeatureControlCallback ();

  return EFI_SUCCESS;
}
