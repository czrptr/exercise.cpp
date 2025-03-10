#pragma once

#include <array>
#include <cstdint>
#include <numeric>

#include "lib/nameof.hh"
#include "lib/type_traits.hh"

#include "serde/descriptor.hh"

namespace serde
{

using Tag = uint64_t;

namespace detail
{

template <size_t Length>
constexpr Tag hash_of(std::array<Tag, Length> const& vector)
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
  if constexpr (lib::is_vector<T>)
  {
    std::hash<std::string> const hasher;
    return hasher(lib::nameof<T>());
  }
  else if constexpr (std::is_class_v<T>)
  {
    auto const tag_of_member = [&](auto const pointer_to_member)
    {
      using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
      return tag_of_impl<Member>();
    };
    return hash_of(foreach_member_of<T>(tag_of_member));
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