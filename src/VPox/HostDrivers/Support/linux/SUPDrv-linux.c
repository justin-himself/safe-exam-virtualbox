/* $Id: SUPDrv-linux.c $ */
/** @file
 * VPoxDrv - The VirtualPox Support Driver - Linux specifics.
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


/*********************************************************************************************************************************
*   Header Files                                                                                                                 *
*********************************************************************************************************************************/
#define LOG_GROUP LOG_GROUP_SUP_DRV
#include "../SUPDrvInternal.h"
#include "the-linux-kernel.h"
#include "version-generated.h"
#include "product-generated.h"
#include "revision-generated.h"

#include <iprt/assert.h>
#include <iprt/spinlock.h>
#include <iprt/semaphore.h>
#include <iprt/initterm.h>
#include <iprt/process.h>
#include <iprt/thread.h>
#include <VPox/err.h>
#include <iprt/mem.h>
#include <VPox/log.h>
#include <iprt/mp.h>

/** @todo figure out the exact version number */
#if RTLNX_VER_MIN(2,6,16)
# include <iprt/power.h>
# define VPOX_WITH_SUSPEND_NOTIFICATION
#endif

#include <linux/sched.h>
#include <linux/miscdevice.h>
#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
# include <linux/platform_device.h>
#endif
#if (RTLNX_VER_MIN(2,6,28)) && defined(SUPDRV_WITH_MSR_PROBER)
# define SUPDRV_LINUX_HAS_SAFE_MSR_API
# include <asm/msr.h>
#endif

#include <asm/desc.h>

#include <iprt/asm-amd64-x86.h>


/*********************************************************************************************************************************
*   Defined Constants And Macros                                                                                                 *
*********************************************************************************************************************************/
/* check kernel version */
# ifndef SUPDRV_AGNOSTIC
#  if RTLNX_VER_MAX(2,6,0)
#   error Unsupported kernel version!
#  endif
# endif

#ifdef CONFIG_X86_HIGH_ENTRY
# error "CONFIG_X86_HIGH_ENTRY is not supported by VPoxDrv at this time."
#endif

/* We cannot include x86.h, so we copy the defines we need here: */
#define X86_EFL_IF          RT_BIT(9)
#define X86_EFL_AC          RT_BIT(18)
#define X86_EFL_DF          RT_BIT(10)
#define X86_EFL_IOPL        (RT_BIT(12) | RT_BIT(13))

/* To include the version number of VirtualPox into kernel backtraces: */
#define VPoxDrvLinuxVersion RT_CONCAT3(RT_CONCAT(VPOX_VERSION_MAJOR, _), \
                                       RT_CONCAT(VPOX_VERSION_MINOR, _), \
                                       VPOX_VERSION_BUILD)
#define VPoxDrvLinuxIOCtl RT_CONCAT(VPoxDrvLinuxIOCtl_,VPoxDrvLinuxVersion)

/* Once externally provided, this string will be printed into kernel log on
 * module start together with the rest of versioning information. */
#ifndef VPOX_EXTRA_VERSION_STRING
# define VPOX_EXTRA_VERSION_STRING ""
#endif


/*********************************************************************************************************************************
*   Internal Functions                                                                                                           *
*********************************************************************************************************************************/
static int  __init VPoxDrvLinuxInit(void);
static void __exit VPoxDrvLinuxUnload(void);
static int  VPoxDrvLinuxCreateSys(struct inode *pInode, struct file *pFilp);
static int  VPoxDrvLinuxCreateUsr(struct inode *pInode, struct file *pFilp);
static int  VPoxDrvLinuxClose(struct inode *pInode, struct file *pFilp);
#ifdef HAVE_UNLOCKED_IOCTL
static long VPoxDrvLinuxIOCtl(struct file *pFilp, unsigned int uCmd, unsigned long ulArg);
#else
static int  VPoxDrvLinuxIOCtl(struct inode *pInode, struct file *pFilp, unsigned int uCmd, unsigned long ulArg);
#endif
static int  VPoxDrvLinuxIOCtlSlow(struct file *pFilp, unsigned int uCmd, unsigned long ulArg, PSUPDRVSESSION pSession);
static int  VPoxDrvLinuxErr2LinuxErr(int);
#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
static int  VPoxDrvProbe(struct platform_device *pDev);
# if RTLNX_VER_MIN(2,6,30)
static int  VPoxDrvSuspend(struct device *pDev);
static int  VPoxDrvResume(struct device *pDev);
# else
static int  VPoxDrvSuspend(struct platform_device *pDev, pm_message_t State);
static int  VPoxDrvResume(struct platform_device *pDev);
# endif
static void VPoxDevRelease(struct device *pDev);
#endif


/*********************************************************************************************************************************
*   Global Variables                                                                                                             *
*********************************************************************************************************************************/
/**
 * Device extention & session data association structure.
 */
static SUPDRVDEVEXT         g_DevExt;

/** Module parameter.
 * Not prefixed because the name is used by macros and the end of this file. */
static int force_async_tsc = 0;

/** The system device name. */
#define DEVICE_NAME_SYS     "vpoxdrv"
/** The user device name. */
#define DEVICE_NAME_USR     "vpoxdrvu"

#if (defined(RT_ARCH_AMD64) && RTLNX_VER_MAX(2,6,23)) || defined(VPOX_WITH_TEXT_MODMEM_HACK)
/**
 * Memory for the executable memory heap (in IPRT).
 */
# ifdef DEBUG
#  define EXEC_MEMORY_SIZE   10485760   /* 10 MB */
# else
#  define EXEC_MEMORY_SIZE   8388608    /* 8 MB */
# endif
extern uint8_t g_abExecMemory[EXEC_MEMORY_SIZE];
# ifndef VPOX_WITH_TEXT_MODMEM_HACK
__asm__(".section execmemory, \"awx\", @progbits\n\t"
        ".align 32\n\t"
        ".globl g_abExecMemory\n"
        "g_abExecMemory:\n\t"
        ".zero " RT_XSTR(EXEC_MEMORY_SIZE) "\n\t"
        ".type g_abExecMemory, @object\n\t"
        ".size g_abExecMemory, " RT_XSTR(EXEC_MEMORY_SIZE) "\n\t"
        ".text\n\t");
# else
__asm__(".text\n\t"
        ".align 4096\n\t"
        ".globl g_abExecMemory\n"
        "g_abExecMemory:\n\t"
        ".zero " RT_XSTR(EXEC_MEMORY_SIZE) "\n\t"
        ".type g_abExecMemory, @object\n\t"
        ".size g_abExecMemory, " RT_XSTR(EXEC_MEMORY_SIZE) "\n\t"
        ".text\n\t");
# endif
#endif

/** The file_operations structure. */
static struct file_operations gFileOpsVPoxDrvSys =
{
    owner:      THIS_MODULE,
    open:       VPoxDrvLinuxCreateSys,
    release:    VPoxDrvLinuxClose,
#ifdef HAVE_UNLOCKED_IOCTL
    unlocked_ioctl: VPoxDrvLinuxIOCtl,
#else
    ioctl:      VPoxDrvLinuxIOCtl,
#endif
};

/** The file_operations structure. */
static struct file_operations gFileOpsVPoxDrvUsr =
{
    owner:      THIS_MODULE,
    open:       VPoxDrvLinuxCreateUsr,
    release:    VPoxDrvLinuxClose,
#ifdef HAVE_UNLOCKED_IOCTL
    unlocked_ioctl: VPoxDrvLinuxIOCtl,
#else
    ioctl:      VPoxDrvLinuxIOCtl,
#endif
};

/** The miscdevice structure for vpoxdrv. */
static struct miscdevice gMiscDeviceSys =
{
    minor:      MISC_DYNAMIC_MINOR,
    name:       DEVICE_NAME_SYS,
    fops:       &gFileOpsVPoxDrvSys,
# if RTLNX_VER_MAX(2,6,18)
    devfs_name: DEVICE_NAME_SYS,
# endif
};
/** The miscdevice structure for vpoxdrvu. */
static struct miscdevice gMiscDeviceUsr =
{
    minor:      MISC_DYNAMIC_MINOR,
    name:       DEVICE_NAME_USR,
    fops:       &gFileOpsVPoxDrvUsr,
# if RTLNX_VER_MAX(2,6,18)
    devfs_name: DEVICE_NAME_USR,
# endif
};


#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
# if RTLNX_VER_MIN(2,6,30)
static struct dev_pm_ops gPlatformPMOps =
{
    .suspend = VPoxDrvSuspend,  /* before entering deep sleep */
    .resume  = VPoxDrvResume,   /* after wakeup from deep sleep */
    .freeze  = VPoxDrvSuspend,  /* before creating hibernation image */
    .restore = VPoxDrvResume,   /* after waking up from hibernation */
};
# endif

static struct platform_driver gPlatformDriver =
{
    .probe = VPoxDrvProbe,
# if RTLNX_VER_MAX(2,6,30)
    .suspend = VPoxDrvSuspend,
    .resume  = VPoxDrvResume,
# endif
    /** @todo .shutdown? */
    .driver =
    {
        .name = "vpoxdrv",
# if RTLNX_VER_MIN(2,6,30)
        .pm = &gPlatformPMOps,
# endif
    }
};

static struct platform_device gPlatformDevice =
{
    .name = "vpoxdrv",
    .dev =
    {
        .release = VPoxDevRelease
    }
};
#endif /* VPOX_WITH_SUSPEND_NOTIFICATION */


DECLINLINE(RTUID) vpoxdrvLinuxUid(void)
{
#if RTLNX_VER_MIN(2,6,29)
# if RTLNX_VER_MIN(3,5,0)
    return from_kuid(current_user_ns(), current->cred->uid);
# else
    return current->cred->uid;
# endif
#else
    return current->uid;
#endif
}

DECLINLINE(RTGID) vpoxdrvLinuxGid(void)
{
#if RTLNX_VER_MIN(2,6,29)
# if RTLNX_VER_MIN(3,5,0)
    return from_kgid(current_user_ns(), current->cred->gid);
# else
    return current->cred->gid;
# endif
#else
    return current->gid;
#endif
}

DECLINLINE(RTUID) vpoxdrvLinuxEuid(void)
{
#if RTLNX_VER_MIN(2,6,29)
# if RTLNX_VER_MIN(3,5,0)
    return from_kuid(current_user_ns(), current->cred->euid);
# else
    return current->cred->euid;
# endif
#else
    return current->euid;
#endif
}

/**
 * Initialize module.
 *
 * @returns appropriate status code.
 */
static int __init VPoxDrvLinuxInit(void)
{
    int       rc;

    /*
     * Check for synchronous/asynchronous TSC mode.
     */
    printk(KERN_DEBUG "vpoxdrv: Found %u processor cores\n", (unsigned)RTMpGetOnlineCount());
    rc = misc_register(&gMiscDeviceSys);
    if (rc)
    {
        printk(KERN_ERR "vpoxdrv: Can't register system misc device! rc=%d\n", rc);
        return rc;
    }
    rc = misc_register(&gMiscDeviceUsr);
    if (rc)
    {
        printk(KERN_ERR "vpoxdrv: Can't register user misc device! rc=%d\n", rc);
        misc_deregister(&gMiscDeviceSys);
        return rc;
    }
    if (!rc)
    {
        /*
         * Initialize the runtime.
         * On AMD64 we'll have to donate the high rwx memory block to the exec allocator.
         */
        rc = RTR0Init(0);
        if (RT_SUCCESS(rc))
        {
#if (defined(RT_ARCH_AMD64) && RTLNX_VER_MAX(2,6,23)) || defined(VPOX_WITH_TEXT_MODMEM_HACK)
# ifdef VPOX_WITH_TEXT_MODMEM_HACK
            set_memory_x(&g_abExecMemory[0], sizeof(g_abExecMemory) / PAGE_SIZE);
            set_memory_rw(&g_abExecMemory[0], sizeof(g_abExecMemory) / PAGE_SIZE);
# endif
            rc = RTR0MemExecDonate(&g_abExecMemory[0], sizeof(g_abExecMemory));
            printk(KERN_DEBUG "VPoxDrv: dbg - g_abExecMemory=%p\n", (void *)&g_abExecMemory[0]);
#endif
            Log(("VPoxDrv::ModuleInit\n"));

            /*
             * Initialize the device extension.
             */
            if (RT_SUCCESS(rc))
                rc = supdrvInitDevExt(&g_DevExt, sizeof(SUPDRVSESSION));
            if (RT_SUCCESS(rc))
            {
#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
                rc = platform_driver_register(&gPlatformDriver);
                if (rc == 0)
                {
                    rc = platform_device_register(&gPlatformDevice);
                    if (rc == 0)
#endif
                    {
                        printk(KERN_INFO "vpoxdrv: TSC mode is %s, tentative frequency %llu Hz\n",
                               SUPGetGIPModeName(g_DevExt.pGip), g_DevExt.pGip->u64CpuHz);
                        LogFlow(("VPoxDrv::ModuleInit returning %#x\n", rc));
                        printk(KERN_DEBUG "vpoxdrv: Successfully loaded version "
                                VPOX_VERSION_STRING " r" RT_XSTR(VPOX_SVN_REV)
                                VPOX_EXTRA_VERSION_STRING
                                " (interface " RT_XSTR(SUPDRV_IOC_VERSION) ")\n");
                        return rc;
                    }
#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
                    else
                        platform_driver_unregister(&gPlatformDriver);
                }
#endif
            }

            rc = -EINVAL;
            RTR0TermForced();
        }
        else
            rc = -EINVAL;

        /*
         * Failed, cleanup and return the error code.
         */
    }
    misc_deregister(&gMiscDeviceSys);
    misc_deregister(&gMiscDeviceUsr);
    Log(("VPoxDrv::ModuleInit returning %#x (minor:%d & %d)\n", rc, gMiscDeviceSys.minor, gMiscDeviceUsr.minor));
    return rc;
}


/**
 * Unload the module.
 */
static void __exit VPoxDrvLinuxUnload(void)
{
    Log(("VPoxDrvLinuxUnload\n"));

#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
    platform_device_unregister(&gPlatformDevice);
    platform_driver_unregister(&gPlatformDriver);
#endif

    /*
     * I Don't think it's possible to unload a driver which processes have
     * opened, at least we'll blindly assume that here.
     */
    misc_deregister(&gMiscDeviceUsr);
    misc_deregister(&gMiscDeviceSys);

    /*
     * Destroy GIP, delete the device extension and terminate IPRT.
     */
    supdrvDeleteDevExt(&g_DevExt);
    RTR0TermForced();
}


/**
 * Common open code.
 *
 * @param   pInode          Pointer to inode info structure.
 * @param   pFilp           Associated file pointer.
 * @param   fUnrestricted   Indicates which device node which was opened.
 */
static int vpoxdrvLinuxCreateCommon(struct inode *pInode, struct file *pFilp, bool fUnrestricted)
{
    int                 rc;
    PSUPDRVSESSION      pSession;
    Log(("VPoxDrvLinuxCreate: pFilp=%p pid=%d/%d %s\n", pFilp, RTProcSelf(), current->pid, current->comm));

#ifdef VPOX_WITH_HARDENING
    /*
     * Only root is allowed to access the unrestricted device, enforce it!
     */
    if (   fUnrestricted
        && vpoxdrvLinuxEuid() != 0 /* root */ )
    {
        Log(("VPoxDrvLinuxCreate: euid=%d, expected 0 (root)\n", vpoxdrvLinuxEuid()));
        return -EPERM;
    }
#endif /* VPOX_WITH_HARDENING */

    /*
     * Call common code for the rest.
     */
    rc = supdrvCreateSession(&g_DevExt, true /* fUser */, fUnrestricted, &pSession);
    if (!rc)
    {
        pSession->Uid = vpoxdrvLinuxUid();
        pSession->Gid = vpoxdrvLinuxGid();
    }

    pFilp->private_data = pSession;

    Log(("VPoxDrvLinuxCreate: g_DevExt=%p pSession=%p rc=%d/%d (pid=%d/%d %s)\n",
         &g_DevExt, pSession, rc, VPoxDrvLinuxErr2LinuxErr(rc),
         RTProcSelf(), current->pid, current->comm));
    return VPoxDrvLinuxErr2LinuxErr(rc);
}


/** /dev/vpoxdrv.  */
static int VPoxDrvLinuxCreateSys(struct inode *pInode, struct file *pFilp)
{
    return vpoxdrvLinuxCreateCommon(pInode, pFilp, true);
}


/** /dev/vpoxdrvu.  */
static int VPoxDrvLinuxCreateUsr(struct inode *pInode, struct file *pFilp)
{
    return vpoxdrvLinuxCreateCommon(pInode, pFilp, false);
}


/**
 * Close device.
 *
 * @param   pInode      Pointer to inode info structure.
 * @param   pFilp       Associated file pointer.
 */
static int VPoxDrvLinuxClose(struct inode *pInode, struct file *pFilp)
{
    Log(("VPoxDrvLinuxClose: pFilp=%p pSession=%p pid=%d/%d %s\n",
         pFilp, pFilp->private_data, RTProcSelf(), current->pid, current->comm));
    supdrvSessionRelease((PSUPDRVSESSION)pFilp->private_data);
    pFilp->private_data = NULL;
    return 0;
}


#ifdef VPOX_WITH_SUSPEND_NOTIFICATION
/**
 * Dummy device release function. We have to provide this function,
 * otherwise the kernel will complain.
 *
 * @param   pDev        Pointer to the platform device.
 */
static void VPoxDevRelease(struct device *pDev)
{
}

/**
 * Dummy probe function.
 *
 * @param   pDev        Pointer to the platform device.
 */
static int VPoxDrvProbe(struct platform_device *pDev)
{
    return 0;
}

/**
 * Suspend callback.
 * @param   pDev        Pointer to the platform device.
 * @param   State       Message type, see Documentation/power/devices.txt.
 *                      Ignored.
 */
# if RTLNX_VER_MIN(2,6,30) && !defined(DOXYGEN_RUNNING)
static int VPoxDrvSuspend(struct device *pDev)
# else
static int VPoxDrvSuspend(struct platform_device *pDev, pm_message_t State)
# endif
{
    RTPowerSignalEvent(RTPOWEREVENT_SUSPEND);
    return 0;
}

/**
 * Resume callback.
 *
 * @param   pDev        Pointer to the platform device.
 */
# if RTLNX_VER_MIN(2,6,30)
static int VPoxDrvResume(struct device *pDev)
# else
static int VPoxDrvResume(struct platform_device *pDev)
# endif
{
    RTPowerSignalEvent(RTPOWEREVENT_RESUME);
    return 0;
}
#endif /* VPOX_WITH_SUSPEND_NOTIFICATION */


/**
 * Device I/O Control entry point.
 *
 * @param   pFilp       Associated file pointer.
 * @param   uCmd        The function specified to ioctl().
 * @param   ulArg       The argument specified to ioctl().
 */
#if defined(HAVE_UNLOCKED_IOCTL) || defined(DOXYGEN_RUNNING)
static long VPoxDrvLinuxIOCtl(struct file *pFilp, unsigned int uCmd, unsigned long ulArg)
#else
static int VPoxDrvLinuxIOCtl(struct inode *pInode, struct file *pFilp, unsigned int uCmd, unsigned long ulArg)
#endif
{
    PSUPDRVSESSION pSession = (PSUPDRVSESSION)pFilp->private_data;
    int rc;
#ifndef VPOX_WITHOUT_EFLAGS_AC_SET_IN_VPOXDRV
# if defined(VPOX_STRICT) || defined(VPOX_WITH_EFLAGS_AC_SET_IN_VPOXDRV)
    RTCCUINTREG fSavedEfl;

    /*
     * Refuse all I/O control calls if we've ever detected EFLAGS.AC being cleared.
     *
     * This isn't a problem, as there is absolutely nothing in the kernel context that
     * depend on user context triggering cleanups.  That would be pretty wild, right?
     */
    if (RT_UNLIKELY(g_DevExt.cBadContextCalls > 0))
    {
        SUPR0Printf("VPoxDrvLinuxIOCtl: EFLAGS.AC=0 detected %u times, refusing all I/O controls!\n", g_DevExt.cBadContextCalls);
        return ESPIPE;
    }

    fSavedEfl = ASMAddFlags(X86_EFL_AC);
# else
    stac();
# endif
#endif

    /*
     * Deal with the two high-speed IOCtl that takes it's arguments from
     * the session and iCmd, and only returns a VPox status code.
     */
    AssertCompile(_IOC_NRSHIFT == 0 && _IOC_NRBITS == 8);
#ifdef HAVE_UNLOCKED_IOCTL
    if (RT_LIKELY(   (unsigned int)(uCmd - SUP_IOCTL_FAST_DO_FIRST) < (unsigned int)32
                  && pSession->fUnrestricted))
        rc = supdrvIOCtlFast(uCmd - SUP_IOCTL_FAST_DO_FIRST, ulArg, &g_DevExt, pSession);
    else
        rc = VPoxDrvLinuxIOCtlSlow(pFilp, uCmd, ulArg, pSession);
#else   /* !HAVE_UNLOCKED_IOCTL */
    unlock_kernel();
    if (RT_LIKELY(   (unsigned int)(uCmd - SUP_IOCTL_FAST_DO_FIRST) < (unsigned int)32
                  && pSession->fUnrestricted))
        rc = supdrvIOCtlFast(uCmd - SUP_IOCTL_FAST_DO_FIRST, ulArg, &g_DevExt, pSession);
    else
        rc = VPoxDrvLinuxIOCtlSlow(pFilp, uCmd, ulArg, pSession);
    lock_kernel();
#endif  /* !HAVE_UNLOCKED_IOCTL */

#ifndef VPOX_WITHOUT_EFLAGS_AC_SET_IN_VPOXDRV
# if defined(VPOX_STRICT) || defined(VPOX_WITH_EFLAGS_AC_SET_IN_VPOXDRV)
    /*
     * Before we restore AC and the rest of EFLAGS, check if the IOCtl handler code
     * accidentially modified it or some other important flag.
     */
    if (RT_UNLIKELY(   (ASMGetFlags() & (X86_EFL_AC | X86_EFL_IF | X86_EFL_DF))
                    != ((fSavedEfl    & (X86_EFL_AC | X86_EFL_IF | X86_EFL_DF)) | X86_EFL_AC) ))
    {
        char szTmp[48];
        RTStrPrintf(szTmp, sizeof(szTmp), "uCmd=%#x: %#x->%#x!", _IOC_NR(uCmd), (uint32_t)fSavedEfl, (uint32_t)ASMGetFlags());
        supdrvBadContext(&g_DevExt, "SUPDrv-linux.c",  __LINE__, szTmp);
    }
    ASMSetFlags(fSavedEfl);
# else
    clac();
# endif
#endif
    return rc;
}


/**
 * Device I/O Control entry point.
 *
 * @param   pFilp       Associated file pointer.
 * @param   uCmd        The function specified to ioctl().
 * @param   ulArg       The argument specified to ioctl().
 * @param   pSession    The session instance.
 */
static int VPoxDrvLinuxIOCtlSlow(struct file *pFilp, unsigned int uCmd, unsigned long ulArg, PSUPDRVSESSION pSession)
{
    int                 rc;
    SUPREQHDR           Hdr;
    PSUPREQHDR          pHdr;
    uint32_t            cbBuf;

    Log6(("VPoxDrvLinuxIOCtl: pFilp=%p uCmd=%#x ulArg=%p pid=%d/%d\n", pFilp, uCmd, (void *)ulArg, RTProcSelf(), current->pid));

    /*
     * Read the header.
     */
    if (RT_FAILURE(RTR0MemUserCopyFrom(&Hdr, ulArg, sizeof(Hdr))))
    {
        Log(("VPoxDrvLinuxIOCtl: copy_from_user(,%#lx,) failed; uCmd=%#x\n", ulArg, uCmd));
        return -EFAULT;
    }
    if (RT_UNLIKELY((Hdr.fFlags & SUPREQHDR_FLAGS_MAGIC_MASK) != SUPREQHDR_FLAGS_MAGIC))
    {
        Log(("VPoxDrvLinuxIOCtl: bad header magic %#x; uCmd=%#x\n", Hdr.fFlags & SUPREQHDR_FLAGS_MAGIC_MASK, uCmd));
        return -EINVAL;
    }

    /*
     * Buffer the request.
     */
    cbBuf = RT_MAX(Hdr.cbIn, Hdr.cbOut);
    if (RT_UNLIKELY(cbBuf > _1M*16))
    {
        Log(("VPoxDrvLinuxIOCtl: too big cbBuf=%#x; uCmd=%#x\n", cbBuf, uCmd));
        return -E2BIG;
    }
    if (RT_UNLIKELY(_IOC_SIZE(uCmd) ? cbBuf != _IOC_SIZE(uCmd) : Hdr.cbIn < sizeof(Hdr)))
    {
        Log(("VPoxDrvLinuxIOCtl: bad ioctl cbBuf=%#x _IOC_SIZE=%#x; uCmd=%#x\n", cbBuf, _IOC_SIZE(uCmd), uCmd));
        return -EINVAL;
    }
    pHdr = RTMemAlloc(cbBuf);
    if (RT_UNLIKELY(!pHdr))
    {
        OSDBGPRINT(("VPoxDrvLinuxIOCtl: failed to allocate buffer of %d bytes for uCmd=%#x\n", cbBuf, uCmd));
        return -ENOMEM;
    }
    if (RT_FAILURE(RTR0MemUserCopyFrom(pHdr, ulArg, Hdr.cbIn)))
    {
        Log(("VPoxDrvLinuxIOCtl: copy_from_user(,%#lx, %#x) failed; uCmd=%#x\n", ulArg, Hdr.cbIn, uCmd));
        RTMemFree(pHdr);
        return -EFAULT;
    }
    if (Hdr.cbIn < cbBuf)
        RT_BZERO((uint8_t *)pHdr + Hdr.cbIn, cbBuf - Hdr.cbIn);

    /*
     * Process the IOCtl.
     */
    rc = supdrvIOCtl(uCmd, &g_DevExt, pSession, pHdr, cbBuf);

    /*
     * Copy ioctl data and output buffer back to user space.
     */
    if (RT_LIKELY(!rc))
    {
        uint32_t cbOut = pHdr->cbOut;
        if (RT_UNLIKELY(cbOut > cbBuf))
        {
            OSDBGPRINT(("VPoxDrvLinuxIOCtl: too much output! %#x > %#x; uCmd=%#x!\n", cbOut, cbBuf, uCmd));
            cbOut = cbBuf;
        }
        if (RT_FAILURE(RTR0MemUserCopyTo(ulArg, pHdr, cbOut)))
        {
            /* this is really bad! */
            OSDBGPRINT(("VPoxDrvLinuxIOCtl: copy_to_user(%#lx,,%#x); uCmd=%#x!\n", ulArg, cbOut, uCmd));
            rc = -EFAULT;
        }
    }
    else
    {
        Log(("VPoxDrvLinuxIOCtl: pFilp=%p uCmd=%#x ulArg=%p failed, rc=%d\n", pFilp, uCmd, (void *)ulArg, rc));
        rc = -EINVAL;
    }
    RTMemFree(pHdr);

    Log6(("VPoxDrvLinuxIOCtl: returns %d (pid=%d/%d)\n", rc, RTProcSelf(), current->pid));
    return rc;
}


/**
 * The SUPDRV IDC entry point.
 *
 * @returns VPox status code, see supdrvIDC.
 * @param   uReq        The request code.
 * @param   pReq        The request.
 */
int VPOXCALL SUPDrvLinuxIDC(uint32_t uReq, PSUPDRVIDCREQHDR pReq)
{
    PSUPDRVSESSION  pSession;

    /*
     * Some quick validations.
     */
    if (RT_UNLIKELY(!VALID_PTR(pReq)))
        return VERR_INVALID_POINTER;

    pSession = pReq->pSession;
    if (pSession)
    {
        if (RT_UNLIKELY(!VALID_PTR(pSession)))
            return VERR_INVALID_PARAMETER;
        if (RT_UNLIKELY(pSession->pDevExt != &g_DevExt))
            return VERR_INVALID_PARAMETER;
    }
    else if (RT_UNLIKELY(uReq != SUPDRV_IDC_REQ_CONNECT))
        return VERR_INVALID_PARAMETER;

    /*
     * Do the job.
     */
    return supdrvIDC(uReq, &g_DevExt, pSession, pReq);
}

EXPORT_SYMBOL(SUPDrvLinuxIDC);


RTCCUINTREG VPOXCALL supdrvOSChangeCR4(RTCCUINTREG fOrMask, RTCCUINTREG fAndMask)
{
#if RTLNX_VER_MIN(5,8,0)
    unsigned long fSavedFlags;
    local_irq_save(fSavedFlags);
    RTCCUINTREG const uOld = cr4_read_shadow();
    cr4_update_irqsoff(fOrMask, ~fAndMask); /* Same as this function, only it is not returning the old value. */
    AssertMsg(cr4_read_shadow() == ((uOld & fAndMask) | fOrMask),
              ("fOrMask=%#RTreg fAndMask=%#RTreg uOld=%#RTreg; new cr4=%#llx\n", fOrMask, fAndMask, uOld, cr4_read_shadow()));
    local_irq_restore(fSavedFlags);
#else
# if RTLNX_VER_MIN(3,20,0)
    RTCCUINTREG const uOld = this_cpu_read(cpu_tlbstate.cr4);
# else
    RTCCUINTREG const uOld = ASMGetCR4();
# endif
    RTCCUINTREG const uNew = (uOld & fAndMask) | fOrMask;
    if (uNew != uOld)
    {
# if RTLNX_VER_MIN(3,20,0)
        this_cpu_write(cpu_tlbstate.cr4, uNew);
        __write_cr4(uNew);
# else
        ASMSetCR4(uNew);
# endif
    }
#endif
    return uOld;
}


void VPOXCALL supdrvOSCleanupSession(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession)
{
    NOREF(pDevExt);
    NOREF(pSession);
}


void VPOXCALL supdrvOSSessionHashTabInserted(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession, void *pvUser)
{
    NOREF(pDevExt); NOREF(pSession); NOREF(pvUser);
}


void VPOXCALL supdrvOSSessionHashTabRemoved(PSUPDRVDEVEXT pDevExt, PSUPDRVSESSION pSession, void *pvUser)
{
    NOREF(pDevExt); NOREF(pSession); NOREF(pvUser);
}


/**
 * Initializes any OS specific object creator fields.
 */
void VPOXCALL supdrvOSObjInitCreator(PSUPDRVOBJ pObj, PSUPDRVSESSION pSession)
{
    NOREF(pObj);
    NOREF(pSession);
}


/**
 * Checks if the session can access the object.
 *
 * @returns true if a decision has been made.
 * @returns false if the default access policy should be applied.
 *
 * @param   pObj        The object in question.
 * @param   pSession    The session wanting to access the object.
 * @param   pszObjName  The object name, can be NULL.
 * @param   prc         Where to store the result when returning true.
 */
bool VPOXCALL supdrvOSObjCanAccess(PSUPDRVOBJ pObj, PSUPDRVSESSION pSession, const char *pszObjName, int *prc)
{
    NOREF(pObj);
    NOREF(pSession);
    NOREF(pszObjName);
    NOREF(prc);
    return false;
}


bool VPOXCALL supdrvOSGetForcedAsyncTscMode(PSUPDRVDEVEXT pDevExt)
{
    return force_async_tsc != 0;
}


bool VPOXCALL supdrvOSAreCpusOfflinedOnSuspend(void)
{
    return true;
}


bool VPOXCALL supdrvOSAreTscDeltasInSync(void)
{
    return false;
}


int  VPOXCALL   supdrvOSLdrOpen(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, const char *pszFilename)
{
    NOREF(pDevExt); NOREF(pImage); NOREF(pszFilename);
    return VERR_NOT_SUPPORTED;
}


int  VPOXCALL   supdrvOSLdrValidatePointer(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, void *pv,
                                           const uint8_t *pbImageBits, const char *pszSymbol)
{
    NOREF(pDevExt); NOREF(pImage); NOREF(pv); NOREF(pbImageBits); NOREF(pszSymbol);
    return VERR_NOT_SUPPORTED;
}


int  VPOXCALL   supdrvOSLdrLoad(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, const uint8_t *pbImageBits, PSUPLDRLOAD pReq)
{
    NOREF(pDevExt); NOREF(pImage); NOREF(pbImageBits); NOREF(pReq);
    return VERR_NOT_SUPPORTED;
}


void VPOXCALL   supdrvOSLdrUnload(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage)
{
    NOREF(pDevExt); NOREF(pImage);
}


/** @def VPOX_WITH_NON_PROD_HACK_FOR_PERF_STACKS
 * A very crude hack for debugging using perf and dtrace.
 *
 * DO ABSOLUTELY NOT ENABLE IN PRODUCTION BUILDS!  DEVELOPMENT ONLY!!
 * DO ABSOLUTELY NOT ENABLE IN PRODUCTION BUILDS!  DEVELOPMENT ONLY!!
 * DO ABSOLUTELY NOT ENABLE IN PRODUCTION BUILDS!  DEVELOPMENT ONLY!!
 *
 */
#if 0 || defined(DOXYGEN_RUNNING)
# define VPOX_WITH_NON_PROD_HACK_FOR_PERF_STACKS
#endif

#if defined(VPOX_WITH_NON_PROD_HACK_FOR_PERF_STACKS) && defined(CONFIG_MODULES_TREE_LOOKUP)
/** Whether g_pfnModTreeInsert and g_pfnModTreeRemove have been initialized.
 * @remarks can still be NULL after init. */
static volatile bool g_fLookedForModTreeFunctions = false;
static void (*g_pfnModTreeInsert)(struct mod_tree_node *) = NULL;   /**< __mod_tree_insert */
static void (*g_pfnModTreeRemove)(struct mod_tree_node *) = NULL;   /**< __mod_tree_remove */
#endif


void VPOXCALL   supdrvOSLdrNotifyOpened(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage, const char *pszFilename)
{
#ifdef VPOX_WITH_NON_PROD_HACK_FOR_PERF_STACKS /* Not for production use!! Debugging only! */
    /*
     * This trick stops working with 4.2 when CONFIG_MODULES_TREE_LOOKUP is
     * defined.  The module lookups are done via a tree structure and we
     * cannot get at the root of it. :-(
     */
# ifdef CONFIG_KALLSYMS
    size_t const cchName = strlen(pImage->szName);
# endif
    struct module *pMyMod, *pSelfMod, *pTestMod, *pTestModByName;
    IPRT_LINUX_SAVE_EFL_AC();

    pImage->pLnxModHack    = NULL;

# ifdef CONFIG_MODULES_TREE_LOOKUP
    /*
     * This is pretty naive, but works for 4.2 on arch linux.  I don't think we
     * can count on finding __mod_tree_remove in all kernel builds as it's not
     * marked noinline like __mod_tree_insert.
     */
    if (!g_fLookedForModTreeFunctions)
    {
        unsigned long ulInsert = kallsyms_lookup_name("__mod_tree_insert");
        unsigned long ulRemove = kallsyms_lookup_name("__mod_tree_remove");
        if (!ulInsert || !ulRemove)
        {
            g_fLookedForModTreeFunctions = true;
            printk(KERN_ERR "vpoxdrv: failed to locate __mod_tree_insert and __mod_tree_remove.\n");
            IPRT_LINUX_RESTORE_EFL_AC();
            return;
        }
        *(unsigned long *)&g_pfnModTreeInsert = ulInsert;
        *(unsigned long *)&g_pfnModTreeRemove = ulRemove;
        ASMCompilerBarrier();
        g_fLookedForModTreeFunctions = true;
    }
    else if (!g_pfnModTreeInsert || !g_pfnModTreeRemove)
        return;
#endif

    /*
     * Make sure we've found our own module, otherwise we cannot access the linked list.
     */
    mutex_lock(&module_mutex);
    pSelfMod = find_module("vpoxdrv");
    mutex_unlock(&module_mutex);
    if (!pSelfMod)
    {
        IPRT_LINUX_RESTORE_EFL_AC();
        return;
    }

    /*
     * Cook up a module structure for the image.
     * We allocate symbol and string tables in the allocation and the module to keep things simple.
     */
# ifdef CONFIG_KALLSYMS
    pMyMod = (struct module *)RTMemAllocZ(sizeof(*pMyMod)
                                          + sizeof(Elf_Sym) * 3
                                          + 1 + cchName * 2 + sizeof("_start") + sizeof("_end") + 4 );
# else
    pMyMod = (struct module *)RTMemAllocZ(sizeof(*pMyMod));
# endif
    if (pMyMod)
    {
        int rc = VINF_SUCCESS;
# ifdef CONFIG_KALLSYMS
        Elf_Sym *paSymbols = (Elf_Sym *)(pMyMod + 1);
        char    *pchStrTab = (char *)(paSymbols + 3);
# endif

        pMyMod->state = MODULE_STATE_LIVE;
        INIT_LIST_HEAD(&pMyMod->list);  /* just in case */

        /* Perf only matches up files with a .ko extension (maybe .ko.gz),
           so in order for this crap to work smoothly, we append .ko to the
           module name and require the user to create symbolic links in
           /lib/modules/`uname -r`:
                for i in VMMR0.r0 VPoxDDR0.r0 VPoxDD2R0.r0; do
                    sudo ln -s /mnt/scratch/vpox/svn/trunk/out/linux.amd64/debug/bin/$i /lib/modules/`uname -r`/$i.ko;
                done  */
        RTStrPrintf(pMyMod->name, sizeof(pMyMod->name), "%s", pImage->szName);

        /* sysfs bits. */
        INIT_LIST_HEAD(&pMyMod->mkobj.kobj.entry); /* rest of kobj is already zeroed, hopefully never accessed... */
        pMyMod->mkobj.mod           = pMyMod;
        pMyMod->mkobj.drivers_dir   = NULL;
        pMyMod->mkobj.mp            = NULL;
        pMyMod->mkobj.kobj_completion = NULL;

        pMyMod->modinfo_attrs       = NULL; /* hopefully not accessed after setup. */
        pMyMod->holders_dir         = NULL; /* hopefully not accessed. */
        pMyMod->version             = "N/A";
        pMyMod->srcversion          = "N/A";

        /* We export no symbols. */
        pMyMod->num_syms            = 0;
        pMyMod->syms                = NULL;
        pMyMod->crcs                = NULL;

        pMyMod->num_gpl_syms        = 0;
        pMyMod->gpl_syms            = NULL;
        pMyMod->gpl_crcs            = NULL;

        pMyMod->num_gpl_future_syms = 0;
        pMyMod->gpl_future_syms     = NULL;
        pMyMod->gpl_future_crcs     = NULL;

# if CONFIG_UNUSED_SYMBOLS
        pMyMod->num_unused_syms     = 0;
        pMyMod->unused_syms         = NULL;
        pMyMod->unused_crcs         = NULL;

        pMyMod->num_unused_gpl_syms = 0;
        pMyMod->unused_gpl_syms     = NULL;
        pMyMod->unused_gpl_crcs     = NULL;
# endif
        /* No kernel parameters either. */
        pMyMod->kp                  = NULL;
        pMyMod->num_kp              = 0;

# ifdef CONFIG_MODULE_SIG
        /* Pretend ok signature. */
        pMyMod->sig_ok              = true;
# endif
        /* No exception table. */
        pMyMod->num_exentries       = 0;
        pMyMod->extable             = NULL;

        /* No init function */
        pMyMod->init                = NULL;
        pMyMod->module_init         = NULL;
        pMyMod->init_size           = 0;
        pMyMod->init_ro_size        = 0;
        pMyMod->init_text_size      = 0;

        /* The module address and size. It's all text. */
        pMyMod->module_core         = pImage->pvImage;
        pMyMod->core_size           = pImage->cbImageBits;
        pMyMod->core_text_size      = pImage->cbImageBits;
        pMyMod->core_ro_size        = pImage->cbImageBits;

#ifdef CONFIG_MODULES_TREE_LOOKUP
        /* Fill in the self pointers for the tree nodes. */
        pMyMod->mtn_core.mod        = pMyMod;
        pMyMod->mtn_init.mod        = pMyMod;
#endif
        /* They invented the tained bit for us, didn't they? */
        pMyMod->taints              = 1;

# ifdef CONFIG_GENERIC_BUGS
        /* No BUGs in our modules. */
        pMyMod->num_bugs            = 0;
        INIT_LIST_HEAD(&pMyMod->bug_list);
        pMyMod->bug_table           = NULL;
# endif

# ifdef CONFIG_KALLSYMS
        /* The core stuff is documented as only used when loading. So just zero them. */
        pMyMod->core_num_syms       = 0;
        pMyMod->core_symtab         = NULL;
        pMyMod->core_strtab         = NULL;

        /* Construct a symbol table with start and end symbols.
           Note! We don't have our own symbol table at this point, image bit
                 are not uploaded yet! */
        pMyMod->num_symtab          = 3;
        pMyMod->symtab              = paSymbols;
        pMyMod->strtab              = pchStrTab;
        RT_ZERO(paSymbols[0]);
        pchStrTab[0] = '\0';
        paSymbols[1].st_name        = 1;
        paSymbols[2].st_name        = 2 + RTStrPrintf(&pchStrTab[paSymbols[1].st_name], cchName + sizeof("_start"),
                                                      "%s_start", pImage->szName);
        RTStrPrintf(&pchStrTab[paSymbols[2].st_name], cchName + sizeof("_end"), "%s_end", pImage->szName);
        paSymbols[1].st_info = 't';
        paSymbols[2].st_info = 'b';
        paSymbols[1].st_other = 0;
        paSymbols[2].st_other = 0;
        paSymbols[1].st_shndx = 0;
        paSymbols[2].st_shndx = 0;
        paSymbols[1].st_value = (uintptr_t)pImage->pvImage;
        paSymbols[2].st_value = (uintptr_t)pImage->pvImage + pImage->cbImageBits - 1;
        paSymbols[1].st_size  = pImage->cbImageBits - 1;
        paSymbols[2].st_size  = 1;
# endif
        /* No arguments, but seems its always non-NULL so put empty string there. */
        pMyMod->args                = "";

# ifdef CONFIG_SMP
        /* No per CPU data. */
        pMyMod->percpu              = NULL;
        pMyMod->percpu_size         = 0;
# endif
# ifdef CONFIG_TRACEPOINTS
        /* No tracepoints we like to share. */
        pMyMod->num_tracepoints     = 0;
        pMyMod->tracepoints_ptrs    = NULL;
#endif
# ifdef HAVE_JUMP_LABEL
        /* No jump lable stuff either. */
        pMyMod->jump_entries        = NULL;
        pMyMod->num_jump_entries    = 0;
# endif
# ifdef CONFIG_TRACING
        pMyMod->num_trace_bprintk_fmt   = 0;
        pMyMod->trace_bprintk_fmt_start = NULL;
# endif
# ifdef CONFIG_EVENT_TRACING
        pMyMod->trace_events        = NULL;
        pMyMod->num_trace_events    = 0;
# endif
# ifdef CONFIG_FTRACE_MCOUNT_RECORD
        pMyMod->num_ftrace_callsites = 0;
        pMyMod->ftrace_callsites    = NULL;
# endif
# ifdef CONFIG_MODULE_UNLOAD
        /* Dependency lists, not worth sharing */
        INIT_LIST_HEAD(&pMyMod->source_list);
        INIT_LIST_HEAD(&pMyMod->target_list);

        /* Nobody waiting and no exit function. */
#  if RTLNX_VER_MAX(3,13,0)
        pMyMod->waiter              = NULL;
#  endif
        pMyMod->exit                = NULL;

        /* References, very important as we must not allow the module
           to be unloaded using rmmod. */
#  if RTLNX_VER_MIN(3,19,0)
        atomic_set(&pMyMod->refcnt, 42);
#  else
        pMyMod->refptr              = alloc_percpu(struct module_ref);
        if (pMyMod->refptr)
        {
            int iCpu;
            for_each_possible_cpu(iCpu)
            {
                per_cpu_ptr(pMyMod->refptr, iCpu)->decs = 0;
                per_cpu_ptr(pMyMod->refptr, iCpu)->incs = 1;
            }
        }
        else
            rc = VERR_NO_MEMORY;
#  endif
# endif
# ifdef CONFIG_CONSTRUCTORS
        /* No constructors. */
        pMyMod->ctors               = NULL;
        pMyMod->num_ctors           = 0;
# endif
        if (RT_SUCCESS(rc))
        {
            bool fIsModText;

            /*
             * Add the module to the list.
             */
            mutex_lock(&module_mutex);
            list_add_rcu(&pMyMod->list, &pSelfMod->list);
            pImage->pLnxModHack = pMyMod;
# ifdef CONFIG_MODULES_TREE_LOOKUP
            g_pfnModTreeInsert(&pMyMod->mtn_core); /* __mod_tree_insert */
# endif
            mutex_unlock(&module_mutex);

            /*
             * Test it.
             */
            mutex_lock(&module_mutex);
            pTestModByName = find_module(pMyMod->name);
            pTestMod = __module_address((uintptr_t)pImage->pvImage + pImage->cbImageBits / 4);
            fIsModText = __module_text_address((uintptr_t)pImage->pvImage + pImage->cbImageBits / 2);
            mutex_unlock(&module_mutex);
            if (   pTestMod == pMyMod
                && pTestModByName == pMyMod
                && fIsModText)
                printk(KERN_ERR "vpoxdrv: fake module works for '%s' (%#lx to %#lx)\n",
                       pMyMod->name, (unsigned long)paSymbols[1].st_value, (unsigned long)paSymbols[2].st_value);
            else
                printk(KERN_ERR "vpoxdrv: failed to find fake module (pTestMod=%p, pTestModByName=%p, pMyMod=%p, fIsModText=%d)\n",
                       pTestMod, pTestModByName, pMyMod, fIsModText);
        }
        else
            RTMemFree(pMyMod);
    }

    IPRT_LINUX_RESTORE_EFL_AC();
#else
    pImage->pLnxModHack    = NULL;
#endif
    NOREF(pDevExt); NOREF(pImage);
}


void VPOXCALL   supdrvOSLdrNotifyUnloaded(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage)
{
#ifdef VPOX_WITH_NON_PROD_HACK_FOR_PERF_STACKS /* Not for production use!! Debugging only! */
    struct module *pMyMod = pImage->pLnxModHack;
    pImage->pLnxModHack = NULL;
    if (pMyMod)
    {
        /*
         * Remove the fake module list entry and free it.
         */
        IPRT_LINUX_SAVE_EFL_AC();
        mutex_lock(&module_mutex);
        list_del_rcu(&pMyMod->list);
# ifdef CONFIG_MODULES_TREE_LOOKUP
        g_pfnModTreeRemove(&pMyMod->mtn_core);
# endif
        synchronize_sched();
        mutex_unlock(&module_mutex);

# if RTLNX_VER_MAX(3,19,0)
        free_percpu(pMyMod->refptr);
# endif
        RTMemFree(pMyMod);
        IPRT_LINUX_RESTORE_EFL_AC();
    }

#else
    Assert(pImage->pLnxModHack == NULL);
#endif
    NOREF(pDevExt); NOREF(pImage);
}


int  VPOXCALL   supdrvOSLdrQuerySymbol(PSUPDRVDEVEXT pDevExt, PSUPDRVLDRIMAGE pImage,
                                       const char *pszSymbol, size_t cchSymbol, void **ppvSymbol)
{
#ifdef VPOX_WITH_NON_PROD_HACK_FOR_PERF_STACKS
# error "implement me!"
#endif
    RT_NOREF(pDevExt, pImage, pszSymbol, cchSymbol, ppvSymbol);
    return VERR_WRONG_ORDER;
}


#ifdef SUPDRV_WITH_MSR_PROBER

int VPOXCALL    supdrvOSMsrProberRead(uint32_t uMsr, RTCPUID idCpu, uint64_t *puValue)
{
# ifdef SUPDRV_LINUX_HAS_SAFE_MSR_API
    uint32_t u32Low, u32High;
    int rc;

    IPRT_LINUX_SAVE_EFL_AC();
    if (idCpu == NIL_RTCPUID)
        rc = rdmsr_safe(uMsr, &u32Low, &u32High);
    else if (RTMpIsCpuOnline(idCpu))
        rc = rdmsr_safe_on_cpu(idCpu, uMsr, &u32Low, &u32High);
    else
        return VERR_CPU_OFFLINE;
    IPRT_LINUX_RESTORE_EFL_AC();
    if (rc == 0)
    {
        *puValue = RT_MAKE_U64(u32Low, u32High);
        return VINF_SUCCESS;
    }
    return VERR_ACCESS_DENIED;
# else
    return VERR_NOT_SUPPORTED;
# endif
}


int VPOXCALL    supdrvOSMsrProberWrite(uint32_t uMsr, RTCPUID idCpu, uint64_t uValue)
{
# ifdef SUPDRV_LINUX_HAS_SAFE_MSR_API
    int rc;

    IPRT_LINUX_SAVE_EFL_AC();
    if (idCpu == NIL_RTCPUID)
        rc = wrmsr_safe(uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue));
    else if (RTMpIsCpuOnline(idCpu))
        rc = wrmsr_safe_on_cpu(idCpu, uMsr, RT_LODWORD(uValue), RT_HIDWORD(uValue));
    else
        return VERR_CPU_OFFLINE;
    IPRT_LINUX_RESTORE_EFL_AC();

    if (rc == 0)
        return VINF_SUCCESS;
    return VERR_ACCESS_DENIED;
# else
    return VERR_NOT_SUPPORTED;
# endif
}

# ifdef SUPDRV_LINUX_HAS_SAFE_MSR_API
/**
 * Worker for supdrvOSMsrProberModify.
 */
static DECLCALLBACK(void) supdrvLnxMsrProberModifyOnCpu(RTCPUID idCpu, void *pvUser1, void *pvUser2)
{
    PSUPMSRPROBER               pReq    = (PSUPMSRPROBER)pvUser1;
    register uint32_t           uMsr    = pReq->u.In.uMsr;
    bool const                  fFaster = pReq->u.In.enmOp == SUPMSRPROBEROP_MODIFY_FASTER;
    uint64_t                    uBefore;
    uint64_t                    uWritten;
    uint64_t                    uAfter;
    int                         rcBefore, rcWrite, rcAfter, rcRestore;
    RTCCUINTREG                 fOldFlags;

    /* Initialize result variables. */
    uBefore = uWritten = uAfter    = 0;
    rcWrite = rcAfter  = rcRestore = -EIO;

    /*
     * Do the job.
     */
    fOldFlags = ASMIntDisableFlags();
    ASMCompilerBarrier(); /* paranoia */
    if (!fFaster)
        ASMWriteBackAndInvalidateCaches();

    rcBefore = rdmsrl_safe(uMsr, &uBefore);
    if (rcBefore >= 0)
    {
        register uint64_t uRestore = uBefore;
        uWritten  = uRestore;
        uWritten &= pReq->u.In.uArgs.Modify.fAndMask;
        uWritten |= pReq->u.In.uArgs.Modify.fOrMask;

        rcWrite   = wrmsr_safe(uMsr, RT_LODWORD(uWritten), RT_HIDWORD(uWritten));
        rcAfter   = rdmsrl_safe(uMsr, &uAfter);
        rcRestore = wrmsr_safe(uMsr, RT_LODWORD(uRestore), RT_HIDWORD(uRestore));

        if (!fFaster)
        {
            ASMWriteBackAndInvalidateCaches();
            ASMReloadCR3();
            ASMNopPause();
        }
    }

    ASMCompilerBarrier(); /* paranoia */
    ASMSetFlags(fOldFlags);

    /*
     * Write out the results.
     */
    pReq->u.Out.uResults.Modify.uBefore    = uBefore;
    pReq->u.Out.uResults.Modify.uWritten   = uWritten;
    pReq->u.Out.uResults.Modify.uAfter     = uAfter;
    pReq->u.Out.uResults.Modify.fBeforeGp  = rcBefore  != 0;
    pReq->u.Out.uResults.Modify.fModifyGp  = rcWrite   != 0;
    pReq->u.Out.uResults.Modify.fAfterGp   = rcAfter   != 0;
    pReq->u.Out.uResults.Modify.fRestoreGp = rcRestore != 0;
    RT_ZERO(pReq->u.Out.uResults.Modify.afReserved);
}
# endif


int VPOXCALL    supdrvOSMsrProberModify(RTCPUID idCpu, PSUPMSRPROBER pReq)
{
# ifdef SUPDRV_LINUX_HAS_SAFE_MSR_API
    if (idCpu == NIL_RTCPUID)
    {
        supdrvLnxMsrProberModifyOnCpu(idCpu, pReq, NULL);
        return VINF_SUCCESS;
    }
    return RTMpOnSpecific(idCpu, supdrvLnxMsrProberModifyOnCpu, pReq, NULL);
# else
    return VERR_NOT_SUPPORTED;
# endif
}

#endif /* SUPDRV_WITH_MSR_PROBER */


/**
 * Converts a supdrv error code to an linux error code.
 *
 * @returns corresponding linux error code.
 * @param   rc          IPRT status code.
 */
static int VPoxDrvLinuxErr2LinuxErr(int rc)
{
    switch (rc)
    {
        case VINF_SUCCESS:              return 0;
        case VERR_GENERAL_FAILURE:      return -EACCES;
        case VERR_INVALID_PARAMETER:    return -EINVAL;
        case VERR_INVALID_MAGIC:        return -EILSEQ;
        case VERR_INVALID_HANDLE:       return -ENXIO;
        case VERR_INVALID_POINTER:      return -EFAULT;
        case VERR_LOCK_FAILED:          return -ENOLCK;
        case VERR_ALREADY_LOADED:       return -EEXIST;
        case VERR_PERMISSION_DENIED:    return -EPERM;
        case VERR_VERSION_MISMATCH:     return -ENOSYS;
        case VERR_IDT_FAILED:           return -1000;
    }

    return -EPERM;
}


SUPR0DECL(int) SUPR0HCPhysToVirt(RTHCPHYS HCPhys, void **ppv)
{
    AssertReturn(!(HCPhys & PAGE_OFFSET_MASK), VERR_INVALID_POINTER);
    AssertReturn(HCPhys != NIL_RTHCPHYS, VERR_INVALID_POINTER);
    /* Would've like to use valid_phys_addr_range for this test, but it isn't exported. */
    AssertReturn((HCPhys | PAGE_OFFSET_MASK) < __pa(high_memory), VERR_INVALID_POINTER);
    *ppv = phys_to_virt(HCPhys);
    return VINF_SUCCESS;
}


RTDECL(int) SUPR0PrintfV(const char *pszFormat, va_list va)
{
    char    szMsg[512];
    IPRT_LINUX_SAVE_EFL_AC();

    RTStrPrintfV(szMsg, sizeof(szMsg) - 1, pszFormat, va);
    szMsg[sizeof(szMsg) - 1] = '\0';

    printk("%s", szMsg);

    IPRT_LINUX_RESTORE_EFL_AC();
    return 0;
}


SUPR0DECL(uint32_t) SUPR0GetKernelFeatures(void)
{
    uint32_t fFlags = 0;
#ifdef CONFIG_PAX_KERNEXEC
    fFlags |= SUPKERNELFEATURES_GDT_READ_ONLY;
#endif
#if RTLNX_VER_MIN(4,12,0)
    fFlags |= SUPKERNELFEATURES_GDT_NEED_WRITABLE;
#endif
#if defined(VPOX_STRICT) || defined(VPOX_WITH_EFLAGS_AC_SET_IN_VPOXDRV)
    fFlags |= SUPKERNELFEATURES_SMAP;
#elif defined(CONFIG_X86_SMAP)
    if (ASMGetCR4() & X86_CR4_SMAP)
        fFlags |= SUPKERNELFEATURES_SMAP;
#endif
    return fFlags;
}


SUPR0DECL(bool) SUPR0FpuBegin(bool fCtxHook)
{
    RT_NOREF(fCtxHook);
#if RTLNX_VER_MIN(4,19,0) /* Going back to 4.19.0 for better coverage, we
                             probably only need 5.17.7+ in the end. */
    /*
     * HACK ALERT!
     *
     * We'd like to use the old __kernel_fpu_begin() API which was removed in
     * early 2019, because we typically run with preemption enabled and have an
     * preemption hook installed which will call kernel_fpu_end() in case we're
     * scheduled out after getting in here.  The preemption hook is almost
     * useless if we run with preemption disabled.
     *
     * For the case where the kernel does not have preemption hooks, we get here
     * with preemption already disabled and one more count doesn't make any
     * difference.
     *
     * So, after the kernel_fpu_begin() call we undo the implicit preempt_disable()
     * call it does, so the preemption hook can do its work and the VPox user has
     * a more responsive system.
     *
     * See @bugref{10209#c12} and onwards for more details.
     */
    Assert(fCtxHook || !RTThreadPreemptIsEnabled(NIL_RTTHREAD));
    kernel_fpu_begin();
# if 0 /* Always do it for now for better test coverage. */
    if (fCtxHook)
# endif
        preempt_enable();
    return false; /** @todo Not sure if we have license to use any extended state, or
                   *        if we're limited to the SSE & x87 FPU. If it's the former,
                   *        we should return @a true and the caller can skip
                   *        saving+restoring the host state and save some time. */
#else
    return false;
#endif
}


SUPR0DECL(void) SUPR0FpuEnd(bool fCtxHook)
{
    RT_NOREF(fCtxHook);
#if RTLNX_VER_MIN(4,19,0)
    /* HACK ALERT! See SUPR0FpuBegin for an explanation of this. */
    Assert(!RTThreadPreemptIsEnabled(NIL_RTTHREAD));
# if 0 /* Always do it for now for better test coverage. */
    if (fCtxHook)
# endif
        preempt_disable();
    kernel_fpu_end();
#endif
}


int VPOXCALL    supdrvOSGetCurrentGdtRw(RTHCUINTPTR *pGdtRw)
{
#if RTLNX_VER_MIN(4,12,0)
    *pGdtRw = (RTHCUINTPTR)get_current_gdt_rw();
    return VINF_SUCCESS;
#else
    return VERR_NOT_IMPLEMENTED;
#endif
}


module_init(VPoxDrvLinuxInit);
module_exit(VPoxDrvLinuxUnload);

MODULE_AUTHOR(VPOX_VENDOR);
MODULE_DESCRIPTION(VPOX_PRODUCT " Support Driver");
MODULE_LICENSE("GPL");
#ifdef MODULE_VERSION
MODULE_VERSION(VPOX_VERSION_STRING " r" RT_XSTR(VPOX_SVN_REV) " (" RT_XSTR(SUPDRV_IOC_VERSION) ")");
#endif

module_param(force_async_tsc, int, 0444);
MODULE_PARM_DESC(force_async_tsc, "force the asynchronous TSC mode");

