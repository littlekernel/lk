// Coccinelle script to detect free() called on the address of an expression.
// free() must be given a pointer previously returned by an allocator; passing the
// address of a local variable or struct member is a bug, not a valid pointer to free.

virtual patch
virtual context
virtual report

// [Pattern Matcher] Find free() applied to an address-of expression
@r@
expression E;
position p;
@@

* free@p(&E)

// [Report Mode] Output Formatter: Print error to console
@script:python depends on report@
p << r.p;
@@

msg = "ERROR: free() called on the address of an expression, not an allocated pointer"
coccilib.report.print_report(p[0], msg)
