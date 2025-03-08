#pragma once

#include <cstdint>
#include <numeric>
#include <tuple>
#include <type_traits>
#include <vector>

#include "lib/type_traits.hh"
#include "serde/descriptor.hh"

namespace serde
{

using Tag = uint64_t;

namespace detail
{

constexpr Tag hash_of(std::vector<Tag> const& vector)
{
  // see https://stackoverflow.com/a/72073933
  return std::accumulate(vector.begin(), vector.end(), vector.size(), [](Tag acc, Tag next)
  {
    next = ((next >> 16) ^ next) * 0x45d9f3b;
    next = ((next >> 16) ^ next) * 0x45d9f3b;
    next = (next >> 16) ^ next;
    return acc ^ (next + 0x9e3779b9 + (acc << 6) + (acc >> 2));
  });
}

template<typename T>
consteval Tag tag_of_impl()
{
  if constexpr (std::is_pointer_v<T>)
  {
    // TODO: add pointer serialization support
    static_assert(false, "cannot calculate tag for pointer type");
  }
  else if constexpr (std::is_class_v<T>)
  {
    auto const descriptor = descriptor_of<T>();
    size_t const descriptor_count = std::tuple_size_v<typeof(descriptor)>;

    std::vector<Tag> member_tags {};
    member_tags.reserve(descriptor_count);

    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
      member_tags.push_back(tag_of_impl<Member>());
    });

    return detail::hash_of(member_tags);
  }
  else
  {
    using Map = lib::type_map<
      lib::type_list<bool, char, uint8_t, int8_t, uint16_t, int16_t, uint32_t, int32_t, uint64_t, int64_t, float, double>,
      // some random prime numbers
      lib::value_list<14411, 32099, 93827, 101719, 241793, 357787, 472993, 504547, 617761, 724967, 841021, 982337>>;

    return Map::get<T>;
  }
}

} // namespace detail

template<typename T>
constexpr auto tag_of = detail::tag_of_impl<T>();

} // namespace serde