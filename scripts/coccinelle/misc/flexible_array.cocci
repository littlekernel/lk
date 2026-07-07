// Coccinelle script to detect zero-length and one-element arrays at the end of structures.
// Zero-length and one-element arrays are deprecated; standard C99 flexible-array members should be used instead.

virtual context
virtual report
virtual patch

// [Context/Report Mode] Pattern Matcher: Find zero-length and one-element arrays at structure ends
@r depends on !patch@
identifier name, array;
type T;
position p;
@@

(
  struct name {
    ...
*   T array@p[\(0\|1\)];
  };
|
  struct {
    ...
*   T array@p[\(0\|1\)];
  };
|
  union name {
    ...
*   T array@p[\(0\|1\)];
  };
|
  union {
    ...
*   T array@p[\(0\|1\)];
  };
)

// [Filter] Exclude structures where array is the only member
@only_field depends on patch@
identifier name, array;
type T;
position q;
@@

(
  struct name {@q
    T array[\(0\|1\)];
  };
|
  struct {@q
    T array[\(0\|1\)];
  };
)

// [Patch Mode] Action: Remove zero-length or one-element specification in flexible-array member
@depends on patch@
identifier name, array;
type T;
position p;
position q != only_field.q;
@@

(
  struct name {@q
    ...
    T array@p[
-       0
    ];
  };
|
  struct name {@q
    ...
    T array@p[
-       1
    ];
  };
|
  struct {@q
    ...
    T array@p[
-       0
    ];
  };
|
  struct {@q
    ...
    T array@p[
-       1
    ];
  };
)

// [Report Mode] Output Formatter: Print warning for r to console
@script:python depends on report@
p << r.p;
@@

msg = "WARNING: use C99 flexible-array member instead of zero-length or one-element array"
coccilib.report.print_report(p[0], msg)
