/*
 * Copyright 2016 Google Inc. All Rights Reserved.
 * Author: gkalsi@google.com (Gurjant Kalsi)
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

// The platform/target should implement this routine by mouting the default
// filesystem and returning a string that points to the mount point. If NULL is
// returned it is assumed that either (1) the platform does not implement FS
// boot or that (2) mounting the default filesystem failed in which case the
// system proceeds to boot without FSBoot.
void attempt_fs_boot(void);
