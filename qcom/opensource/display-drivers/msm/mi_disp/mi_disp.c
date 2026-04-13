/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2020 XiaoMi, Inc. All rights reserved.
 * Copyright (c) 2026 Vaisakh Murali
 */

#define pr_fmt(fmt) "mi_disp: " fmt

#include <linux/cdev.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <drm/drm_connector.h>

#include "dsi_display.h"
#include "dsi_panel.h"
#include "mi_disp.h"

/* External references */
extern int dsi_display_set_backlight(struct drm_connector *connector, void *display, u32 bl_lvl);
extern int dsi_display_get_active_displays(void **display_array, u32 max_display_count);
extern int dsi_panel_tx_cmd_set(struct dsi_panel *panel, enum dsi_cmd_set_type type);

static struct mi_disp *g_mi_disp = NULL;

struct mi_disp *get_disp_core(void)
{
    return g_mi_disp;
}
EXPORT_SYMBOL(get_disp_core);

struct dsi_display *get_display(void)
{
    void *active_displays[1] = { NULL };
    int count;

    count = dsi_display_get_active_displays(active_displays, 1);

    if (count > 0 && active_displays[0]) {
        return (struct dsi_display *)active_displays[0];
    }

    return NULL;
}

static int mi_disp_get_version(unsigned long arg)
{
    struct disp_version req;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
        return -EFAULT;

    // Default to what xiaomi has
    req.version = 0x0100;

    pr_debug("%s: Faking version response: 0x%04x\n", __func__, req.version);

    if (copy_to_user((void __user *)arg, &req, sizeof(req)))
        return -EFAULT;

    return 0;
}

static int mi_disp_stub_event(unsigned long arg, bool is_register)
{
    struct disp_event_req req;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
        return -EFAULT;

    pr_debug("%s: HAL %s event type: %u. Faking success.\n",
             __func__, is_register ? "registered" : "deregistered", req.type);

    return 0;
}

static int mi_disp_get_brightness(struct dsi_display *display, unsigned long arg)
{
    struct disp_brightness_req req;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
        pr_err("%s: Failed to copy disp_brightness_req\n", __func__);
        return -EFAULT;
    }

    if (!display->panel) {
        pr_err("%s: DSI panel not initialized\n", __func__);
        return -ENODEV;
    }

    req.brightness = display->panel->bl_config.bl_level;
    
    pr_debug("%s: Returning brightness: %u\n", __func__, req.brightness);

    if (copy_to_user((void __user *)arg, &req, sizeof(req))) {
        pr_err("%s: Failed to return disp_brightness_req to user\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int mi_disp_get_feature(struct dsi_display *display, unsigned long arg)
{
    struct disp_feature_req req;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
        pr_err("%s: Failed to copy disp_feature_req\n", __func__);
        return -EFAULT;
    }

    if (!display->panel) {
        pr_err("%s: DSI panel not initialized\n", __func__);
        return -ENODEV;
    }

    switch (req.feature_id) {
        case DISP_FEATURE_BRIGHTNESS:
        case DISP_FEATURE_BACKLIGHT:
            req.feature_val = display->panel->bl_config.bl_level;
            pr_debug("%s: Returning feature brightness: %d\n", __func__, req.feature_val);
            break;

        default:
            pr_debug("%s: Unhandled GET feature_id: %u\n", __func__, req.feature_id);
            req.feature_val = 0;
            break;
    }

    if (copy_to_user((void __user *)arg, &req, sizeof(req))) {
        pr_err("%s: Failed to return disp_feature_req to user\n", __func__);
        return -EFAULT;
    }

    return 0;
}

static int mi_disp_set_feature(struct dsi_display *display, unsigned long arg)
{
    struct disp_feature_req req;
    int rc = 0;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
        pr_err("%s: Failed to copy disp_feature_req\n", __func__);
        return -EFAULT;
    }

    switch (req.feature_id) {
        case DISP_FEATURE_BRIGHTNESS:
        case DISP_FEATURE_BACKLIGHT:
            pr_debug("%s: requested brightness: %d\n", __func__, req.feature_val);
            rc = dsi_display_set_backlight(NULL, display, (u32)req.feature_val);
            break;
        case DISP_FEATURE_LOCAL_HBM:
            pr_debug("%s: feature requested LHBM state: %d\n", __func__, req.feature_val);
            rc = mi_disp_set_local_hbm(req.feature_val);
            break;

        default:
            // Ignore unneeded features
            pr_debug("%s: Unhandled feature_id: %u\n", __func__, req.feature_id);
            break;
    }

    return rc;
}

static int mi_disp_set_brightness(struct dsi_display *display, unsigned long arg)
{
    struct disp_brightness_req req;
    int rc = 0;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
        pr_err("%s: Failed to copy disp_brightness_req\n", __func__);
        return -EFAULT;
    }

    pr_debug("%s: ioctl requested brightness: %u\n", __func__, req.brightness);

    rc = dsi_display_set_backlight(NULL, display, req.brightness);

    return rc;
}

int mi_disp_set_local_hbm(int state)
{
    struct dsi_display *display = get_display();
    int rc = 0;

    if (!display || !display->panel) {
        pr_err_ratelimited("%s: DSI display/panel not initialized\n", __func__);
        return -ENODEV;
    }

    pr_debug("%s: Setting LHBM state to: %d\n", __func__, state);

    mutex_lock(&display->display_lock);

    switch (state) {
        case LOCAL_HBM_NORMAL_WHITE_1000NIT:
            rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_SET_MI_LOCAL_HBM_NORMAL_WHITE_1000NIT); 
            break;

        case LOCAL_HBM_OFF_TO_NORMAL:
            rc = dsi_panel_tx_cmd_set(display->panel, DSI_CMD_SET_MI_LOCAL_HBM_OFF_TO_NORMAL);
            break;

        default:
            pr_debug("%s: Unhandled LHBM state: %d\n", __func__, state);
            break;
    }

    mutex_unlock(&display->display_lock);
    return rc;
}
EXPORT_SYMBOL(mi_disp_set_local_hbm);

static int mi_disp_set_local_hbm_ioctl(struct dsi_display *display, unsigned long arg)
{
    struct disp_local_hbm_req req;

    if (copy_from_user(&req, (void __user *)arg, sizeof(req))) {
        pr_err("%s: Failed to copy disp_local_hbm_req\n", __func__);
        return -EFAULT;
    }

    return mi_disp_set_local_hbm(req.local_hbm_value);
}

long mi_disp_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    struct dsi_display *display = get_display();
    unsigned int nr = _IOC_NR(cmd);

    if (!display) {
        pr_err_ratelimited("%s: DSI display not initialized\n", __func__);
        return -ENODEV;
    }

    if (_IOC_TYPE(cmd) != 'D') {
        return -ENOTTY;
    }

    switch (nr) {
        case _IOC_NR(MI_DISP_IOCTL_VERSION):
            return mi_disp_get_version(arg);

        case _IOC_NR(MI_DISP_IOCTL_REGISTER_EVENT):
            return mi_disp_stub_event(arg, true);

        case _IOC_NR(MI_DISP_IOCTL_DEREGISTER_EVENT):
            return mi_disp_stub_event(arg, false);

        case _IOC_NR(MI_DISP_IOCTL_GET_FEATURE):
            return mi_disp_get_feature(display, arg);

        case _IOC_NR(MI_DISP_IOCTL_GET_BRIGHTNESS):
            return mi_disp_get_brightness(display, arg);

        case _IOC_NR(MI_DISP_IOCTL_SET_FEATURE):
            return mi_disp_set_feature(display, arg);

        case _IOC_NR(MI_DISP_IOCTL_SET_BRIGHTNESS):
            return mi_disp_set_brightness(display, arg);

        case _IOC_NR(MI_DISP_IOCTL_SET_LOCAL_HBM):
            return mi_disp_set_local_hbm_ioctl(display, arg);

        default:
            // More IOCTLs to be added
            pr_debug("%s: Unhandled ioctl sequence: 0x%02x\n", __func__, nr);
            return -ENOTTY;
    }
}

static const struct file_operations mi_disp_fops = {
    .owner = THIS_MODULE,
    .unlocked_ioctl = mi_disp_ioctl,
};

int mi_disp_init(void)
{
    int ret = 0;
    struct mi_disp *mi_disp = NULL;

    if (g_mi_disp) {
        pr_warn("already initialised\n");
        return 0;
    }

    mi_disp = kzalloc(sizeof(*mi_disp), GFP_KERNEL);
    if (!mi_disp) {
        pr_err("Cannot allocate memory\n");
        return -ENOMEM;
    }

    mi_disp->class = class_create(THIS_MODULE, MI_DISPLAY_CLASS);
    if (IS_ERR(mi_disp->class)) {
        pr_err("Class creation failure\n");
        ret = PTR_ERR(mi_disp->class);
        goto err_free_mem;
    }

    ret = alloc_chrdev_region(&mi_disp->dev_id, 0, 1, MI_DISPLAY_CLASS);
    if (ret < 0) {
        pr_err("Failed to allocate chrdev region\n");
        goto err_class_destroy;
    }

    cdev_init(&mi_disp->cdev, &mi_disp_fops);
    ret = cdev_add(&mi_disp->cdev, mi_disp->dev_id, 1);
    if (ret < 0) {
        pr_err("Failed to add cdev\n");
        goto err_unregister_chrdev;
    }

    mi_disp->node = device_create(mi_disp->class, NULL, mi_disp->dev_id, NULL, "common_node");
    if (IS_ERR(mi_disp->node)) {
        pr_err("Failed to create device node\n");
        ret = PTR_ERR(mi_disp->node);
        goto err_cdev_del;
    }

    mi_disp->proc_dir = proc_mkdir(MI_DISPLAY_CLASS, NULL);
	if (!mi_disp->proc_dir) {
		pr_err("ProcFS creation failure\n");
		ret = -ENOMEM;
        goto err_proc_mkdir;;
	}

	mi_disp->debugfs_dir = debugfs_create_dir(MI_DISPLAY_CLASS, NULL);

	g_mi_disp = mi_disp;

    pr_info("initialised!\n");
    return 0;

err_proc_mkdir:
    device_destroy(mi_disp->class, mi_disp->dev_id);
err_cdev_del:
    cdev_del(&mi_disp->cdev);
err_unregister_chrdev:
    unregister_chrdev_region(mi_disp->dev_id, 1);
err_class_destroy:
    class_destroy(mi_disp->class);
err_free_mem:
    kfree(mi_disp);
    return ret;
}

void mi_disp_exit(void)
{
    if (!g_mi_disp)
        return;

	debugfs_remove_recursive(g_mi_disp->debugfs_dir);
	remove_proc_entry(MI_DISPLAY_CLASS, NULL);
    device_destroy(g_mi_disp->class, g_mi_disp->dev_id);
    cdev_del(&g_mi_disp->cdev);
    unregister_chrdev_region(g_mi_disp->dev_id, 1);
	class_destroy(g_mi_disp->class);
	kfree(g_mi_disp);
	g_mi_disp = NULL;
}

MODULE_DESCRIPTION("Xiaomi Display");
MODULE_LICENSE("GPL v2");