// Coccinelle script to remove unneeded double semicolons.
// Accidental extra semicolons after statements or switch cases are redundant and should be removed.

virtual patch
virtual report
virtual context

// [Pattern Matcher] Find switch default case semicolons
@r_default@
position p;
@@
switch (...)
{
default: ...;@p
}

// [Pattern Matcher] Find switch case semicolons
@r_case@
position p;
@@
(
switch (...)
{
case ...:;@p
}
|
switch (...)
{
case ...:...
case ...:;@p
}
|
switch (...)
{
case ...:...
case ...:
case ...:;@p
}
)

// [Pattern Matcher] Find unneeded extra semicolons after statements
@r1@
statement S;
position p1;
position p != {r_default.p, r_case.p};
identifier label;
@@
(
label:;
|
S@p1;@p
)

// [Filter] Ignore semicolons on different lines from statement end
@script:python@
p << r1.p;
p1 << r1.p1;
@@
if p[0].line != p1[0].line_end:
    cocci.include_match(False)

// [Patch Mode] Action: Remove unneeded extra semicolon
@depends on patch@
position r1.p;
@@
-;@p

// [Report Mode] Output Formatter: Print warning for r1 to console
@script:python depends on report@
p << r1.p;
@@
coccilib.report.print_report(p[0], "WARNING: Unneeded semicolon")

// [Context Mode] Pattern Matcher: Highlight unneeded extra semicolon
@depends on context@
position r1.p;
@@
*;@p
