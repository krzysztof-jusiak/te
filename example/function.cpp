//
// Copyright (c) 2018 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <cassert>

#include "te.hpp"

template <class>
struct Function;

template <class R, class... Ts>
struct Function<R(Ts...)> {
  constexpr inline R operator()(Ts... args) {
    return te::call<R>([](auto &self, Ts... args) { return self(args...); },
                       *this, args...);
  }
};

template struct Function<int(int)>;

int main() {
  te::poly<Function<int(int)>> f{[](int i) { return i; }};
  assert(42 == f(42));
}
