// Coccinelle script to remove redundant casts from memory allocation functions.
// In C, void * is implicitly convertible to any pointer type, so casting the return value of malloc/calloc/realloc/memalign is redundant.

virtual patch
virtual context
virtual report

// [Pattern Matcher] Find explicit casts on memory allocation functions
@r1@
type T;
position p;
@@

  (T *)
  \(malloc@p\|calloc@p\|realloc@p\|memalign@p\)(...)

// [Filter] Ignore C++ files where void* cast is mandatory
@script:python@
p << r1.p;
@@

if p[0].file.endswith(('.cpp', '.cc', '.cxx', '.hpp', '.hh', '.C', '.H')):
    cocci.include_match(False)

// [Patch Mode] Action: Remove redundant cast from memory allocation functions
@depends on patch@
type T;
position r1.p;
@@

- (T *)
  \(malloc@p\|calloc@p\|realloc@p\|memalign@p\)(...)

// [Report Mode] Output Formatter: Print warning to console
@script:python depends on report@
p << r1.p;
t << r1.T;
@@

msg = "WARNING: casting value returned by memory allocation function to (%s *) is useless in C." % (t)
coccilib.report.print_report(p[0], msg)
