#pragma once

#include <cstdint>
#include <numeric>
#include <vector>

#include "lib/nameof.hh"
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
  return std::accumulate(
    vector.begin(),
    vector.end(),
    vector.size(),
    [](Tag acc, Tag next)
    {
      next = ((next >> 16) ^ next) * 0x45d9f3b;
      next = ((next >> 16) ^ next) * 0x45d9f3b;
      next = (next >> 16) ^ next;
      return acc ^ (next + 0x9e3779b9 + (acc << 6) + (acc >> 2));
    });
}

template <typename T>
Tag tag_of_impl()
{
  if constexpr (std::is_pointer_v<T>)
  {
    // TODO: add pointer serialization support
    static_assert(false, "cannot calculate tag for pointer type");
  }
  else if constexpr (lib::is_vector<T>)
  {
    std::hash<std::string> const hasher;
    return hasher(lib::nameof<T>());
  }
  else if constexpr (std::is_class_v<T>)
  {
    std::vector<Tag> member_tags;
    detail::foreach_member_of<T>(
      [&](auto const pointer_to_member)
      {
        using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
        member_tags.push_back(tag_of_impl<Member>());
      });
    return detail::hash_of(member_tags);
  }
  else
  {
    std::hash<std::string> const hasher;
    return hasher(lib::nameof<T>());
  }
}

} // namespace detail

template <typename T>
Tag tag_of()
{
  return detail::tag_of_impl<T>();
}

} // namespace serde