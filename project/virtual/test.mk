# common libraries for -test variants

MODULES += \
  app/shell \
  app/stringtests \
  app/tests \
  arch/test \
  lib/aes \
  lib/aes/test \
  lib/cksum \
  lib/debugcommands \
  lib/unittest \
  lib/version \

# set a build system variable for other modules to include test code
# on their own.
WITH_TESTS := true
GLOBAL_DEFINES += WITH_TESTS=1

