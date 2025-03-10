#pragma once

#include <array>
#include <tuple>
#include <type_traits>

template <typename T>
constexpr auto descriptor_of()
{
  static_assert(false, "descriptor_of must be specialized in order to be used");
}

namespace serde::detail
{

template <typename T, typename F>
constexpr auto foreach_member_of(F&& f)
{
  auto const descriptor = descriptor_of<T>();
  using ReturnType = decltype(f(std::get<0>(descriptor)));

  if constexpr (std::is_same_v<ReturnType, void>)
  {
    std::apply(
      [&](auto&&... members)
      {
        // apply f to each argument
        ((std::forward<F>(f)(std::forward<typeof(members)>(members))), ...);
      },
      descriptor);
  }
  else
  {
    auto const size = std::tuple_size_v<typeof(descriptor)>;
    std::array<ReturnType, size> result;
    auto it = 0uz;

    std::apply(
      [&](auto&&... members)
      {
        // apply f to each argument
        ((result[it++] = std::forward<F>(f)(std::forward<typeof(members)>(members))), ...);
      },
      descriptor);

    return result;
  }
}

} // namespace serde::detail
