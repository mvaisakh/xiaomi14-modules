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
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "mi_disp.h"

static struct mi_disp *g_mi_disp = NULL;

struct mi_disp *get_disp_core(void)
{
    return g_mi_disp;
}
EXPORT_SYMBOL(get_disp_core);

static const struct file_operations mi_disp_fops = {
    .owner = THIS_MODULE,
};

int __init mi_disp_init(void)
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

static void __exit mi_disp_exit(void)
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

module_init(mi_disp_init);
module_exit(mi_disp_exit);

MODULE_DESCRIPTION("Xiaomi Display");
MODULE_LICENSE("GPL v2");