#pragma once

#include <cstddef>
#include <vector>

#include <fmt/core.h>

#include <range/v3/all.hpp>
#include <range/v3/view/iota.hpp>

#include "lib/nameof.hh"
#include "lib/type_traits.hh"

#include "serde/common.hh"
#include "serde/descriptor.hh"
#include "serde/tag.hh"

namespace serde
{
namespace detail
{

template <typename... Rest>
inline auto merge(Rest&&... rest)
{
  return ranges::concat_view(rest...) | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> serialize_raw(T const& t)
{
  auto const begin = reinterpret_cast<std::byte const*>(&t);
  auto const bytes = ranges::subrange(begin, begin + sizeof(T));
  return bytes | little_endian_order() | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> serialize_raw_with_tag(T const& t)
{
  auto const tag = tag_of<T>();
  typename_of_tag()[tag] = lib::nameof<T>();
  return ranges::concat_view(serialize_raw(tag), serialize_raw(t)) | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> dispatch_serialize(T const& t);

template <typename T>
std::vector<std::byte> serialize_vector(std::vector<T> const& vector)
{
  return merge(
    serialize_raw(tag_of<std::vector<T>>()),
    serialize_raw_with_tag(vector.size()),
    vector | ranges::views::transform(dispatch_serialize<T>) | ranges::views::join);
}

template <typename T>
std::vector<std::byte> serialize_class(T const& t)
{
  std::vector<std::vector<std::byte>> members;
  foreach_member_of<T>(
    [&](auto const pointer_to_member)
    {
      members.push_back(dispatch_serialize(t.*pointer_to_member));
    });
  return merge(serialize_raw(tag_of<T>()), members | ranges::views::join);
}

template <typename T>
std::vector<std::byte> serialize_pointer(T* pointer)
{
  auto const serialized_tag = serialize_raw(tag_of<T*>());
  return (pointer == nullptr)
    ? merge(serialized_tag, serialize_raw(SerializedPointer::IsNull))
    : merge(serialized_tag, serialize_raw(SerializedPointer::IsPresent), dispatch_serialize(*pointer));
}

template <typename T>
std::vector<std::byte> dispatch_serialize(T const& t)
{
  if constexpr (lib::is_vector<T>)
  {
    return serialize_vector(t);
  }
  else if constexpr (std::is_class_v<T>)
  {
    return serialize_class(t);
  }
  else if constexpr (std::is_pointer_v<T>)
  {
    return serialize_pointer(t);
  }
  else
  {
    return serialize_raw_with_tag(t);
  }
}

} // namespace detail

template <typename T>
std::vector<std::byte> serialize(T const& t)
{
  return detail::dispatch_serialize(t);
}

} // namespace serde
