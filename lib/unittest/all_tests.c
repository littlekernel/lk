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

static struct test_case_element *test_case_list = NULL;
static struct test_case_element *failed_test_case_list = NULL;

/*
 * Registers a test case with the unit test framework.
 */
void unittest_register_test_case(struct test_case_element *elem) {
    DEBUG_ASSERT(elem);
    DEBUG_ASSERT(elem->next == NULL);
    elem->next = test_case_list;
    test_case_list = elem;
}

/*
 * Runs all registered test cases.
 */
bool run_all_tests(void) {
    unsigned int n_tests   = 0;
    unsigned int n_success = 0;
    unsigned int n_failed  = 0;

    bool all_success = true;
    struct test_case_element *current = test_case_list;
    while (current) {
        if (!current->test_case()) {
            current->failed_next = failed_test_case_list;
            failed_test_case_list = current;
            all_success = false;
        }
        current = current->next;
        n_tests++;
    }

    if (all_success) {
        n_success = n_tests;
        unittest_printf("SUCCESS!  All test cases passed!\n");
    } else {
        struct test_case_element *failed = failed_test_case_list;
        while (failed) {
            struct test_case_element *failed_next =
                    failed->failed_next;
            failed->failed_next = NULL;
            failed = failed_next;
            n_failed++;
        }
        n_success = n_tests - n_failed;
        failed_test_case_list = NULL;
    }

    unittest_printf("\n====================================================\n");
    unittest_printf  ("    CASES:  %d     SUCCESS:  %d     FAILED:  %d   ",
                      n_tests, n_success, n_failed);
    unittest_printf("\n====================================================\n");

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
        for (struct test_case_element *current = test_case_list; current; current = current->next) {
            puts(current->name);
        }
    } else {
        // look for unit test that matches the string name
        bool found_test = false;
        for (struct test_case_element *current = test_case_list; current; current = current->next) {
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

