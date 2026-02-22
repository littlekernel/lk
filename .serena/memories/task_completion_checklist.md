# What to do when a task is completed

1. Build the affected project variant(s), typically one representative QEMU target:
   - e.g. `make qemu-virt-arm64-test`
2. If boot/runtime behavior changed, run appropriate QEMU smoke test:
   - e.g. `scripts/do-qemuarm -6`
3. Run broader regression when needed:
   - `./scripts/run-qemu-boot-tests.py --arch arm64` (or full matrix if required)
4. Keep changes consistent with LK style and module conventions.
5. Check diffs for unrelated edits before handoff.
6. Summarize what changed, risks, and any unverified areas.
