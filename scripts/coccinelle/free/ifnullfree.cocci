// Coccinelle script to remove redundant NULL checks before free().
// free(NULL) is safe, so "if (x) free(x);" can be simplified to "free(x);".

virtual patch
virtual context
virtual report

// [Pattern Matcher] Find redundant NULL checks
@r@
expression x;
position p;
@@

if (x)@p
    free(x);

// [Patch Mode] Action: Remove the redundant 'if' check
@depends on patch@
expression x;
position r.p;
@@

- if (x)@p
    free(x);

// [Context Mode] Pattern Matcher: Highlight redundant NULL check
@depends on context@
expression x;
position r.p;
@@

* if (x)@p
    free(x);

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p << r.p;
@@

msg = "WARNING: redundant NULL check before free"
coccilib.report.print_report(p[0], msg)
