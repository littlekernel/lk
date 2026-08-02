// Coccinelle script to simplify "T ret; ret = e; return ret;" into "return e;"
// when the local variable exists solely to carry the return value.

virtual patch
virtual context
virtual report

// [Pattern Matcher] Find a single-use local variable assigned once and returned
// on the same straight-line path (no intervening goto/label to avoid crossing
// into unrelated control-flow branches).
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
  ... when != ret
      when != goto L;
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
  ... when != ret
      when != goto L;
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
  ... when != ret
      when != goto L;
* return@p2 ret;

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p1 << r.p1;
e << r.e;
@@

msg = "WARNING: local variable only used to store return value; use 'return %s;'" % (e)
coccilib.report.print_report(p1[0], msg)
