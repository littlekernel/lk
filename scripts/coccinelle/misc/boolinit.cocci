// Coccinelle script to use true/false instead of 0/1 for bool initializers.
// stdbool.h's true/false communicate intent more clearly than bare integer literals.

virtual patch
virtual context
virtual report

// [Pattern Matcher] Find bool declared with a 0 or 1 literal initializer
@r@
type T;
identifier b;
constant c;
position p;
@@

T b = c@p;

// [Filter] Restrict declaration type to bool and literal to 0 or 1
@script:python depends on r@
t << r.T;
c << r.c;
@@

if t not in ("bool", "_Bool") or c not in ("0", "1"):
    cocci.include_match(False)

// [Patch Mode] Action: Replace the 0/1 literal with false/true
@depends on patch@
position r.p;
@@
(
- 0@p
+ false
|
- 1@p
+ true
)

// [Context Mode] Pattern Matcher: Highlight the 0/1 literal initializer
@depends on context@
position r.p;
@@
(
* 0@p
|
* 1@p
)

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p << r.p;
c << r.c;
@@

repl = "true" if c == "1" else "false"
msg = "WARNING: use '%s' instead of '%s' for bool initializer" % (repl, c)
coccilib.report.print_report(p[0], msg)
