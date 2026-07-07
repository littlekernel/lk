// Coccinelle script to remove redundant NULL checks before free().
// free(NULL) is safe, so "if (x) free(x);" can be simplified to "free(x);".

virtual patch
virtual context
virtual report

// [Context/Patch Mode] Pattern Matcher: Find redundant NULL checks
@r depends on context || patch@
expression x;
position p;
@@

* if (x)@p
      free(x);

// [Patch Mode] Action: Remove the redundant 'if' check
@depends on patch && r@
expression x;
@@

- if (x)
      free(x);

// [Report Mode] Pattern Matcher: Find redundant NULL checks
@r2 depends on report@
expression x;
position p;
@@

 if (x)@p
      free(x);

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p << r2.p;
@@

msg = "WARNING: redundant NULL check before free"
coccilib.report.print_report(p[0], msg)
