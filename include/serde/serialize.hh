#pragma once

#include <vector>

#include <range/v3/all.hpp>

#include "lib/nameof.hh"
#include "lib/type_traits.hh"

#include "serde/common.hh"
#include "serde/descriptor.hh"
#include "serde/tag.hh"

namespace serde
{
namespace detail
{

template <typename T>
std::vector<std::byte> serialize(T const& t)
{
  auto const begin = reinterpret_cast<std::byte const*>(&t);
  auto const bytes = ranges::subrange(begin, begin + sizeof(T));
  return bytes | little_endian_order() | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> serialize_with_tag(T const& t)
{
  auto const tag = tag_of<T>();
  typename_of_tag()[tag] = lib::nameof<T>();
  return ranges::concat_view(serialize(tag), serialize(t)) | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> serialize_impl(T const& t);

template <typename T>
std::vector<std::byte> serialize_vector(std::vector<T> const& vector)
{
  std::vector<std::vector<std::byte>> bytes;
  bytes.push_back(serialize(tag_of<std::vector<T>>()));
  bytes.push_back(serialize_with_tag(vector.size()));
  for (auto const& element : vector)
  {
    bytes.push_back(serialize_impl(element));
  }
  return bytes | ranges::views::join | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> serialize_class(T const& t)
{
  std::vector<std::vector<std::byte>> bytes;
  bytes.push_back(serialize(tag_of<T>()));
  foreach_member_of<T>(
    [&](auto const pointer_to_member)
    {
      bytes.push_back(serialize_impl(t.*pointer_to_member));
    });
  return bytes | ranges::views::join | ranges::to<std::vector>;
}
template <typename T>
std::vector<std::byte> serialize_pointer(T* pointer)
{
  std::vector<std::vector<std::byte>> bytes;
  bytes.push_back(serialize(tag_of<T*>()));
  if (pointer == nullptr)
  {
    bytes.push_back(serialize(SerializedPointer::IsNull));
  }
  else
  {
    bytes.push_back(serialize(SerializedPointer::IsPresent));
    bytes.push_back(serialize_impl(*pointer));
  }
  return bytes | ranges::views::join | ranges::to<std::vector>;
}

template <typename T>
std::vector<std::byte> serialize_impl(T const& t)
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
    return serialize_with_tag(t);
  }
}

} // namespace detail

template <typename T>
std::vector<std::byte> serialize(T const& t)
{
  return detail::serialize_impl(t);
}

} // namespace serde
