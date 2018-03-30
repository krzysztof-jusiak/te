//
// Copyright (c) 2018 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#include <sstream>
#include <type_traits>
#include <vector>

#include "common/test.hpp"
#include "te.hpp"

struct Drawable {
  void draw(std::ostream &out) const {
    te::call([](auto const &self, auto &out) { self.draw(out); }, *this, out);
  }
};

struct Square {
  void draw(std::ostream &out) const { out << "Square"; }
};

struct Circle {
  void draw(std::ostream &out) const { out << "Circle"; }
};

struct Triangle {
  void draw(std::ostream &out) const { out << "Triangle"; }
};

test should_erase_the_call = [] {
  te::poly<Drawable> drawable{Square{}};

  {
    std::stringstream str{};
    drawable.draw(str);
    expect("Square" == str.str());
  }

  {
    std::stringstream str{};
    drawable = Circle{};
    drawable.draw(str);
    expect("Circle" == str.str());
  }
};

test should_reassign = [] {
  te::poly<Drawable> drawable{Circle{}};
  drawable = Square{};

  {
    std::stringstream str{};
    drawable.draw(str);
    expect("Square" == str.str());
  }
};

test should_support_containers = [] {
  std::vector<te::poly<Drawable>> drawables{};

  drawables.push_back(Square{});
  drawables.push_back(Circle{});
  drawables.push_back(Triangle{});

  std::stringstream str{};
  for (const auto &drawable : drawables) {
    drawable.draw(str);
  }
  expect("SquareCircleTriangle" == str.str());
};

struct Addable {
  auto add(int i) {
    return te::call<int>(
        [](auto &self, auto... args) { return self.add(args...); }, *this, i);
  }
  auto add(int a, int b) {
    return te::call<int>(
        [](auto &self, auto... args) { return self.add(args...); }, *this, a,
        b);
  }
};

class Calc {
 public:
  constexpr auto add(int i) { return i; }
  constexpr auto add(int a, int b) { return a + b; }
};

test should_support_overloads = [] {
  te::poly<Addable> addable{Calc{}};
  expect(3 == addable.add(3));
  expect(3 == addable.add(1, 2));
};

namespace v1 {
struct Drawable {
  void draw(std::ostream &out) const {
    te::call([](auto const &self, auto &&... args) { self.draw(args...); },
             *this, out, "v1");
  }
};
}  // namespace v1

namespace v2 {
struct Drawable : v1::Drawable {
  Drawable() { te::extends<v1::Drawable>(*this); }
};
}  // namespace v2

namespace v3 {
struct Drawable : v2::Drawable {
  Drawable() { te::extends<v2::Drawable>(*this); }

  void draw(std::ostream &out) const {
    te::call([](auto const &self, auto &&... args) { self.draw(args...); },
             *this, out, "v3");
  }
};
}  // namespace v3

test should_support_overrides = [] {
  struct Square {
    void draw(std::ostream &out, const std::string &v) const {
      out << v << "::Square ";
    }
  };

  struct Circle {
    void draw(std::ostream &out, const std::string &v) const {
      out << v << "::Circle ";
    }
  };

  const auto draw = [](auto const &drawable, auto &str) { drawable.draw(str); };

  {
    std::stringstream str{};
    draw(te::poly<v1::Drawable>{Circle{}}, str);
    draw(te::poly<v1::Drawable>{Square{}}, str);
    expect("v1::Circle v1::Square " == str.str());
  }

  {
    std::stringstream str{};
    draw(te::poly<v2::Drawable>{Circle{}}, str);
    draw(te::poly<v2::Drawable>{Square{}}, str);
    expect("v1::Circle v1::Square " == str.str());
  }

  {
    std::stringstream str{};
    draw(te::poly<v3::Drawable>{Circle{}}, str);
    draw(te::poly<v3::Drawable>{Square{}}, str);
    expect("v3::Circle v3::Square " == str.str());
  }
};

template <class T>
struct DrawableT {
  void draw(T &out) const {
    te::call([](auto const &self, T &out) { self.draw(out); }, *this, out);
  }
};

template struct DrawableT<std::ostream>;
template struct DrawableT<std::stringstream>;

test should_support_templated_interfaces = [] {
  {
    std::stringstream str{};
    te::poly<DrawableT<std::ostream>> drawable{Square{}};
    drawable.draw(str);
    expect("Square" == str.str());
  }

  {
    std::stringstream str{};
    te::poly<DrawableT<std::stringstream>> drawable{Circle{}};
    drawable.draw(str);
    expect("Circle" == str.str());
  }
};

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
template struct Function<int(int, int)>;

test should_support_function_lambda_expr = [] {
  {
    te::poly<Function<int(int)>> f{[](int i) { return i; }};
    expect(0 == f(0));
    expect(42 == f(42));
  }

  {
    te::poly<Function<int(int, int)>> f{[](int a, int b) { return a + b; }};
    expect(0 == f(0, 0));
    expect(3 == f(1, 2));
  }
};

class Ctor;
class CopyCtor;
class MoveCtor;
class Dtor;

struct Storage {
  template <class T>
  static auto &calls() {
    static auto calls = 0;
    return calls;
  }

  Storage() { ++calls<Ctor>(); }
  Storage(const Storage &) { ++calls<CopyCtor>(); }
  Storage(Storage &&) { ++calls<MoveCtor>(); }
  ~Storage() { ++calls<Dtor>(); }
};

test should_support_dynamic_storage = [] {
  Storage::calls<Ctor>() = 0;
  Storage::calls<CopyCtor>() = 0;
  Storage::calls<MoveCtor>() = 0;
  Storage::calls<Dtor>() = 0;

  { te::dynamic_storage storage{Storage{}}; }

  expect(1 == Storage::calls<Ctor>());
  expect(1 == Storage::calls<CopyCtor>());
  expect(0 == Storage::calls<MoveCtor>());
  expect(2 == Storage::calls<Dtor>());
};

test should_support_local_storage = [] {
  Storage::calls<Ctor>() = 0;
  Storage::calls<CopyCtor>() = 0;
  Storage::calls<MoveCtor>() = 0;
  Storage::calls<Dtor>() = 0;

  { te::local_storage<16> storage{Storage{}}; }

  expect(1 == Storage::calls<Ctor>());
  expect(0 == Storage::calls<CopyCtor>());
  expect(1 == Storage::calls<MoveCtor>());
  expect(2 == Storage::calls<Dtor>());
};

test should_support_custom_storage = [] {
  te::poly<Addable> addable_def{Calc{}};
  expect(42 == addable_def.add(40, 2));

  te::poly<Addable, te::local_storage<16>> addable_local{Calc{}};
  expect(42 == addable_local.add(40, 2));

  te::poly<Addable, te::local_storage<16>, te::static_vtable<Addable>>
      addable_local_static{Calc{}};
  expect(42 == addable_local_static.add(40, 2));
};

#if (__GNUC__ < 7)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

test should_set_get_mappings = [] {
  struct B {};
  te::detail::mappings<class A, 0>::set<B>();
  static_assert(
      std::is_same<B, decltype(get(te::detail::mappings<class A, 0>{}))>{});

  struct C {};
  te::detail::mappings<class A, 1>::set<C>();
  static_assert(
      std::is_same<C, decltype(get(te::detail::mappings<class A, 1>{}))>{});
};

test should_return_mappings_size = [] {
  static_assert(
      0 ==
      te::detail::mappings_size<class Size, std::integral_constant<int, 0>>());
  te::detail::mappings<class Size, 1>::set<std::integral_constant<int, 1>>();
  te::detail::mappings<class Size, 2>::set<std::integral_constant<int, 2>>();
  te::detail::mappings<class Size, 3>::set<std::integral_constant<int, 3>>();
  static_assert(
      3 ==
      te::detail::mappings_size<class Size, std::integral_constant<int, 1>>());
};
