/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
/*
 * All unit tests get registered here.  A call to run_all_tests() will run
 * them and provide results.
 */
#include <lib/unittest.h>

#include <assert.h>
#include <lk/console_cmd.h>
#include <lk/err.h>
#include <lk/init.h>

static struct list_node test_case_list = LIST_INITIAL_VALUE(test_case_list);

#if WITH_LIB_CONSOLE
#include <lib/console.h>

#if RUN_UNITTESTS_AT_BOOT
static void unittest_run_at_boot(uint level) {
    const char run_at_boot_script[] = "sleep 5; ut all";
    console_t *con = console_create(false);
    if (con) {
        console_run_script(con, run_at_boot_script);
        // TODO: destroy console afterwards
    }
}

LK_INIT_HOOK(unittest_at_boot, unittest_run_at_boot, LK_INIT_LEVEL_LAST)
#endif
#endif

/*
 * Registers a test case with the unit test framework.
 */
void unittest_register_test_case(struct test_case_element *elem) {
    DEBUG_ASSERT(elem);
    DEBUG_ASSERT(!list_in_list(&elem->node));
    list_add_tail(&test_case_list, &elem->node);
}

/*
 * Runs all registered test cases.
 */
bool run_all_tests(void) {
    unsigned int n_tests = 0;
    unsigned int n_success = 0;
    unsigned int n_failed = 0;
    struct list_node failed_test_case_list = LIST_INITIAL_VALUE(failed_test_case_list);

    bool all_success = true;
    struct test_case_element *current;
    list_for_every_entry(&test_case_list, current, struct test_case_element, node) {
        if (!current->test_case()) {
            list_add_tail(&failed_test_case_list, &current->failed_node);
            all_success = false;
            n_failed++;
        } else {
            n_success++;
        }
        n_tests++;
    }

    unittest_printf("\n====================================================\n");
    unittest_printf("    CASES:  %d     SUCCESS:  %d     FAILED:  %d   ",
                    n_tests, n_success, n_failed);
    unittest_printf("\n====================================================\n");

    if (all_success) {
        unittest_printf("\nSUCCESS! All test cases passed\n");
    } else {
        unittest_printf("\nFAILURE! Some test cases failed:\n");
        struct test_case_element *failed;
        struct test_case_element *temp;
        list_for_every_entry_safe(&failed_test_case_list, failed, temp, struct test_case_element, failed_node) {
            unittest_printf("    %s\n", failed->name);
            list_delete(&failed->failed_node);
        }
        unittest_printf("\n");
    }

    return all_success;
}

static int do_unittests(int argc, const console_cmd_args *argv) {

    if (argc < 2) {
usage:
        printf("usage:\n");
        printf("%s all          : run all unit tests\n", argv[0].str);
        printf("%s list         : list all test cases\n", argv[0].str);
        printf("%s <test name>  : run specific test\n", argv[0].str);
        return -1;
    }

    if (!strcmp(argv[1].str, "all")) {
        bool result = run_all_tests();
        printf("UNIT TEST: run_all_tests return %u\n", result);
    } else if (!strcmp(argv[1].str, "list")) {
        struct test_case_element *current;
        list_for_every_entry(&test_case_list, current, struct test_case_element, node) {
            puts(current->name);
        }
    } else {
        // look for unit test that matches the string name
        bool found_test = false;
        struct test_case_element *current;
        list_for_every_entry(&test_case_list, current, struct test_case_element, node) {
            if (strcmp(argv[1].str, current->name) == 0) {
                found_test = true;
                current->test_case();
                break;
            }
        }

        if (!found_test) {
            goto usage;
        }
    }

    return NO_ERROR;
}

STATIC_COMMAND_START
STATIC_COMMAND("ut", "run some or all of the unit tests", do_unittests)
STATIC_COMMAND_END(unit_tests);
