/*
 * Copyright (c) 2014 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <app/tests.h>
#include <stdio.h>
#include <arch/mmu.h>
#include <err.h>
#include <debug.h>
#include <arch/x86/mmu.h>
#include <err.h>

#define test_value1 (0xdeadbeef)
#define test_value2 (0xdeadcafe)

/* testing the ARCH independent map & query routine */
int arch_mmu_map_test(vaddr_t vaddr, paddr_t paddr, uint count, uint map_flags, uint skip_unmap)
{
	vaddr_t *test_vaddr_first, *test_vaddr_last;
	paddr_t out_paddr;
	uint ret_flags;
	status_t ret;

	/* Do the Mapping */
	ret = arch_mmu_map(vaddr, paddr, count, map_flags);
	if(ret)
		return ret;
	printf("\nAdd Mapping => Vaddr:%llx Paddr:%llx pages=%d flags:%llx\n", vaddr, paddr, count, map_flags);

	if(count > 0) {
		/* Check the mapping */
		ret = arch_mmu_query((vaddr_t)vaddr, &out_paddr, &ret_flags);
		if(ret)
			return ret;

		if(out_paddr != paddr)
			return 1;

		if(map_flags != ret_flags)
			return 2;

		printf("\nQuery of the existing mapping - successfull (ret Paddr:%llx) (ret Flags:%llx)\n", out_paddr, ret_flags);

		/* Write and Read test */
		if (ret_flags & X86_MMU_PG_RW) {
			/* first page */
			test_vaddr_first = (vaddr_t *)vaddr;
			*test_vaddr_first = test_value1;
			printf("\nReading MAPPED addr => Vaddr:%llx Value=%llx\n", test_vaddr_first, *test_vaddr_first);

			/* last page */
			test_vaddr_last = (vaddr_t *)(vaddr + ((count-1)*PAGE_SIZE));
			*test_vaddr_last = test_value2;
			printf("\nReading MAPPED addr => Vaddr:%llx Value=%llx\n", test_vaddr_last, *test_vaddr_last);
		}
		else
			printf("\n Can't write onto these addresses (NO RW permission) - Will cause FAULT\n");
	}

	if (skip_unmap) {
		/* Unmap */
		ret = arch_mmu_unmap((vaddr_t)vaddr, count);
		if(ret != (int)count)
			return 3;

		printf("\nRemove Mapping => Vaddr:%llx pages=%d\n", vaddr, count);

		/* Check the mapping again - mappnig should NOT be found now */
		ret = arch_mmu_query((vaddr_t)vaddr, &out_paddr, &ret_flags);
		if(ret != ERR_NOT_FOUND)
			return 4;
	}

	return NO_ERROR;
}


int x86_mmu_tests(void)
{
	int return_status;

	/* Test Case # 1 */
	return_status = arch_mmu_map_test((0x17efe000),(0x17efe000),512, X86_MMU_PG_RW | X86_MMU_PG_P, 0);
	if(!return_status)
		printf("\n\n---- x86 MMU Test result:SUCCESS ----\n\n");
	else
		printf("\n\n ----- x86 MMU Test result:FAILURE (Return status:%d) ----\n", return_status);

	/* Test Case # 2 */
	return_status = arch_mmu_map_test((0x17efe000),(0x17efe000), 0, X86_MMU_PG_RW | X86_MMU_PG_P, 0);
	if(!return_status)
		printf("\n\n ----- x86 MMU Test result:SUCCESS ---- \n\n");
	else
		printf("\n\n ----- x86 MMU Test result:FAILURE (Return status:%d) ----\n", return_status);

	/* Test Case # 3 */
	return_status = arch_mmu_map_test((0x7fffe0000),(0x17ffe000), 256, X86_MMU_PG_RW | X86_MMU_PG_P, 0);
	if(!return_status)
		printf("\n\n ----- x86 MMU Test result:SUCCESS ---- \n\n");
	else
		printf("\n\n ----- x86 MMU Test result:FAILURE (Return status:%d) ----\n", return_status);

	/* Test case # 4 */
	return_status = arch_mmu_map_test((0x7fffe0000),(0x17ffe000), 1024, X86_MMU_PG_P, 1);
	if(!return_status)
		printf("\n\n ----- x86 MMU Test result:SUCCESS ---- \n\n");
	else
		printf("\n\n ----- x86 MMU Test result:FAILURE (Return status:%d) ----\n", return_status);

	return_status = arch_mmu_map_test((0x7fffe0000),(0x17ffe000), 1024, X86_MMU_PG_RW | X86_MMU_PG_P, 0);
        if(!return_status)
		printf("\n\n ----- x86 MMU Test result:SUCCESS ---- \n\n");
	else
		printf("\n\n ----- x86 MMU Test result:FAILURE (Return status:%d) ----\n", return_status);

	return 0;
}
