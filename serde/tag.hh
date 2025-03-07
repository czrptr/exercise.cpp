#pragma once

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <vector>

#include "lib/type_traits.hh"
#include "serde/descriptor.hh"

namespace serde
{

using Tag = uint32_t;

namespace detail
{

constexpr Tag hash_of(std::vector<Tag> const& vector)
{
  // copied from stack overflow
  Tag seed = vector.size();
  for(auto it : vector)
  {
    it = ((it >> 16) ^ it) * 0x45d9f3b;
    it = ((it >> 16) ^ it) * 0x45d9f3b;
    it = (it >> 16) ^ it;
    seed ^= it + 0x9e3779b9 + (seed << 6) + (seed >> 2);
  }
  return seed;
}

template<typename T>
consteval Tag tag_of_impl()
{
  // TODO: maybe use prime numbers?
  if constexpr (std::is_same_v<T, bool>)
  {
    return 0u;
  }
  else if constexpr (std::is_same_v<T, char>)
  {
    return 1u;
  }
  else if constexpr (std::is_same_v<T, uint8_t>)
  {
    return 2u;
  }
  else if constexpr (std::is_same_v<T, int8_t>)
  {
    return 3u;
  }
  else if constexpr (std::is_same_v<T, uint16_t>)
  {
    return 4u;
  }
  else if constexpr (std::is_same_v<T, int16_t>)
  {
    return 5u;
  }
  else if constexpr (std::is_same_v<T, uint32_t>)
  {
    return 6u;
  }
  else if constexpr (std::is_same_v<T, int32_t>)
  {
    return 7u;
  }
  else if constexpr (std::is_same_v<T, uint64_t>)
  {
    return 8u;
  }
  else if constexpr (std::is_same_v<T, int64_t>)
  {
    return 9u;
  }
  else if constexpr (std::is_same_v<T, float>)
  {
    return 10u;
  }
  else if constexpr (std::is_same_v<T, double>)
  {
    return 11u;
  }
  else if constexpr (std::is_class_v<T>)
  {
    auto const descriptor = descriptor_of<T>();
    size_t const descriptor_count = std::tuple_size_v<typeof(descriptor)>;

    std::vector<Tag> member_tags {};
    member_tags.reserve(descriptor_count);

    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer_t<typeof(pointer_to_member)>;
      member_tags.push_back(tag_of_impl<Member>());
    });

    return detail::hash_of(member_tags);
  }
  else
  {
    static_assert(false, "cannot calculate tag for the given type");
  }
}

template<typename T>
struct tag_of_helper
{
  static constexpr auto value = detail::tag_of_impl<T>();
};

} // namespace detail

template<typename T>
constexpr auto tag_of = detail::tag_of_helper<T>::value;

} // namespace serde