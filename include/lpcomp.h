/*
 * Copyright (c) 2017 ARM Ltd
 * Copyright (c) 2015-2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Public APIs for GPIO drivers
 */

#ifndef __LPCOMP_H__
#define __LPCOMP_H__

#include <zephyr/types.h>
#include <device.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * @typedef lpcomp_callback_t
 * @brief Define the application callback function signature for LPCOMP.
 *
 * @param lpcomp Device struct for the LPCOMP device.
 */
typedef void *(*lpcomp_callback_handler_t)(void);

/**
 * @brief COMP Driver APIs
 * @defgroup lpcomp_interface COMP Driver APIs
 * @ingroup io_interfaces
 * @{
 */

/**
 * @cond INTERNAL_HIDDEN
 *
 * COMP driver API definition and system call entry points
 *
 * (Internal use only.)
 */

struct lpcomp_driver_api {

	//int (*config)(struct device *dev, u32_t input, int flags);

	/** Pointer to the enable routine. */
	void (*enable)(struct device *dev);

	/** Pointer to the disable routine. */
	void (*disable)(struct device *dev);

	/** Set the callback function */
	void (*set_callback)(struct device *dev, lpcomp_callback_handler_t cb);
};

/*
__syscall int lpcomp_config(struct device *port, int access_op, u32_t pin,
			  int flags);

static inline int _impl_lpcomp_config(struct device *dev, u32_t input, int flags)
{
	const struct lpcomp_driver_api *api =
		(const struct lpcomp_driver_api *)dev->driver_api;

	return api->config(dev, input, flags);
}*/

__syscall void lpcomp_enable(struct device *dev);

static inline void _impl_lpcomp_enable(struct device *dev)
{
	const struct lpcomp_driver_api *api = dev->driver_api;

	api->enable(dev);
}

__syscall void lpcomp_disable(struct device *dev);

static inline void _impl_lpcomp_disable(struct device *dev)
{
	const struct lpcomp_driver_api *api = dev->driver_api;

	api->disable(dev);
}

__syscall void lpcomp_callback_set(struct device *dev, lpcomp_callback_handler_t cb);

static inline void _impl_lpcomp_callback_set(struct device *dev,
					lpcomp_callback_handler_t cb)
{
	const struct lpcomp_driver_api *api =
		(const struct lpcomp_driver_api *)dev->driver_api;

	__ASSERT(cb, "Callback pointer should not be NULL");

	if ((api != NULL) && (api->set_callback != NULL)) {
		api->set_callback(dev, cb);
	}
}

/**
 * @}
 */


#ifdef __cplusplus
}
#endif

#include <syscalls/lpcomp.h>

#endif /* __LPCOMP_H__ */
