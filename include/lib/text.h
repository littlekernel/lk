#ifndef __LIB_TEXT_H
#define __LIB_TEXT_H

#include <lib/font.h>

/* super cheezy mechanism to stick lines of text up on top of whatever is being drawn */
/* XXX replace with something more generic later */
void text_draw(int x, int y, const char *string);

/* super dumb, someone has to call this to refresh everything */
void text_update(void);

#endif

