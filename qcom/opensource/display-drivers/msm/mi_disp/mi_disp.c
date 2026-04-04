/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2020 XiaoMi, Inc. All rights reserved.
 * Copyright (c) 2026 Vaisakh Murali
 */

#define pr_fmt(fmt) "mi_disp: " fmt

#include <linux/debugfs.h>
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

    mi_disp->proc_dir = proc_mkdir(MI_DISPLAY_CLASS, NULL);
	if (!mi_disp->proc_dir) {
		pr_err("ProcFS creation failure\n");
		ret = -ENOMEM;
        goto err_class_destroy;
	}

	mi_disp->debugfs_dir = debugfs_create_dir(MI_DISPLAY_CLASS, NULL);

	g_mi_disp = mi_disp;

    pr_info("initialised!\n");
    return 0;

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
	class_destroy(g_mi_disp->class);
	kfree(g_mi_disp);
	g_mi_disp = NULL;
}

module_init(mi_disp_init);
module_exit(mi_disp_exit);

MODULE_DESCRIPTION("Xiaomi Display");
MODULE_LICENSE("GPL v2");