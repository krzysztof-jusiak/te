//
// Copyright (c) 2018 Kris Jusiak (kris at jusiak dot net)
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
#pragma once
#if not defined(__cpp_variadic_templates) or                                   \
    not defined(__cpp_rvalue_references) or not defined(__cpp_decltype) or     \
    not defined(__cpp_alias_templates) or                                      \
    not defined(__cpp_generic_lambdas) or not defined(__cpp_constexpr) or      \
    not defined(__cpp_return_type_deduction) or                                \
    not defined(__cpp_fold_expressions) or not defined(__cpp_static_assert) or \
    not defined(__cpp_delegating_constructors)
#error "Type.Erasure requires C++17 support"
#else
#pragma GCC system_header
#include <memory>
#include <type_traits>
#include <utility>

namespace te {
inline namespace v1 {
namespace detail {
template <class...>
struct type_list {};
template <class, std::size_t>
struct mappings final {
  friend auto get(mappings);
  template <class T>
  struct set {
    friend auto get(mappings) { return T{}; }
  };
};

template <std::size_t, class...>
constexpr std::size_t mappings_size_impl(...) {
  return {};
}

template <std::size_t N, class T, class... Ts>
constexpr auto mappings_size_impl(bool dummy)
    -> decltype(get(mappings<T, N>{}), std::size_t{}) {
  return 1 + mappings_size_impl<N + 1, T, Ts...>(dummy);
}

template <class... Ts>
constexpr auto mappings_size() {
  return mappings_size_impl<1, Ts...>(bool{});
}
}  // namespace detail

class dynamic_storage {
  struct deleter final {
    void (*del)(void *);
    constexpr void operator()(void *ptr) const { del(ptr); }
  };

 public:
  template <class T>
  constexpr explicit dynamic_storage(T &&t)
      : data_{new std::decay_t<T>{t},
              deleter{[](void *data) { delete static_cast<T *>(data); }}} {}

  operator void *() const { return data_.get(); }

 private:
  std::unique_ptr<void, deleter> data_;
};

template <std::size_t Size>
class local_storage {
 public:
  template <class T>
  constexpr explicit local_storage(T &&t) {
    static_assert(sizeof(std::decay_t<T>) <= Size);
    new (data_) std::decay_t<T>{std::forward<T>(t)};
    dtor_ = [](void *data) { static_cast<T *>(data)->~T(); };
  }

  ~local_storage() { dtor_(data_); }

  operator void *() const { return data_; }

 private:
  unsigned char data_[Size]{};
  void (*dtor_)(void *);
};

class dynamic_vtable {
  using ptr_t = void *;

 public:
  explicit dynamic_vtable(const std::size_t size)
      : vtable_{std::make_unique<ptr_t[]>(size)} {}

  decltype(auto) operator[](std::size_t index) const { return vtable_[index]; }

 private:
  std::unique_ptr<ptr_t[]> vtable_{};
};

template <class>
class static_vtable {
  using ptr_t = void *;

 public:
  explicit static_vtable(const std::size_t size) {
    static auto vtable = std::make_unique<ptr_t[]>(size);
    vtable_ = vtable.get();
  }

  decltype(auto) operator[](std::size_t index) const { return vtable_[index]; }

 private:
  ptr_t *vtable_{};
};

template <class I, class TStorage = dynamic_storage,
          class TVtable = dynamic_vtable>
class poly final : public I {
  static_assert(std::is_copy_constructible<I>{} and std::is_destructible<I>{});

 public:
  template <class T,
            class = std::enable_if_t<not std::is_convertible<T, poly>{} and
                                     std::is_copy_constructible<T>{} and
                                     std::is_destructible<T>{}> >
  constexpr poly(T &&t)
      : poly(std::forward<T>(t),
             std::make_index_sequence<detail::mappings_size<I>()>{}) {}

 private:
  template <class T, std::size_t... Ns>
  constexpr poly(T &&t, std::index_sequence<Ns...>)
      : vtable{sizeof...(Ns)}, storage{std::forward<T>(t)} {
    static_assert(sizeof...(Ns) > 0);
    (init<Ns + 1, T>(decltype(get(detail::mappings<I, Ns + 1>{})){}), ...);
  }

  template <std::size_t N, class T, class TExpr, class... TArgs>
  constexpr void init(detail::type_list<TExpr, TArgs...>) {
    vtable[N - 1] = reinterpret_cast<void *>(+[](void *self, TArgs... args) {
      return (*static_cast<TExpr *>(nullptr))(*static_cast<T *>(self), args...);
    });
  }

  template <std::size_t N, class R, class TExpr, class... Ts>
  friend constexpr auto call(const poly &self,
                             std::integral_constant<std::size_t, N>,
                             detail::type_list<R>, const TExpr, Ts &&... args) {
    void(typename detail::mappings<I, N>::template set<
         detail::type_list<TExpr, Ts...> >{});
    return reinterpret_cast<R (*)(void *, Ts...)>(self.vtable[N - 1])(
        self.storage, std::forward<Ts>(args)...);
  }

  TVtable vtable;
  TStorage storage;
};

template <class R = void, std::size_t N = 0, class TExpr, class I, class... Ts>
constexpr auto call(const TExpr expr, const I &interface, Ts &&... args) {
  struct is_stateless_expr : TExpr {};
  static_assert(sizeof(is_stateless_expr) == sizeof(std::true_type));
  return call(
      static_cast<const poly<I> &>(interface),
      std::integral_constant<std::size_t,
                             detail::mappings_size<I, class call>() + 1>{},
      detail::type_list<R>{}, expr, std::forward<Ts>(args)...);
}

namespace detail {
template <class I, class T, std::size_t... Ns>
constexpr auto extends_impl(std::index_sequence<Ns...>) {
  (void(typename mappings<T, Ns + 1>::template set<decltype(
            get(mappings<I, Ns + 1>{}))>{}),
   ...);
}
}  // namespace detail

template <class I, class T>
constexpr auto extends(const T &) {
  detail::extends_impl<I, T>(
      std::make_index_sequence<detail::mappings_size<I, T>()>{});
}

}  // namespace v1
}  // namespace te
#endif
