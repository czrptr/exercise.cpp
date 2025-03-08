#pragma once

#include <type_traits>

namespace lib
{
namespace detail
{

template <typename T>
struct remove_member_pointer_impl;

template <typename StructT, typename MemberT>
struct remove_member_pointer_impl<MemberT StructT::*>
{
  using type = MemberT;
};

template <typename StructT, typename MemberT>
struct remove_member_pointer_impl<MemberT StructT::* const>
{
  using type = MemberT;
};

} // namespace detail

template <typename T>
using remove_member_pointer = detail::remove_member_pointer_impl<T>::type;

template <typename... Ts>
struct type_list;

template <typename T, typename... Ts>
struct type_list<T, Ts...>
{
  using head = T;
  using rest = type_list<Ts...>;
};

template <auto... V>
struct value_list;

template <auto V, auto... Vs>
struct value_list<V, Vs...>
{
  static constexpr auto head = V;
  using rest = value_list<Vs...>;
};

template <typename Ts, typename Vs>
struct type_map
{
public:
  using types = Ts;
  using values = Vs;

private:
  template <typename T, typename Map = type_map<Ts, Vs>>
  static consteval auto get_impl()
  {
    if constexpr (std::is_same_v<T, typename Map::types::head>)
    {
      return Map::values::head;
    }
    else
    {
      using NextMap = type_map<typename Map::types::rest, typename Map::values::rest>;
      return get_impl<T, NextMap>();
    }
  }

public:
  template <typename T>
  static constexpr auto get = get_impl<T>();
};

} // namespace lib