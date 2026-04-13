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
	DISP_FEATURE_LOCAL_HBM = 9,
	DISP_FEATURE_BACKLIGHT = 23,
	DISP_FEATURE_BRIGHTNESS = 24,
};

struct disp_version {
    struct disp_base base;
    __u32 version;
};

struct disp_event_req {
    struct disp_base base;
    __u32 type;
};

struct disp_brightness_req {
	struct disp_base base;
	__u32 brightness;
	__u32 brightness_clone;
};

struct disp_local_hbm_req {
	struct disp_base base;
	__u32 local_hbm_value;
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

enum local_hbm_state {
	LOCAL_HBM_OFF_TO_NORMAL = 0,
	LOCAL_HBM_NORMAL_WHITE_1000NIT = 1,
	LOCAL_HBM_MAX,
};

// IOCTLs
#define MI_DISP_IOCTL_GET_BRIGHTNESS          _IOWR('D', 0x0B, struct disp_brightness_req)
#define MI_DISP_IOCTL_GET_FEATURE             _IOWR('D', 0x0F, struct disp_feature_req)
#define MI_DISP_IOCTL_SET_BRIGHTNESS           _IOW('D', 0x0C, struct disp_brightness_req)
#define MI_DISP_IOCTL_SET_FEATURE             _IOWR('D', 0x01, struct disp_feature_req)
#define MI_DISP_IOCTL_VERSION                  _IOR('D', 0x00, struct disp_version)
#define MI_DISP_IOCTL_REGISTER_EVENT           _IOW('D', 0x07, struct disp_event_req)
#define MI_DISP_IOCTL_DEREGISTER_EVENT         _IOW('D', 0x08, struct disp_event_req)
#define MI_DISP_IOCTL_SET_LOCAL_HBM            _IOW('D', 0x0E, struct disp_local_hbm_req)

struct mi_disp {
    struct class *class;
    struct proc_dir_entry *proc_dir;
    struct dentry *debugfs_dir;

    dev_t dev_id;
    struct cdev cdev;
    struct device *node;
};

// Local HBM
int mi_disp_set_local_hbm(int state);

// Init sequence
int mi_disp_init(void);
void mi_disp_exit(void);

#endif /* _MI_DISP_H_ */