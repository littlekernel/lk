include project/target/dartuinoP0.mk
include project/virtual/test.mk
include project/virtual/minip.mk

# Console serial port is on pins PA9(TX) and PB7(RX)

include project/virtual/fs.mk

DISPLAY_PANEL_TYPE ?= LS013B7DH06

MODULES += \
  target/dartuinoP0/projects/system

MODULE_DEPS += \
    app/accelerometer \

MODULE_SRCS += \
    $(LOCAL_DIR)/sensor_bus.c \

GLOBAL_DEFINES += \
	ENABLE_SENSORBUS=1


ifneq ($(DISPLAY_PANEL_TYPE),)

MODULE_SRCS += \
    $(LOCAL_DIR)/memory_lcd.c

GLOBAL_DEFINES += \
    ENABLE_LCD=1

endif

ifeq ($(DISPLAY_PANEL_TYPE),LS013B7DH06)

GLOBAL_DEFINES += \
    LCD_LS013B7DH06=1
MODULE_SRCS += \
    $(LOCAL_DIR)/display/LS013B7DH06.c

else ifeq ($(DISPLAY_PANEL_TYPE),LS027B7DH01)

GLOBAL_DEFINES += \
    LCD_LS027B7DH01=1
MODULE_SRCS += \
    $(LOCAL_DIR)/display/memory_lcd_mono.c

else ifeq ($(DISPLAY_PANEL_TYPE),LS013B7DH03)
GLOBAL_DEFINES += \
    LCD_LS013B7DH03=1
MODULE_SRCS += \
    $(LOCAL_DIR)/display/memory_lcd_mono.c
endif