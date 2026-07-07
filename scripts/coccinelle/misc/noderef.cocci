// Coccinelle script to detect sizeof applied to pointer expressions.
// Applying sizeof to a pointer expression x gives the size of the pointer, not the dereferenced size *x.

virtual report
virtual context
virtual patch

// [Patch Mode] Action: Replace sizeof(x) with sizeof(*x) for pointer expressions
@depends on patch@
expression *x;
expression f;
expression i;
type T;
@@

(
x = <+... sizeof(
- x
+ *x
   ) ...+>
|
f(...,(T)(x),...,sizeof(
- x
+ *x
   ),...)
|
f(...,sizeof(
- x
+ *x
   ),...,(T)(x),...)
|
f(...,(T)(x),...,i*sizeof(
- x
+ *x
   ),...)
|
f(...,i*sizeof(
- x
+ *x
   ),...,(T)(x),...)
)

// [Context/Report Mode] Pattern Matcher: Find sizeof applied to pointer expressions
@r depends on !patch@
expression *x;
expression f;
expression i;
position p;
type T;
@@

(
*x = <+... sizeof@p(x) ...+>
|
*f(...,(T)(x),...,sizeof@p(x),...)
|
*f(...,sizeof@p(x),...,(T)(x),...)
|
*f(...,(T)(x),...,i*sizeof@p(x),...)
|
*f(...,i*sizeof@p(x),...,(T)(x),...)
)

// [Report Mode] Output Formatter: Print error for r to console
@script:python depends on report@
p << r.p;
@@

msg = "ERROR: application of sizeof to pointer"
coccilib.report.print_report(p[0], msg)
