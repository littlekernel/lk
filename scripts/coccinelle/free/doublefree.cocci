// Coccinelle script to detect a pointer freed twice without an intervening reassignment.
// Calling free() a second time on the same, unmodified pointer is undefined behavior.

virtual patch
virtual context
virtual report

// [Pattern Matcher] Find free(x) followed by another free(x) with no reassignment of x
@r@
expression x, E;
position p1, p2;
@@

free@p1(x);
... when != x = E
    when != free(&x)
free@p2(x);

// [Patch Mode] Action: Drop the redundant second free() call
@depends on patch@
expression r.x;
position r.p2;
@@

- free@p2(x);

// [Context Mode] Pattern Matcher: Highlight the redundant second free() call
@depends on context@
expression r.x;
position r.p2;
@@

* free@p2(x);

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p1 << r.p1;
p2 << r.p2;
@@

msg = "WARNING: possible double free, already freed on line %s" % (p1[0].line)
coccilib.report.print_report(p2[0], msg)
