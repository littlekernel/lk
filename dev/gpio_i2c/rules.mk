# I2C bit-banging using GPIO lines.
#
# Before including the module, define:
#
#   GPIO_I2C_BUS_COUNT: number of SDA/SCL pairs to be used.
#   GPIO_I2C_PULLUPS (optional): if 1, enables pullups.

LOCAL_DIR := $(GET_LOCAL_DIR)

MODULE := $(LOCAL_DIR)

MODULE_SRCS := \
	$(LOCAL_DIR)/gpio_i2c.c

MODULE_DEFINES += \
	GPIO_I2C_BUS_COUNT=$(GPIO_I2C_BUS_COUNT)

ifneq ($(GPIO_I2C_PULLUPS),)
MODULE_DEFINES += \
	GPIO_I2C_PULLUPS=$(GPIO_I2C_PULLUPS)
endif

include make/module.mk
