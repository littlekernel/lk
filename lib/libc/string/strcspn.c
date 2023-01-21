
/*
** Copyright 2004, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
/*
 * Copyright (c) 2008 Travis Geiselbrecht
 *
 * Use of this source code is governed by a MIT-style
 * license that can be found in the LICENSE file or at
 * https://opensource.org/licenses/MIT
 */
#include <string.h>
#include <sys/types.h>

size_t
strcspn (const char *string, const char *reject)
{
  size_t count = 0;
  while (strchr (reject, *string) == 0)
    {
      ++count, ++string;
    }

  return count;
}
