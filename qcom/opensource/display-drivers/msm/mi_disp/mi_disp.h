/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2020 XiaoMi, Inc. All rights reserved.
 * Copyright (c) 2026 Vaisakh Murali
 */

#ifndef _MI_DISP_H_
#define _MI_DISP_H_

#include <linux/types.h>
#include <linux/device.h>
#include <linux/proc_fs.h>

#define MI_DISPLAY_CLASS "mi_display"

struct mi_disp {
    struct class *class;
    struct proc_dir_entry *proc_dir;
    struct dentry *debugfs_dir;
};

int mi_disp_init(void);
void mi_disp_exit(void);

#endif /* _MI_DISP_H_ */