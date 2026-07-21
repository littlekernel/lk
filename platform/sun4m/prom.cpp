//
// Copyright (c) 2026 Travis Geiselbrecht
//
// Use of this source code is governed by a MIT-style
// license that can be found in the LICENSE file or at
// https://opensource.org/licenses/MIT
//
#include "platform_p.h"
#include <kernel/novm.h>
#include <kernel/thread.h>
#include <lk/debug.h>
#include <lk/err.h>
#include <lk/reg.h>
#include <lk/trace.h>
#include <platform.h>
#include <platform/debug.h>
#include <platform/interrupts.h>
#include <platform/timer.h>
#include <string.h>
#include <sys/types.h>

#define LOCAL_TRACE 0

namespace {

struct prom_nodeops {
    int (*no_nextnode)(int node);
    int (*no_child)(int node);
    int (*no_proplen)(int node, const char *name);
    int (*no_getprop)(int node, const char *name, char *val);
    int (*no_setprop)(int node, const char *name, char *val, int len);
    const char *(*no_nextprop)(int node, const char *name);
};

struct prom_mlist {
    struct prom_mlist *next;
    char *address;
    unsigned int size;
};

// V0-style physical/virtual memory descriptor tables
struct prom_v0_mem {
    struct prom_mlist **v0_totphys;    // Total physical memory
    struct prom_mlist **v0_prommap;    // Memory used by the PROM
    struct prom_mlist **v0_available;  // Available physical memory
};

// V0 legacy device open/read/write vector
struct prom_v0_devops {
    int (*v0_devopen)(char *dev);
    int (*v0_devclose)(int d);
    int (*v0_rdblkdev)(int d, int nblks, int blkno, char *buf);
    int (*v0_wrblkdev)(int d, int nblks, int blkno, char *buf);
    int (*v0_wrnetdev)(int d, int len, char *buf);
    int (*v0_rdnetdev)(int d, int len, char *buf);
    int (*v0_rdchardev)(int d, int len, int timeout, char *buf);
    int (*v0_wrchardev)(int d, int len, char *buf);
    int (*v0_seekdev)(int d, long offset, int whence);
};

struct prom_v0_bootarg {
    char *name;
    char *val;
};

struct prom_v0_bootargs {
    struct prom_v0_bootarg argv[8];
    char args[100];
    char boot_dev[2];
    int boot_dev_ctrl;
    int boot_dev_unit;
    int dev_partition;
    char *kernel_file_name;
    void *aieee1; // reserved
};

// OBP v2 boot path/args and stdin/stdout ihandles
struct bootargs_v2 {
    char **bootpath;
    char **bootargs;
    int *fd_stdin;
    int *fd_stdout;
};

// OBP v2 ihandle-based device ops
struct dev_v2_funcs {
    int (*v2_inst2pkg)(int d);                                       // Convert ihandle to phandle
    char *(*v2_dumb_mem_alloc)(char *va, unsigned int sz);
    void (*v2_dumb_mem_free)(char *va, unsigned int sz);
    char *(*v2_dumb_mmap)(char *virta, int which_io, unsigned int paddr, unsigned int sz);
    void (*v2_dumb_munmap)(char *virta, unsigned int size);
    int (*v2_dev_open)(char *path);
    void (*v2_dev_close)(int ihandle);
    int (*v2_dev_read)(int ihandle, char *buf, int len);
    int (*v2_dev_write)(int ihandle, const char *buf, int len);
    int (*v2_dev_seek)(int ihandle, long hi, long lo);
    void (*v2_wheee2)(void); // never issued (multistage load support)
    void (*v2_wheee3)(void);
};

// Layout of the PROM's romvec, as handed to us at boot in %o0.
// The structure is versioned: v0/v1 is the base layout, v2 appends
// bootargs_v2/dev_v2_funcs/pv_setctxt, and v3 further appends the
// per-cpu start/stop/idle/resume vector. Later fields are only valid
// to dereference if pv_romvers is high enough to have appended them.
struct prom_romvec {
    // Magic Cookie & Versions
    uint32_t pv_magic_cookie;    // Always 0x10010407
    uint32_t pv_romvers;         // Major version: 0, 2, 3
    uint32_t pv_plugin_revision; // PROM plugin revision
    uint32_t pv_printrev;        // Printable version number

    // Memory Descriptors (V0 Style)
    struct prom_v0_mem pv_v0mem;

    // Device Tree Operations
    struct prom_nodeops *pv_nodeops;

    // Boot & Legacy Device Ops
    char **pv_bootstr;
    struct prom_v0_devops pv_v0devops;

    // Console Identifiers & Character I/O Callbacks
    char *pv_stdin;
    char *pv_stdout;
    int (*pv_getchar)(void);
    void (*pv_putchar)(int ch);
    int (*pv_nbgetchar)(void);
    int (*pv_nbputchar)(int ch);
    void (*pv_putstr)(const char *str, int len);

    // System Control Callbacks
    void (*pv_reboot)(const char *bootstr);
    void (*pv_printf)(const char *fmt, ...);
    void (*pv_abort)(void);
    volatile int *pv_ticks;
    void (*pv_halt)(void);
    void (**pv_synchook)(void);

    // Forth Evaluator (V0 vs V2 Union). v2_eval takes 5 extra int args
    // that get pushed onto the Forth stack before the string is eval'd.
    union {
        void (*v0_eval)(int len, char *str);
        void (*v2_eval)(char *str, int arg0, int arg1, int arg2, int arg3, int arg4);
    } pv_fortheval;

    // V0 Boot Arguments & Network
    struct prom_v0_bootargs **pv_v0bootargs;
    uint32_t (*pv_enaddr)(int d, char *enaddr);

    // --- Appended in OpenBoot Version 2 (OBP v2) ---
    struct bootargs_v2 pv_v2bootargs;
    struct dev_v2_funcs pv_v2devops;

    // Prom version 3 memory allocation
    char *(*pv_memalloc)(char *va, unsigned int size, unsigned int align);

    int filler[14];
    void (*pv_setctxt)(int ctxt, char *va, int pmeg);

    // --- Appended in OpenBoot Version 3 (OBP v3) ---
    int (*pv_cpustart)(unsigned int whichcpu, int ctxtbl_ptr, int thiscontext, char *prog_counter);
    int (*pv_cpustop)(unsigned int whichcpu);
    int (*pv_cpuidle)(unsigned int whichcpu);
    int (*pv_cpuresume)(unsigned int whichcpu);
};

struct prom_nodeops *g_prom_nodeops;
char name_buf[128];
char val_buf[1024];

bool is_printable_string(const char *str, int len) {
    if (len <= 0) {
        return false;
    }
    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (i == len - 1 && c == '\0') {
            return (len > 1); // empty string or string with null terminator
        }
        if (c < 32 || c > 126) {
            return false;
        }
    }
    // Might not be null-terminated
    return false;
}

void print_property_val(const char *name, const char *val, int len, int depth) {
    for (int i = 0; i < depth + 1; i++) {
        dprintf(INFO, "  ");
    }
    dprintf(INFO, "prop: %s = ", name);
    if (is_printable_string(val, len)) {
        dprintf(INFO, "\"%s\"\n", val);
    } else {
        dprintf(INFO, "[");
        for (int i = 0; i < len; i++) {
            dprintf(INFO, "%02x%s", (unsigned char)val[i], (i == len - 1) ? "" : " ");
        }
        dprintf(INFO, "]\n");
    }
}

void dump_prom_node(int node, int depth) {
    if (node == 0) {
        return;
    }

    int len = g_prom_nodeops->no_getprop(node, "name", name_buf);
    if (len <= 0) {
        name_buf[0] = '\0';
    } else {
        if (len >= 128) {
            len = 127;
        }
        name_buf[len] = '\0';
    }

    for (int i = 0; i < depth; i++) {
        dprintf(INFO, "  ");
    }
    dprintf(INFO, "- node: %s (0x%x)\n", name_buf, node);

    // List and print all properties using nextprop safely
    const char *prop_name = g_prom_nodeops->no_nextprop(node, "");
    while (prop_name && prop_name[0] != '\0') {
        int plen = g_prom_nodeops->no_proplen(node, prop_name);
        LTRACEF("Property: %s, length: %d\n", prop_name, plen);
        if (plen > 0) {
            if (plen < 1024) {
                int read_len = g_prom_nodeops->no_getprop(node, prop_name, val_buf);
                if (read_len > 0) {
                    print_property_val(prop_name, val_buf, read_len, depth);
                }
            } else {
                for (int i = 0; i < depth + 1; i++) {
                    dprintf(INFO, "  ");
                }
                dprintf(INFO, "prop: %s = [Too large to print (%d bytes)]\n", prop_name, plen);
            }
        } else {
            for (int i = 0; i < depth + 1; i++) {
                dprintf(INFO, "  ");
            }
            dprintf(INFO, "prop: %s (empty)\n", prop_name);
        }
        prop_name = g_prom_nodeops->no_nextprop(node, prop_name);
    }

    // Recurse children
    int child = g_prom_nodeops->no_child(node);
    if (child != 0) {
        dump_prom_node(child, depth + 1);
    }

    // Traverse siblings
    int next = g_prom_nodeops->no_nextnode(node);
    if (next != 0) {
        dump_prom_node(next, depth);
    }
}

int prom_find_node_by_name_helper(int start_node, const char *name) {
    if (start_node == 0) {
        return 0;
    }

    static char temp_name[128];
    int len = g_prom_nodeops->no_getprop(start_node, "name", temp_name);
    if (len > 0) {
        if (len >= 128) {
            len = 127;
        }
        temp_name[len] = '\0';
        if (strcmp(temp_name, name) == 0) {
            return start_node;
        }
    }

    int child = g_prom_nodeops->no_child(start_node);
    if (child != 0) {
        int found = prom_find_node_by_name_helper(child, name);
        if (found != 0) {
            return found;
        }
    }

    int next = g_prom_nodeops->no_nextnode(start_node);
    if (next != 0) {
        int found = prom_find_node_by_name_helper(next, name);
        if (found != 0) {
            return found;
        }
    }

    return 0;
}

void platform_prom_parse_virtual_memory() {
    if (!g_prom_nodeops) {
        return;
    }
    int root = g_prom_nodeops->no_child(0);
    int vm_node = prom_find_node_by_name_helper(root, "virtual-memory");
    if (vm_node == 0) {
        dprintf(INFO, "PROM: virtual-memory node not found\n");
        return;
    }

    dprintf(INFO, "PROM: Parsing virtual-memory node (0x%x):\n", vm_node);

    // 1. Parse "reg"
    int len = g_prom_nodeops->no_proplen(vm_node, "reg");
    if (len > 0) {
        if (len <= (int)sizeof(val_buf)) {
            int read_len = g_prom_nodeops->no_getprop(vm_node, "reg", (char *)val_buf);
            if (read_len > 0) {
                dprintf(INFO, "  reg (len %d):\n", read_len);
                for (int i = 0; i + 12 <= read_len; i += 12) {
                    uint64_t base = 0;
                    for (int j = 0; j < 8; j++) {
                        base = (base << 8) | val_buf[i + j];
                    }
                    uint32_t size = 0;
                    for (int j = 8; j < 12; j++) {
                        size = (size << 8) | val_buf[i + j];
                    }
                    dprintf(INFO, "    base 0x%llx, size 0x%x\n", (unsigned long long)base, size);
                }
            }
        } else {
            dprintf(INFO, "  reg (len %d) too large for stack buffer\n", len);
        }
    }

    // 2. Parse "available"
    len = g_prom_nodeops->no_proplen(vm_node, "available");
    if (len > 0) {
        if (len <= (int)sizeof(val_buf)) {
            int read_len = g_prom_nodeops->no_getprop(vm_node, "available", (char *)val_buf);
            if (read_len > 0) {
                dprintf(INFO, "  available (len %d):\n", read_len);
                for (int i = 0; i + 12 <= read_len; i += 12) {
                    uint64_t base = 0;
                    for (int j = 0; j < 8; j++) {
                        base = (base << 8) | val_buf[i + j];
                    }
                    uint32_t size = 0;
                    for (int j = 8; j < 12; j++) {
                        size = (size << 8) | val_buf[i + j];
                    }
                    dprintf(INFO, "    base 0x%llx, size 0x%x\n", (unsigned long long)base, size);
                }
            }
        } else {
            dprintf(INFO, "  available (len %d) too large for stack buffer\n", len);
        }
    }

    // 3. Parse "translations"
    len = g_prom_nodeops->no_proplen(vm_node, "translations");
    if (len > 0) {
        if (len <= (int)sizeof(val_buf)) {
            int read_len = g_prom_nodeops->no_getprop(vm_node, "translations", (char *)val_buf);
            if (read_len > 0) {
                dprintf(INFO, "  translations (len %d):\n", read_len);
                for (int i = 0; i + 12 <= read_len; i += 12) {
                    uint32_t virt = 0;
                    for (int j = 0; j < 4; j++) {
                        virt = (virt << 8) | val_buf[i + j];
                    }
                    uint32_t size = 0;
                    for (int j = 4; j < 8; j++) {
                        size = (size << 8) | val_buf[i + j];
                    }
                    uint32_t mode = 0;
                    for (int j = 8; j < 12; j++) {
                        mode = (mode << 8) | val_buf[i + j];
                    }
                    dprintf(INFO, "    virt 0x%x, size 0x%x, mode 0x%x\n", virt, size, mode);
                }
            }
        } else {
            dprintf(INFO, "  translations (len %d) too large for stack buffer\n", len);
        }
    }
}

void dump_prom_mlist(const char *label, struct prom_mlist **list_ptr) {
    if (!list_ptr) {
        dprintf(INFO, "    %s: (null)\n", label);
        return;
    }
    dprintf(INFO, "    %s @ %p:\n", label, list_ptr);
    struct prom_mlist *entry = *list_ptr;
    int count = 0;
    while (entry && count < 32) {
        dprintf(INFO, "      address %p, size 0x%x\n", entry->address, entry->size);
        entry = entry->next;
        count++;
    }
    if (count == 0) {
        dprintf(INFO, "      (empty)\n");
    } else if (entry) {
        dprintf(INFO, "      ...(truncated)\n");
    }
}

// Dumps every field in the romvec that's valid for the PROM version we found,
// since the struct layout grows in place as pv_romvers increases (see the
// prom_romvec comment above).
void dump_prom_romvec(struct prom_romvec *rom) {
    dprintf(INFO, "PROM romvec @ %p:\n", rom);
    dprintf(INFO, "  magic_cookie     0x%x\n", rom->pv_magic_cookie);
    dprintf(INFO, "  romvers          %u\n", rom->pv_romvers);
    dprintf(INFO, "  plugin_revision  %u\n", rom->pv_plugin_revision);
    dprintf(INFO, "  printrev         0x%x\n", rom->pv_printrev);

    if (rom->pv_romvers == 0) {
        dprintf(INFO, "  v0mem:\n");
        dump_prom_mlist("totphys", rom->pv_v0mem.v0_totphys);
        dump_prom_mlist("prommap", rom->pv_v0mem.v0_prommap);
        dump_prom_mlist("available", rom->pv_v0mem.v0_available);
    } else {
        dprintf(INFO, "  v0mem            (unused for OBP v%u, see /memory node instead)\n", rom->pv_romvers);
    }

    dprintf(INFO, "  nodeops          %p\n", rom->pv_nodeops);
    dprintf(INFO, "  bootstr          %p\n", rom->pv_bootstr);

    if (rom->pv_romvers < 2) {
        dprintf(INFO, "  v0devops:\n");
        dprintf(INFO, "    devopen        %p\n", rom->pv_v0devops.v0_devopen);
        dprintf(INFO, "    devclose       %p\n", rom->pv_v0devops.v0_devclose);
        dprintf(INFO, "    rdblkdev       %p\n", rom->pv_v0devops.v0_rdblkdev);
        dprintf(INFO, "    wrblkdev       %p\n", rom->pv_v0devops.v0_wrblkdev);
        dprintf(INFO, "    wrnetdev       %p\n", rom->pv_v0devops.v0_wrnetdev);
        dprintf(INFO, "    rdnetdev       %p\n", rom->pv_v0devops.v0_rdnetdev);
        dprintf(INFO, "    rdchardev      %p\n", rom->pv_v0devops.v0_rdchardev);
        dprintf(INFO, "    wrchardev      %p\n", rom->pv_v0devops.v0_wrchardev);
        dprintf(INFO, "    seekdev        %p\n", rom->pv_v0devops.v0_seekdev);
    } else {
        dprintf(INFO, "  v0devops         (unused for OBP v%u)\n", rom->pv_romvers);
    }

    // NOTE: pv_stdin/pv_stdout/pv_ticks and the v2bootargs sub-pointers are
    // intentionally printed as raw addresses, not followed. Real firmware
    // (e.g. OpenBIOS under QEMU) does not reliably populate every legacy
    // field even when pv_romvers claims support for it, and following a
    // stale/garbage pointer here has caused a data access fault at boot.
    dprintf(INFO, "  stdin            %p\n", rom->pv_stdin);
    dprintf(INFO, "  stdout           %p\n", rom->pv_stdout);
    dprintf(INFO, "  getchar          %p\n", rom->pv_getchar);
    dprintf(INFO, "  putchar          %p\n", rom->pv_putchar);
    dprintf(INFO, "  nbgetchar        %p\n", rom->pv_nbgetchar);
    dprintf(INFO, "  nbputchar        %p\n", rom->pv_nbputchar);
    dprintf(INFO, "  putstr           %p\n", rom->pv_putstr);

    dprintf(INFO, "  reboot           %p\n", rom->pv_reboot);
    dprintf(INFO, "  printf           %p\n", rom->pv_printf);
    dprintf(INFO, "  abort            %p\n", rom->pv_abort);
    dprintf(INFO, "  ticks            %p\n", rom->pv_ticks);
    dprintf(INFO, "  halt             %p\n", rom->pv_halt);
    dprintf(INFO, "  synchook         %p\n", rom->pv_synchook);

    if (rom->pv_romvers < 2) {
        dprintf(INFO, "  fortheval        %p (v0 style, takes len+str)\n", rom->pv_fortheval.v0_eval);
        dprintf(INFO, "  v0bootargs       %p\n", rom->pv_v0bootargs);
        dprintf(INFO, "  enaddr           %p\n", rom->pv_enaddr);
    } else {
        dprintf(INFO, "  fortheval        %p (v2+ style, takes str + 5 int args)\n", rom->pv_fortheval.v2_eval);
    }

    if (rom->pv_romvers >= 2) {
        dprintf(INFO, "  v2bootargs:\n");
        dprintf(INFO, "    bootpath       %p\n", rom->pv_v2bootargs.bootpath);
        dprintf(INFO, "    bootargs       %p\n", rom->pv_v2bootargs.bootargs);
        dprintf(INFO, "    fd_stdin       %p\n", rom->pv_v2bootargs.fd_stdin);
        dprintf(INFO, "    fd_stdout      %p\n", rom->pv_v2bootargs.fd_stdout);

        dprintf(INFO, "  v2devops:\n");
        dprintf(INFO, "    inst2pkg       %p\n", rom->pv_v2devops.v2_inst2pkg);
        dprintf(INFO, "    dumb_mem_alloc %p\n", rom->pv_v2devops.v2_dumb_mem_alloc);
        dprintf(INFO, "    dumb_mem_free  %p\n", rom->pv_v2devops.v2_dumb_mem_free);
        dprintf(INFO, "    dumb_mmap      %p\n", rom->pv_v2devops.v2_dumb_mmap);
        dprintf(INFO, "    dumb_munmap    %p\n", rom->pv_v2devops.v2_dumb_munmap);
        dprintf(INFO, "    dev_open       %p\n", rom->pv_v2devops.v2_dev_open);
        dprintf(INFO, "    dev_close      %p\n", rom->pv_v2devops.v2_dev_close);
        dprintf(INFO, "    dev_read       %p\n", rom->pv_v2devops.v2_dev_read);
        dprintf(INFO, "    dev_write      %p\n", rom->pv_v2devops.v2_dev_write);
        dprintf(INFO, "    dev_seek       %p\n", rom->pv_v2devops.v2_dev_seek);

        dprintf(INFO, "  memalloc         %p\n", rom->pv_memalloc);
        dprintf(INFO, "  setctxt          %p\n", rom->pv_setctxt);
    }

    if (rom->pv_romvers >= 3) {
        dprintf(INFO, "  cpustart         %p\n", rom->pv_cpustart);
        dprintf(INFO, "  cpustop          %p\n", rom->pv_cpustop);
        dprintf(INFO, "  cpuidle          %p\n", rom->pv_cpuidle);
        dprintf(INFO, "  cpuresume        %p\n", rom->pv_cpuresume);
    }
}

} // namespace

void platform_prom_init(void *romvec) {
    struct prom_romvec *rom = static_cast<struct prom_romvec *>(romvec);
    if (rom && rom->pv_magic_cookie == 0x10010407) {
        dprintf(INFO, "Found valid PROM (OpenPROM v%d)\n", rom->pv_romvers);
        dump_prom_romvec(rom);
        g_prom_nodeops = rom->pv_nodeops;
        if (g_prom_nodeops) {
            int root = g_prom_nodeops->no_child(0);
            dprintf(INFO, "Dumping OpenPROM device tree:\n");
            dump_prom_node(root, 0);
            platform_prom_parse_virtual_memory();
        } else {
            dprintf(INFO, "PROM Nodeops is NULL!\n");
        }
    } else {
        dprintf(INFO, "PROM magic cookie invalid or null romvec pointer.\n");
    }
}

status_t platform_prom_initialize_arena(void *romvec, uintptr_t *out_base, size_t *out_size) {
    struct prom_romvec *rom = static_cast<struct prom_romvec *>(romvec);
    if (!rom || rom->pv_magic_cookie != 0x10010407) {
        return ERR_INVALID_ARGS;
    }
    struct prom_nodeops *ops = rom->pv_nodeops;
    if (!ops) {
        return ERR_NOT_FOUND;
    }

    int root = ops->no_child(0);
    int mem_node = prom_find_node_by_name_helper(root, "memory");
    if (mem_node == 0) {
        return ERR_NOT_FOUND;
    }

    // Always use "available" property to avoid overwriting OpenBIOS structures at the top of RAM
    int plen = ops->no_proplen(mem_node, "available");
    if (plen > 0 && plen % 12 == 0) {
        uint8_t avail_buf[12];
        int read_len = ops->no_getprop(mem_node, "available", (char *)avail_buf);
        if (read_len >= 12) {
            uint64_t base = 0;
            for (int i = 0; i < 8; i++) {
                base = (base << 8) | avail_buf[i];
            }
            uint32_t size = 0;
            for (int i = 8; i < 12; i++) {
                size = (size << 8) | avail_buf[i];
            }
            if (size > 0) {
                *out_base = (uintptr_t)base;
                *out_size = (size_t)size;
                return NO_ERROR;
            }
        }
    }

    return ERR_NOT_FOUND;
}

int platform_prom_get_root_node() {
    if (!g_prom_nodeops) {
        return 0;
    }
    return g_prom_nodeops->no_child(0);
}

int platform_prom_find_node_by_name(const char *name) {
    if (!g_prom_nodeops) {
        return 0;
    }
    int root = g_prom_nodeops->no_child(0);
    return prom_find_node_by_name_helper(root, name);
}
