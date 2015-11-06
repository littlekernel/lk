include project/target/stm32746g-eval2.mk
include project/virtual/test.mk
include project/virtual/minip.mk

MODULES += \
    app/loader

LK_HEAP_IMPLEMENTATION=cmpctmalloc
