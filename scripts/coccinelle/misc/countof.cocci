// Coccinelle script to use countof macro instead of opencoded division.
// Replace division of sizeof array by sizeof element with countof(E).

virtual patch
virtual context
virtual report

// [Filter] Include LK compiler header
@i@
@@

#include <lk/compiler.h>

// [Context Mode] Pattern Matcher: Find division of sizeof array by sizeof element
@depends on context@
type T;
T[] E;
@@
(
* (sizeof(E)/sizeof(*E))
|
* (sizeof(E)/sizeof(E[...]))
|
* (sizeof(E)/sizeof(T))
)

// [Patch Mode] Action: Replace opencoded array size division with countof(E)
@depends on patch@
type T;
T[] E;
@@
(
- (sizeof(E)/sizeof(*E))
+ countof(E)
|
- (sizeof(E)/sizeof(E[...]))
+ countof(E)
|
- (sizeof(E)/sizeof(T))
+ countof(E)
)

// [Report Mode] Pattern Matcher: Find division of sizeof array by sizeof element
@r depends on report@
type T;
T[] E;
position p;
@@
(
 (sizeof(E)@p /sizeof(*E))
|
 (sizeof(E)@p /sizeof(E[...]))
|
 (sizeof(E)@p /sizeof(T))
)

// [Report Mode] Output Formatter: Print warning for r to console
@script:python depends on report@
p << r.p;
@@

msg = "WARNING: Use countof"
coccilib.report.print_report(p[0], msg)
