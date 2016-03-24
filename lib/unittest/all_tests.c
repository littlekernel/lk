/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
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
/*
 * All unit tests get registered here.  A call to run_all_tests() will run
 * them and provide results.
 */
#include <unittest.h>
#include <assert.h>

static struct test_case_element *test_case_list = NULL;
static struct test_case_element *failed_test_case_list = NULL;

/*
 * Registers a test case with the unit test framework.
 */
void unittest_register_test_case(struct test_case_element *elem)
{
    DEBUG_ASSERT(elem);
    DEBUG_ASSERT(elem->next == NULL);
    elem->next = test_case_list;
    test_case_list = elem;
}

/*
 * Runs all registered test cases.
 */
bool run_all_tests(void)
{
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
