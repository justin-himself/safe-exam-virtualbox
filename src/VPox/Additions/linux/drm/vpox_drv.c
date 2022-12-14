/*  $Id: vpox_drv.c $ */
/** @file
 * VirtualPox Additions Linux kernel video driver
 */

/*
 * Copyright (C) 2013-2020 Oracle Corporation
 * This file is based on ast_drv.c
 * Copyright 2012 Red Hat Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * Authors: Dave Airlie <airlied@redhat.com>
 *          Michael Thayer <michael.thayer@oracle.com,
 *          Hans de Goede <hdegoede@redhat.com>
 */
#include <linux/module.h>
#include <linux/console.h>
#include <linux/vt_kern.h>

#include "vpox_drv.h"

#include <drm/drm_crtc_helper.h>
#if RTLNX_VER_MIN(5,1,0) || RTLNX_RHEL_MAJ_PREREQ(8,1)
# include <drm/drm_probe_helper.h>
#endif

#if RTLNX_VER_MIN(5,14,0) || RTLNX_RHEL_RANGE(8,6, 8,99)
# include <drm/drm_aperture.h>
#endif

#include "version-generated.h"
#include "revision-generated.h"

/** Detect whether kernel mode setting is OFF. */
#if defined(CONFIG_VGA_CONSOLE)
# if RTLNX_VER_MIN(5,17,0)
#  define VPOX_VIDEO_NOMODESET() drm_firmware_drivers_only() && vpox_modeset == -1
# elif RTLNX_VER_MIN(4,7,0)
#  define VPOX_VIDEO_NOMODESET() vgacon_text_force() && vpox_modeset == -1
# else /* < 4.7.0 */
# define VPOX_VIDEO_NOMODESET() 0
# endif /* < 4.7.0 */
#else /* !CONFIG_VGA_CONSOLE */
# define VPOX_VIDEO_NOMODESET() 0
#endif /* !CONFIG_VGA_CONSOLE */

static int vpox_modeset = -1;

MODULE_PARM_DESC(modeset, "Disable/Enable modesetting");
module_param_named(modeset, vpox_modeset, int, 0400);

static struct drm_driver driver;

static const struct pci_device_id pciidlist[] = {
	{ 0x90ee, 0xbeef, PCI_ANY_ID, PCI_ANY_ID, 0, 0, 0 },
	{ 0, 0, 0},
};
MODULE_DEVICE_TABLE(pci, pciidlist);

static int vpox_pci_probe(struct pci_dev *pdev, const struct pci_device_id *ent)
{
#if RTLNX_VER_MIN(4,19,0) || RTLNX_RHEL_MIN(8,3)
	struct drm_device *dev = NULL;
	int ret = 0;

# if RTLNX_VER_MIN(5,14,0) || RTLNX_RHEL_RANGE(8,6, 8,99)
#  if RTLNX_VER_MIN(5,15,0) || RTLNX_RHEL_MIN(9,1)
	ret = drm_aperture_remove_conflicting_pci_framebuffers(pdev, &driver);
#  else
	ret = drm_aperture_remove_conflicting_pci_framebuffers(pdev, "vpoxvideofb");
#  endif
	if (ret)
	{
		printk("unable to remove conflicting framebuffer devices\n");
		return ret;
	}
# endif /* >= 5.14. */

	dev = drm_dev_alloc(&driver, &pdev->dev);
	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		goto err_drv_alloc;
	}
# if RTLNX_VER_MAX(5,14,0) && !RTLNX_RHEL_RANGE(8,6, 8,99)
	dev->pdev = pdev;
# endif
	pci_set_drvdata(pdev, dev);

	ret = vpox_driver_load(dev);
	if (ret)
		goto err_vpox_driver_load;

	ret = drm_dev_register(dev, 0);
	if (ret)
		goto err_drv_dev_register;
	return ret;

err_drv_dev_register:
	vpox_driver_unload(dev);
err_vpox_driver_load:
	drm_dev_put(dev);
err_drv_alloc:
	return ret;
#else /* < 4.19.0 || RHEL < 8.3 */
	return drm_get_pci_dev(pdev, ent, &driver);
#endif
}

static void vpox_pci_remove(struct pci_dev *pdev)
{
	struct drm_device *dev = pci_get_drvdata(pdev);

#if RTLNX_VER_MAX(4,19,0)
	drm_put_dev(dev);
#else
	drm_dev_unregister(dev);
	vpox_driver_unload(dev);
	drm_dev_put(dev);
#endif
}

#if RTLNX_VER_MAX(4,9,0) && !RTLNX_RHEL_MAJ_PREREQ(7,4)
static void drm_fb_helper_set_suspend_unlocked(struct drm_fb_helper *fb_helper,
					bool suspend)
{
	if (!fb_helper || !fb_helper->fbdev)
		return;

	console_lock();
	fb_set_suspend(fb_helper->fbdev, suspend);
	console_unlock();
}
#endif

static int vpox_drm_freeze(struct drm_device *dev)
{
	struct vpox_private *vpox = dev->dev_private;

	drm_kms_helper_poll_disable(dev);

	pci_save_state(VPOX_DRM_TO_PCI_DEV(dev));

	drm_fb_helper_set_suspend_unlocked(&vpox->fbdev->helper, true);

	return 0;
}

static int vpox_drm_thaw(struct drm_device *dev)
{
	struct vpox_private *vpox = dev->dev_private;

	drm_mode_config_reset(dev);
	drm_helper_resume_force_mode(dev);
	drm_fb_helper_set_suspend_unlocked(&vpox->fbdev->helper, false);

	return 0;
}

static int vpox_drm_resume(struct drm_device *dev)
{
	int ret;

	if (pci_enable_device(VPOX_DRM_TO_PCI_DEV(dev)))
		return -EIO;

	ret = vpox_drm_thaw(dev);
	if (ret)
		return ret;

	drm_kms_helper_poll_enable(dev);

	return 0;
}

static int vpox_pm_suspend(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *ddev = pci_get_drvdata(pdev);
	int error;

	error = vpox_drm_freeze(ddev);
	if (error)
		return error;

	pci_disable_device(pdev);
	pci_set_power_state(pdev, PCI_D3hot);

	return 0;
}

static int vpox_pm_resume(struct device *dev)
{
	struct drm_device *ddev = pci_get_drvdata(to_pci_dev(dev));

	return vpox_drm_resume(ddev);
}

static int vpox_pm_freeze(struct device *dev)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct drm_device *ddev = pci_get_drvdata(pdev);

	if (!ddev || !ddev->dev_private)
		return -ENODEV;

	return vpox_drm_freeze(ddev);
}

static int vpox_pm_thaw(struct device *dev)
{
	struct drm_device *ddev = pci_get_drvdata(to_pci_dev(dev));

	return vpox_drm_thaw(ddev);
}

static int vpox_pm_poweroff(struct device *dev)
{
	struct drm_device *ddev = pci_get_drvdata(to_pci_dev(dev));

	return vpox_drm_freeze(ddev);
}

static const struct dev_pm_ops vpox_pm_ops = {
	.suspend = vpox_pm_suspend,
	.resume = vpox_pm_resume,
	.freeze = vpox_pm_freeze,
	.thaw = vpox_pm_thaw,
	.poweroff = vpox_pm_poweroff,
	.restore = vpox_pm_resume,
};

static struct pci_driver vpox_pci_driver = {
	.name = DRIVER_NAME,
	.id_table = pciidlist,
	.probe = vpox_pci_probe,
	.remove = vpox_pci_remove,
	.driver.pm = &vpox_pm_ops,
};

#if RTLNX_VER_MAX(4,7,0) && !RTLNX_RHEL_MAJ_PREREQ(7,4)
/* This works around a bug in X servers prior to 1.18.4, which sometimes
 * submit more dirty rectangles than the kernel is willing to handle and
 * then disable dirty rectangle handling altogether when they see the
 * EINVAL error.  I do not want the code to hang around forever, which is
 * why I am limiting it to certain kernel versions.  We can increase the
 * limit if some distributions uses old X servers with new kernels. */
long vpox_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long rc = drm_ioctl(filp, cmd, arg);

	if (cmd == DRM_IOCTL_MODE_DIRTYFB && rc == -EINVAL)
		return -EOVERFLOW;

	return rc;
}
#endif /* RTLNX_VER_MAX(4,7,0) && !RTLNX_RHEL_MAJ_PREREQ(7,4) */

static const struct file_operations vpox_fops = {
	.owner = THIS_MODULE,
	.open = drm_open,
	.release = drm_release,
#if RTLNX_VER_MAX(4,7,0) && !RTLNX_RHEL_MAJ_PREREQ(7,4)
	.unlocked_ioctl = vpox_ioctl,
#else
	.unlocked_ioctl = drm_ioctl,
#endif
	.mmap = vpox_mmap,
	.poll = drm_poll,
#if RTLNX_VER_MAX(3,12,0) && !RTLNX_RHEL_MAJ_PREREQ(7,0)
	.fasync = drm_fasync,
#endif
#ifdef CONFIG_COMPAT
	.compat_ioctl = drm_compat_ioctl,
#endif
	.read = drm_read,
};

#if RTLNX_VER_MIN(5,9,0) || RTLNX_RHEL_MIN(8,4) || RTLNX_SUSE_MAJ_PREREQ(15,3)
static void
#else
static int
#endif
vpox_master_set(struct drm_device *dev,
		struct drm_file *file_priv, bool from_open)
{
	struct vpox_private *vpox = dev->dev_private;

	/*
	 * We do not yet know whether the new owner can handle hotplug, so we
	 * do not advertise dynamic modes on the first query and send a
	 * tentative hotplug notification after that to see if they query again.
	 */
	vpox->initial_mode_queried = false;

	mutex_lock(&vpox->hw_mutex);
	/* Start the refresh timer in case the user does not provide dirty
	 * rectangles. */
	vpox->need_refresh_timer = true;
	schedule_delayed_work(&vpox->refresh_work, VPOX_REFRESH_PERIOD);
	mutex_unlock(&vpox->hw_mutex);

#if RTLNX_VER_MAX(5,9,0) && !RTLNX_RHEL_MAJ_PREREQ(8,4) && !RTLNX_SUSE_MAJ_PREREQ(15,3)
	return 0;
#endif
}

#if RTLNX_VER_MAX(4,8,0) && !RTLNX_RHEL_MAJ_PREREQ(7,4)
static void vpox_master_drop(struct drm_device *dev,
			     struct drm_file *file_priv, bool from_release)
#else
static void vpox_master_drop(struct drm_device *dev, struct drm_file *file_priv)
#endif
{
	struct vpox_private *vpox = dev->dev_private;

	/* See vpox_master_set() */
	vpox->initial_mode_queried = false;
	vpox_report_caps(vpox);

	mutex_lock(&vpox->hw_mutex);
	vpox->need_refresh_timer = false;
	mutex_unlock(&vpox->hw_mutex);
}

static struct drm_driver driver = {
#if RTLNX_VER_MAX(5,4,0) && !RTLNX_RHEL_MAJ_PREREQ(8,3) && !RTLNX_SUSE_MAJ_PREREQ(15,3)
	.driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_HAVE_IRQ |
# if RTLNX_VER_MAX(5,1,0) && !RTLNX_RHEL_MAJ_PREREQ(8,1)
	    DRIVER_IRQ_SHARED |
# endif
	    DRIVER_PRIME,
#else  /* >= 5.4.0 && RHEL >= 8.3 && SLES >= 15-SP3 */
		.driver_features = DRIVER_MODESET | DRIVER_GEM | DRIVER_HAVE_IRQ,
#endif /* <  5.4.0 */

#if RTLNX_VER_MAX(4,19,0) && !RTLNX_RHEL_MAJ_PREREQ(8,3)
	/* Legacy hooks, but still supported. */
	.load = vpox_driver_load,
	.unload = vpox_driver_unload,
#endif
	.lastclose = vpox_driver_lastclose,
	.master_set = vpox_master_set,
	.master_drop = vpox_master_drop,
#if RTLNX_VER_MIN(3,18,0) || RTLNX_RHEL_MAJ_PREREQ(7,2)
# if RTLNX_VER_MAX(4,14,0) && !RTLNX_RHEL_MAJ_PREREQ(7,5) && !RTLNX_SUSE_MAJ_PREREQ(15,1) && !RTLNX_SUSE_MAJ_PREREQ(12,5)
	.set_busid = drm_pci_set_busid,
# endif
#endif

	.fops = &vpox_fops,
#if RTLNX_VER_MAX(5,15,0) && !RTLNX_RHEL_MAJ_PREREQ(9,1)
	.irq_handler = vpox_irq_handler,
#endif
	.name = DRIVER_NAME,
	.desc = DRIVER_DESC,
	.date = DRIVER_DATE,
	.major = DRIVER_MAJOR,
	.minor = DRIVER_MINOR,
	.patchlevel = DRIVER_PATCHLEVEL,

#if RTLNX_VER_MAX(4,7,0)
	.gem_free_object = vpox_gem_free_object,
#endif
	.dumb_create = vpox_dumb_create,
	.dumb_map_offset = vpox_dumb_mmap_offset,
#if RTLNX_VER_MAX(3,12,0) && !RTLNX_RHEL_MAJ_PREREQ(7,3)
	.dumb_destroy = vpox_dumb_destroy,
#elif RTLNX_VER_MAX(5,12,0) && !RTLNX_RHEL_MAJ_PREREQ(8,5)
	.dumb_destroy = drm_gem_dumb_destroy,
#endif
	.prime_handle_to_fd = drm_gem_prime_handle_to_fd,
	.prime_fd_to_handle = drm_gem_prime_fd_to_handle,

	.gem_prime_import = drm_gem_prime_import,
	.gem_prime_import_sg_table = vpox_gem_prime_import_sg_table,
	.gem_prime_mmap = vpox_gem_prime_mmap,

#if RTLNX_VER_MAX(5,11,0) && !RTLNX_RHEL_MAJ_PREREQ(8,5)
	.dev_priv_size = 0,
# if RTLNX_VER_MIN(4,7,0)
	.gem_free_object_unlocked = vpox_gem_free_object,
# endif
	.gem_prime_export = drm_gem_prime_export,
	.gem_prime_pin = vpox_gem_prime_pin,
	.gem_prime_unpin = vpox_gem_prime_unpin,
	.gem_prime_get_sg_table = vpox_gem_prime_get_sg_table,
	.gem_prime_vmap = vpox_gem_prime_vmap,
	.gem_prime_vunmap = vpox_gem_prime_vunmap,
#endif
};

static int __init vpox_init(void)
{
	printk("vpoxvideo: loading version " VPOX_VERSION_STRING " r" __stringify(VPOX_SVN_REV) "\n");
	if (VPOX_VIDEO_NOMODESET())
	{
		printk("vpoxvideo: kernel is running with *nomodeset* parameter,\n");
		printk("vpoxvideo: please consider either to remove it or load driver\n");
		printk("vpoxvideo: with parameter modeset=1, unloading\n");
		return -EINVAL;
	}

	if (vpox_modeset == 0)
	{
		printk("vpoxvideo: driver loaded with modeset=0 parameter, unloading\n");
		return -EINVAL;
	}

#if RTLNX_VER_MIN(3,18,0) || RTLNX_RHEL_MAJ_PREREQ(7,3)
	return pci_register_driver(&vpox_pci_driver);
#else
	return drm_pci_init(&driver, &vpox_pci_driver);
#endif
}

static void __exit vpox_exit(void)
{
#if RTLNX_VER_MIN(3,18,0) || RTLNX_RHEL_MAJ_PREREQ(7,3)
	pci_unregister_driver(&vpox_pci_driver);
#else
	drm_pci_exit(&driver, &vpox_pci_driver);
#endif
}

module_init(vpox_init);
module_exit(vpox_exit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL and additional rights");
#ifdef MODULE_VERSION
MODULE_VERSION(VPOX_VERSION_STRING " r" __stringify(VPOX_SVN_REV));
#endif
