/*
 * Copyright (C) 2024 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

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