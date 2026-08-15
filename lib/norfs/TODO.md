# norfs TODO

## Current state: does not build

`lib/norfs` is not referenced by any project, target or platform, and could not
be built if it were.

It was written against a `flash_nor` API that has since been reduced. The
module calls:

- `flash_nor_read()`
- `flash_nor_write()`
- `flash_nor_erase_pages()`
- `FLASH_PTR()`

None of these exist anywhere in the tree any more. All that remains of the API
is `flash_nor_get_bank()` (declared in `dev/include/dev/flash_nor.h`,
implemented only by `platform/stm32f1xx/flash_nor.c`).

It additionally includes `platform/flash_nor_config.h` — for `FLASH_PAGE_SIZE`,
which `lib/norfs/include/lib/norfs_config.h` builds its geometry constants on —
and no platform in the tree supplies that header.

## Way forward: port onto lib/bio

Porting to `lib/bio` would let norfs build and be tested generically, instead of
depending on one platform's NOR driver.

### The easy part

`nvram_read()`, `nvram_write()` and `nvram_erase_pages()` in `norfs.c` are thin
static wrappers over the flash_nor calls. They map onto `bio_read()`,
`bio_write()` and `bio_erase()` directly.

### The direct-pointer part

`nvram_flash_pointer()` returns a raw pointer into memory-mapped flash. bio
already covers this — no new ioctl is needed:

- `BIO_IOCTL_GET_MEM_MAP` — maps device memory and returns the pointer
- `BIO_IOCTL_GET_MAP_ADDR` — fetches the map address without putting the device
  into linear mode
- `BIO_IOCTL_PUT_MEM_MAP` — releases the mapping

See `enum bio_ioctl_num` in `lib/bio/include/lib/bio.h`. `lib/fs/spifs` already
uses this pattern. It is implemented by `lib/bio/mem.c`,
`platform/zynq/spiflash.c`, and `platform/stm32f7xx/{qspi,flash}.c`.

Two callers of `nvram_flash_pointer()` need reworking:

- `collect_garbage_object()` — on the garbage collection path, so this one needs
  the most care
- `load_and_verify_obj()`

Either take the mapping via ioctl, or read into a buffer where a mapping is not
available. Note that a `PUT_MEM_MAP` counterpart is needed wherever a
`GET_MEM_MAP` is taken.

### Payoff: it becomes testable

`lib/bio/mem.c` implements the memory-map ioctl, so a ported norfs could run
against a RAM-backed bdev. That would let `lib/norfs/test` (which already exists
and depends on `lib/unittest`) join the other filesystem tests under
`WITH_TESTS`, rather than needing real NOR hardware.

The geometry constants in `norfs_config.h` would need to come from the bdev's
parameters rather than `FLASH_PAGE_SIZE`, or stay fixed with the test device
configured to match.
