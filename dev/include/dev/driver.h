/*
 * Copyright (c) 2012 Corey Tabaka
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

#include <sys/types.h>
#include <lk/list.h> // for containerof
#include <lk/compiler.h>

__BEGIN_CDECLS

struct driver;

/*
 * Contains the data pertaining to an instance of a device. More than one
 * instance may exist for a given driver type (i.e. uart0, uart1, etc..).
 */
struct device {
    const char *name;
    const struct driver *driver;

    uint32_t flags;

    /* instance specific config data populated at instantiation */
    const void *config;

    /* instance specific data populated by the driver at init */
    void *state;

    // TODO: add generic state, such as suspend/resume state, etc...
    enum {
        DEVICE_UNINITIALIZED,
        DEVICE_INITIALIZED,
        DEVICE_INITIALIZED_FAILED,
    } device_state;
};

/* set this flag to auto initialize the device when device_init_all is called */
#define DEVICE_FLAG_AUTOINIT 0x1

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

/* macro-expanding concat */
#define concat(a, b) __ex_concat(a, b)
#define __ex_concat(a, b) a ## b

#define DRIVER_EXPORT(type_, ops_) \
    const struct driver concat(__driver_, type_) \
        __ALIGNED(sizeof(void *)) __SECTION("drivers") = { \
        .type = #type_, \
        .ops = ops_, \
    }

#define DEVICE_INSTANCE(type_, name_, config_, flags_) \
    extern const struct driver concat(__driver_, type_); \
    struct device concat(__device_, concat(type_, concat(_, name_))) \
        __ALIGNED(sizeof(void *)) __SECTION("devices") = { \
        .name = #name_, \
        .driver = &concat(__driver_, type_), \
        .flags = flags_, \
        .config = config_, \
        .state = NULL, \
        .device_state = DEVICE_UNINITIALIZED, \
    }

/*
 * returns the driver specific ops pointer given the device instance, specific
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

/* initialize all static devices with AUTOINIT flag set */
status_t device_init_all(void);

status_t device_fini_all(void);

status_t device_init(struct device *dev);
status_t device_fini(struct device *dev);

status_t device_suspend(struct device *dev);
status_t device_resume(struct device *dev);

__END_CDECLS

