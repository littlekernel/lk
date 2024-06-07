#ifndef __DEFER_HEADER_
#define __DEFER_HEADER_

// Macro for running a block of code before function exits.
// Example:
// DEFER {
//     fclose(hc);
//     hc = nullptr;
//   };
// It works by creating a new local variable struct holding the lambda, the
// destructor of that struct will invoke the lambda.

// ScopeGuard ensures that the specified functor is executed no matter how the
// current scope exits.
template <typename F> class ScopeGuard {
public:
  constexpr ScopeGuard(F &&f) : f_(static_cast<F &&>(f)) {}
  constexpr ScopeGuard(ScopeGuard &&that) noexcept
      : f_(that.f_), active_(that.active_) {
    that.active_ = false;
  }

  template <typename Functor>
  constexpr ScopeGuard(ScopeGuard<Functor> &&that)
      : f_(that.f_), active_(that.active_) {
    that.active_ = false;
  }

  ~ScopeGuard() { f_(); }

  ScopeGuard() = delete;
  ScopeGuard(const ScopeGuard &) = delete;
  void operator=(const ScopeGuard &) = delete;
  void operator=(ScopeGuard &&that) = delete;

private:
  template <typename Functor> friend class ScopeGuard;
  F f_;
  bool active_ = true;
};

constexpr struct {
  template <typename F> constexpr auto operator<<(F &&f) const noexcept {
    return ScopeGuard<F>(static_cast<F &&>(f));
  }
} deferrer;

#define TOKENPASTE1(x, y) x##y
#define TOKENPASTE2(x, y) TOKENPASTE1(x, y)
#define DEFER                                                                  \
  auto TOKENPASTE2(_deferred_lambda_call, __COUNTER__) = deferrer              \
                                                         << [&]() mutable

#endif