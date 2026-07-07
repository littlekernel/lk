// Coccinelle script to use strdup() instead of manual memory allocation and string copy.
// strdup(src) is equivalent to malloc(strlen(src) + 1) followed by strcpy/memcpy/strlcpy.

virtual patch
virtual context
virtual report

// [Filter] Exclude strdup and strndup implementation functions
@ignore@
identifier fn = {strdup, strndup};
position p;
@@

fn(...) {
  ...
(
  malloc@p(...)
|
  calloc@p(...)
)
  ...
}

// [Patch Mode] Action: Replace manual allocation and copy (without length variable) with strdup()
@depends on patch@
expression from, to;
expression E1, E2;
statement S;
position p != ignore.p;
@@

(
-  to = malloc@p(strlen(from) + 1);
+  to = strdup(from);
|
-  to = calloc@p(1, strlen(from) + 1);
+  to = strdup(from);
|
-  to = calloc@p(strlen(from) + 1, 1);
+  to = strdup(from);
)
   ... when != \(from = E1 \| to = E1 \)
   if (to == NULL || ...) S
   ... when != \(from = E2 \| to = E2 \)
(
-  strcpy(to, from);
|
-  strlcpy(to, from, strlen(from) + 1);
|
-  memcpy(to, from, strlen(from) + 1);
)

// [Patch Mode] Action: Replace manual allocation and copy (with length variable) with strdup()
@depends on patch@
expression x, from, to;
expression E1, E2, E3;
statement S;
position p != ignore.p;
@@

-   x = strlen(from) + 1;
    ... when != \( x = E1 \| from = E1 \)
(
-   to = malloc@p(x);
+   to = strdup(from);
|
-   to = calloc@p(1, x);
+   to = strdup(from);
|
-   to = calloc@p(x, 1);
+   to = strdup(from);
)
    ... when != \(x = E2 \| from = E2 \| to = E2 \)
    if (to == NULL || ...) S
    ... when != \(x = E3 \| from = E3 \| to = E3 \)
(
-   memcpy(to, from, x);
|
-   strlcpy(to, from, x);
|
-   strcpy(to, from);
)

// [Context/Report Mode] Pattern Matcher: Find manual allocation and copy without length variable
@r1 depends on !patch exists@
expression from, to;
expression E1, E2;
statement S;
position p1 != ignore.p, p2;
@@

(
*  to = malloc@p1(strlen(from) + 1);
|
*  to = calloc@p1(1, strlen(from) + 1);
|
*  to = calloc@p1(strlen(from) + 1, 1);
)
   ... when != \(from = E1 \| to = E1 \)
   if (to == NULL || ...) S
   ... when != \(from = E2 \| to = E2 \)
(
*  strcpy@p2(to, from);
|
*  strlcpy@p2(to, from, strlen(from) + 1);
|
*  memcpy@p2(to, from, strlen(from) + 1);
)

// [Context/Report Mode] Pattern Matcher: Find manual allocation and copy with length variable
@r2 depends on !patch exists@
expression x, from, to;
expression E1, E2, E3;
statement S;
position p1 != ignore.p, p2;
@@

*   x = strlen(from) + 1;
    ... when != \( x = E1 \| from = E1 \)
(
*   to = malloc@p1(x);
|
*   to = calloc@p1(1, x);
|
*   to = calloc@p1(x, 1);
)
    ... when != \(x = E2 \| from = E2 \| to = E2 \)
    if (to == NULL || ...) S
    ... when != \(x = E3 \| from = E3 \| to = E3 \)
(
*   memcpy@p2(to, from, x);
|
*   strlcpy@p2(to, from, x);
|
*   strcpy@p2(to, from);
)

// [Report Mode] Output Formatter: Print warning for r1 to console
@script:python depends on report@
p1 << r1.p1;
p2 << r1.p2;
@@

msg = "WARNING opportunity for strdup (copy on line %s)" % (p2[0].line)
coccilib.report.print_report(p1[0], msg)

// [Report Mode] Output Formatter: Print warning for r2 to console
@script:python depends on report@
p1 << r2.p1;
p2 << r2.p2;
@@

msg = "WARNING opportunity for strdup (copy on line %s)" % (p2[0].line)
coccilib.report.print_report(p1[0], msg)