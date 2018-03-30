# Type.Erasure <a href="http://www.boost.org/LICENSE_1_0.txt" target="_blank">![Boost Licence](http://img.shields.io/badge/license-boost-blue.svg)</a> <a href="https://travis-ci.org/boost-experimental/te" target="_blank">![Build Status](https://img.shields.io/travis/boost-experimental/te/master.svg?label=linux/osx)</a> <a href="http://github.com/boost-experimental/te/issues" target="_blank">![Github Issues](https://img.shields.io/github/issues/boost-experimental/te.svg)</a> <a href="https://godbolt.org/g/pQNp7i">![Try it online](https://img.shields.io/badge/try%20it-online-blue.svg)</a>

> Your C++17 **one header only** type erasure library with no dependencies, macors and limited boilerplate

### Quick start

> Type.Erasure requires only one file. Get the latest header [here!](https://github.com/boost-experimental/te/blob/master/include/te.hpp)

```cpp
#include <te.hpp>
```

#### Erase it

```cpp
// Define interface of something that can be drawn
struct Drawable {
  void draw(std::ostream &out) const {
    te::call([](auto const &self, auto &out) { self.draw(out); }, *this, out);
  }
};

// Define implementation which is drawable (No inheritance)
struct Square {
  void draw(std::ostream &out) const { out << "Square"; }
};

// Define other implementation which is drawable (No inheritance)
struct Circle {
  void draw(std::ostream &out) const { out << "Circle"; }
};

// Define object which can hold drawable objects (No dynamic storage)
void draw(te::poly<Drawable> const &drawable) { drawable.draw(std::cout); }

int main() {
  // Call draw polymorphically (Value semantics)
  draw(Circle{}); // prints Circle
  draw(Square{}); // prints Square
}
```

#### Store it

```cpp
int main() {
  std::vector<te::poly<Drawable>> drawables{};

  drawables.push_back(Square{});
  drawables.push_back(Circle{});

  for (const auto &drawable : drawables) {
    drawable.draw(std::cout);
  }
}
```

#### Overload it

```cpp
struct Addable {
  constexpr auto add(int i)        { return te::call<int>(add_impl, *this, i); }
  constexpr auto add(int a, int b) { return te::call<int>(add_impl, *this, a, b); }

private:
  static constexpr auto add_impl = [](auto &self, auto... args) {
    return self.add(args...);
  };
};

class Calc {
public:
  constexpr auto add(int i)        { return i; }
  constexpr auto add(int a, int b) { return a + b; }
};

int main() {
  te::poly<Addable> addable{Calc{}};
  assert(3 == addable.add(3));
  assert(3 == addable.add(1, 2));
}
```

#### Override it

```cpp
namespace v1 {
struct Drawable {
  void draw(std::ostream &out) const {
    te::call([](auto const &self, auto &&... args) { self.draw(args...); }, *this, out, "v1");
  }
};
} // namespace v1

namespace v2 {
struct Drawable : v1::Drawable {
  Drawable() { te::extends<v1::Drawable>(*this); }
};
} // namespace v2

namespace v3 {
struct Drawable : v2::Drawable {
  Drawable() { te::extends<v2::Drawable>(*this); }

  void draw(std::ostream &out) const {
    te::call([](auto const &self, auto &&... args) { self.draw(args...); }, *this, out, "v3");
  }
};
} // namespace v3

struct Square {
  void draw(std::ostream &out, const std::string &v) const {
    out << v << "::Square";
  }
};

struct Circle {
  void draw(std::ostream &out, const std::string &v) const {
    out << v << "::Circle";
  }
};

template <class T> void draw(te::poly<T> const &drawable) {
  drawable.draw(std::cout);
}

int main() {
  draw<v1::Drawable>(Circle{}); // prints v1::Circle
  draw<v1::Drawable>(Square{}); // prints v1::Square

  draw<v2::Drawable>(Circle{}); // prints v1::Circle
  draw<v2::Drawable>(Square{}); // prints v1::Square

  draw<v3::Drawable>(Circle{}); // prints v3::Circle
  draw<v3::Drawable>(Square{}); // prints v3::Square
}
```

#### Customize it

```cpp
int main() {
  te::poly<Drawable,
           te::local_storage<16>,      // te::dynamic_storage (default)
           te::static_vtable<Drawable> // te::dynamic_vtable (default)
  > drawable{Circle{}};
  drawable.draw(std::cout); // prints Circle
}
```

#### Call it

```cpp
template <class> struct Function;

template <class R, class... Ts> struct Function<R(Ts...)> {
  constexpr inline R operator()(Ts... args) {
    return te::call<R>([](auto &self, Ts... args) { return self(args...); }, *this, args...);
  }
};

template struct Function<int(int)>;

int main() {
  te::poly<Function<int(int)>> f{[](int i) { return i; }};
  assert(42 == f(42));
}
```

---

**Disclaimer** `Type.Erasure` is an experimental library.
