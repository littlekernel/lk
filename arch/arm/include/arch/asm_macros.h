/*
 * Copyright (c) 2015, Google Inc. All rights reserved
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#pragma once

/* Set fault handler for next instruction */
.macro set_fault_handler, handler
.Lfault_location\@:
.pushsection .rodata.fault_handler_table
.long    .Lfault_location\@
.long    \handler
.popsection
.endm
