// Coccinelle script to simplify "T ret; ret = e; return ret;" into "return e;"
// when the local variable exists solely to carry the return value.
//
// The assignment and the return must be immediately adjacent: anything sitting
// between them (e.g. a barrier like DSB) executes relative to the side effects
// of evaluating e, so skipping over it would silently reorder that statement
// to happen before e is evaluated instead of after. Only the declaration may
// have unrelated statements between it and the assignment, since dropping an
// inert declaration can never reorder anything.

virtual patch
virtual context
virtual report

@r@
identifier ret, L;
type T;
expression e;
position p1, p2;
@@

  T ret@p1;
  ... when != ret
      when != goto L;
  ret = e;
  return@p2 ret;

// [Patch Mode] Action: Drop the local variable and return the expression directly
@depends on patch@
identifier r.ret, r.L;
type r.T;
expression r.e;
position r.p1, r.p2;
@@

- T ret@p1;
  ... when != ret
      when != goto L;
- ret = e;
- return@p2 ret;
+ return e;

// [Context Mode] Pattern Matcher: Highlight the redundant local variable usage
@depends on context@
identifier r.ret, r.L;
type r.T;
expression r.e;
position r.p1, r.p2;
@@

* T ret@p1;
  ... when != ret
      when != goto L;
* ret = e;
* return@p2 ret;

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p1 << r.p1;
e << r.e;
@@

msg = "WARNING: local variable only used to store return value; use 'return %s;'" % (e)
coccilib.report.print_report(p1[0], msg)
