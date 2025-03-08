#pragma once

#include <tuple>

template <typename T>
constexpr auto descriptor_of()
{
  static_assert(false, "descriptor_of must be specialized in order to be used");
}

namespace serde::detail
{

template <typename T, typename F>
constexpr void foreach_member_of(F&& f)
{
  std::apply(
    [f](auto... members)
    {
      // apply f to each argument
      ((f(members)), ...);
    },
    descriptor_of<T>());
}

} // namespace serde::detail
