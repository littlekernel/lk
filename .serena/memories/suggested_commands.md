# Suggested commands (Linux)

## Build / clean
- `make pc-x86-64-test`
- `make qemu-virt-riscv64-test`
- `make qemu-virt-arm64-test`
- `make qemu-virt-arm64-test DEBUG=0`
- `make build-qemu-virt-arm64-test clean`
- `make spotless`
- `scripts/buildall -q -e -r`

## Run in QEMU
- `scripts/do-qemuarm -6`
- `scripts/do-qemuarm -6 -P 64k`
- `scripts/do-qemuarm -6 -s -S` (wait for gdb)
- `scripts/do-qemuriscv -6S`
- `scripts/do-qemux86 -6`

## Tests
- `./scripts/run-qemu-boot-tests.py --arch arm64`
- `./scripts/run-qemu-boot-tests.py`
- In LK shell: `ut all`

## Common shell utilities
- `git status`, `git diff`, `git log --oneline`
- `ls`, `cd`, `find`, `grep`, `sed`, `awk`
