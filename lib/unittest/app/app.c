/*
 * Copyright (c) 2013, Google, Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <lib/unittest.h>

#include <app.h>

/* simple app to run all of the unittests at boot */
void unittest_entry(const struct app_descriptor *app, void *args) {
    run_all_tests();
}

APP_START(unittest)
.entry = unittest_entry,
APP_END
