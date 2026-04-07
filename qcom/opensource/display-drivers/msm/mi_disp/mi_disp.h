/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (c) 2020, The Linux Foundation. All rights reserved.
 * Copyright (c) 2020 XiaoMi, Inc. All rights reserved.
 * Copyright (c) 2026 Vaisakh Murali
 */

#ifndef _MI_DISP_H_
#define _MI_DISP_H_

#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/proc_fs.h>

#define MI_DISPLAY_CLASS "mi_display"

struct disp_base {
	__u32 flag;
	__u32 disp_id;
};

enum disp_feature_id {
    DISP_FEATURE_BACKLIGHT = 23,
	DISP_FEATURE_BRIGHTNESS = 24,
};

struct disp_brightness_req {
	struct disp_base base;
	__u32 brightness;
	__u32 brightness_clone;
};

struct disp_feature_req {
	struct disp_base base;
	__u32 feature_id;
	__s32 feature_val;
	__u32 tx_len;
	__u64 tx_ptr;
	__u32 rx_len;
	__u64 rx_ptr;
};

// IOCTLs
#define MI_DISP_IOCTL_SET_BRIGHTNESS           _IOW('D', 0x0C, struct disp_brightness_req)
#define MI_DISP_IOCTL_SET_FEATURE             _IOWR('D', 0x01, struct disp_feature_req)

struct mi_disp {
    struct class *class;
    struct proc_dir_entry *proc_dir;
    struct dentry *debugfs_dir;

    dev_t dev_id;
    struct cdev cdev;
    struct device *node;
};

// Init sequence
int mi_disp_init(void);
void mi_disp_exit(void);

#endif /* _MI_DISP_H_ */