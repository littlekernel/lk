include project/target/nextcoinP0.mk

# Keep the FS for loading recovery images.
include project/virtual/fs.mk

MODULES += \
  app/moot \
  lib/version \
  lib/buildsig \
  target/nextcoin/projects/bootloader
