/*
 * Copyright (c) 2014 Brian Swetland
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */

#pragma once

// connect to host, issue cmd:args, and if txfd != -1, send the contents
// of that file as the command payload
int lkboot_txn(const char *host, const char *cmd, int txfd, const char *args);

// return number of bytes of data the last txn resulted in and if nonzero
// set *ptr = the buffer (which remains valid until next lkboot_txn())
unsigned lkboot_get_reply(void **ptr);
