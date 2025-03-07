#pragma once

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <vector>

#include "source/Descriptor.hh"

namespace detail
{

constexpr uint32_t hash_of(std::vector<uint32_t> const& vector)
{
  // copied from stack overflow
  uint32_t seed = vector.size();
  for(auto it : vector)
  {
    it = ((it >> 16) ^ it) * 0x45d9f3b;
    it = ((it >> 16) ^ it) * 0x45d9f3b;
    it = (it >> 16) ^ it;
    seed ^= it + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

} // namespace detail

template<typename T>
consteval uint32_t tag_of()
{
  // TODO: maybe use prime numbers?
  if constexpr (std::is_same_v<T, bool>)
  {
    return 0u;
  }
  if constexpr (std::is_same_v<T, char>)
  {
    return 1u;
  }
  if constexpr (std::is_same_v<T, uint8_t>)
  {
    return 2u;
  }
  if constexpr (std::is_same_v<T, int8_t>)
  {
    return 3u;
  }
  if constexpr (std::is_same_v<T, uint16_t>)
  {
    return 4u;
  }
  if constexpr (std::is_same_v<T, int16_t>)
  {
    return 5u;
  }
  if constexpr (std::is_same_v<T, uint32_t>)
  {
    return 6u;
  }
  if constexpr (std::is_same_v<T, int32_t>)
  {
    return 7u;
  }
  if constexpr (std::is_same_v<T, uint64_t>)
  {
    return 8u;
  }
  if constexpr (std::is_same_v<T, int64_t>)
  {
    return 9u;
  }
  if constexpr (std::is_same_v<T, float>)
  {
    return 10u;
  }
  if constexpr (std::is_same_v<T, double>)
  {
    return 11u;
  }
  if constexpr (std::is_class_v<T>)
  {
    auto const descriptor = descriptor_of<T>();
    size_t const descriptor_count = std::tuple_size_v<typeof(descriptor)>;

    std::vector<uint32_t> member_tags {};
    member_tags.reserve(descriptor_count);

    std::apply([&](auto&& ... args) {
      ((member_tags.push_back(tag_of<underlying_member_t<typeof(args)>>())), ...);
    }, descriptor);

    return detail::hash_of(member_tags);
  }
}
