/*
 * Copyright (c) 2012 Corey Tabaka
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef __DEV_DRIVER_H
#define __DEV_DRIVER_H

#include <sys/types.h>
#include <list.h> // for containerof
#include <compiler.h>

struct driver;

/*
 * Contains the data pertaining to an instance of a device. More than one
 * instance may exist for a given driver type (i.e. uart0, uart1, etc..).
 */
struct device {
	const char *name;
	const struct driver *driver;

	/* instance specific config data populated at instantiation */
	const void *config;

	/* instance specific data populated by the driver at init */
	void *state;
	
	// TODO: add generic state, such as suspend/resume state, etc...
};

/* device class, mainly used as a unique magic pointer to validate ops */
struct device_class {
	const char *name;
};

/* standard driver ops; extensions should contain this ops structure */
struct driver_ops {
	const struct device_class *device_class;

	status_t (*init)(struct device *dev);
	status_t (*fini)(struct device *dev);

	status_t (*suspend)(struct device *dev);
	status_t (*resume)(struct device *dev);
};

/* describes a driver, one per driver type */
struct driver {
	const char *type;
	const struct driver_ops *ops;
};

#define DRIVER_EXPORT(type_, ops_) \
	struct driver concat(__driver_, type_) \
		__SECTION(".drivers") = { \
		.type = #type_, \
		.ops = ops_, \
	}

#define DEVICE_INSTANCE(type_, name_, config_) \
	extern struct driver concat(__driver_, type_); \
	struct device concat(__device_, concat(type_, concat(_, name_))) \
		__SECTION(".devices") = { \
		.name = #name_, \
		.driver = &concat(__driver_, type_), \
		.config = config_, \
	}

/*
 * returns the driver specific ops pointer given the device instance, specifc
 * ops type, and generic ops member name within the specific ops structure.
 */
#define device_get_driver_ops(dev, type, member) ({ \
	type *__ops = NULL; \
	if (dev && dev->driver && dev->driver->ops) \
		__ops = containerof(dev->driver->ops, type, member); \
	__ops; \
})

#define device_get_by_name(type_, name_) ({ \
	extern struct device concat(__device_, concat(type_, concat(_, name_))); \
	&concat(__device_, concat(type_, concat(_, name_))); \
})

status_t device_init_all(void);
status_t device_fini_all(void);

status_t device_init(struct device *dev);
status_t device_fini(struct device *dev);

status_t device_suspend(struct device *dev);
status_t device_resume(struct device *dev);

#endif

