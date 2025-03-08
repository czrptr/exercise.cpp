#pragma once

#include <bit>
#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <unordered_map>
#include <vector>

#include <fmt/core.h>

#include <range/v3/all.hpp>

#include "lib/nameof.hh"
#include "lib/span.hh"
#include "lib/type_traits.hh"

#include "serde/descriptor.hh"
#include "serde/sizeof.hh"
#include "serde/tag.hh"

namespace serde
{
namespace detail
{

std::unordered_map<Tag, std::string>& typename_of_tag();

inline constexpr auto little_endian_order()
{
  if constexpr (std::endian::native == std::endian::little)
  {
    return ranges::views::all;
  }
  else // std::endian::native == std::endian::big
  {
    return ranges::views::reverse;
  }
}

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
T deserialize(std::span<std::byte const> bytes)
{
  assert(bytes.size() == sizeof(T));

  T result;
  ranges::copy(bytes | little_endian_order(), reinterpret_cast<std::byte*>(&result));
  return result;
}

template <typename T>
std::pair<Tag, T> deserialize_with_tag(std::span<std::byte const> bytes)
{
  auto const [tag_bytes, value_bytes] = lib::split(bytes, sizeof(Tag));
  return {deserialize<Tag>(tag_bytes), deserialize<T>(value_bytes)};
}

template <typename T>
void check_and_advance(Tag tag, size_t& cursor)
{
  Tag const expected_tag = tag_of<T>();
  if (tag == expected_tag)
  {
    cursor += sizeof(Tag);
    return;
  }
  throw std::logic_error(fmt::format(
    "Metadata mismatch: expecting '{}' but found '{}' starting at byte {}",
    lib::nameof<T>(),
    typename_of_tag()[tag],
    cursor));
}

template <typename T>
T deserialize_check_and_advance(std::span<std::byte const> bytes, size_t& cursor)
{
  auto const [tag, value] = deserialize_with_tag<T>(bytes);
  check_and_advance<T>(tag, cursor);
  cursor += sizeof(T);
  return value;
}

template <typename T>
std::vector<std::byte> serialize_impl(T const& t)
{
  if constexpr (lib::is_vector<T>)
  {
    std::vector<std::vector<std::byte>> bytes;
    bytes.push_back(serialize(tag_of<T>()));
    bytes.push_back(serialize_with_tag(t.size()));
    for (auto const& element : t)
    {
      bytes.push_back(serialize_impl(element));
    }
    return bytes | ranges::views::join | ranges::to<std::vector>;
  }
  else if constexpr (std::is_class_v<T>)
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
  else
  {
    return serialize_with_tag(t);
  }
}

template <typename T>
T deserialize_impl(std::span<std::byte const> bytes, size_t& cursor)
{
  if constexpr (lib::is_vector<T>)
  {
    auto bytes_to_process = lib::take(bytes, sizeof(Tag));
    check_and_advance<T>(deserialize<Tag>(bytes_to_process), cursor);
    using SizeType = typename T::size_type;
    bytes_to_process = lib::take(bytes, serde::serialized_sizeof<SizeType>);
    auto const size = deserialize_check_and_advance<size_t>(bytes_to_process, cursor);
    T result;
    result.reserve(size);
    for (size_t it = 0u; it < size; it += 1u)
    {
      using Element = typename T::value_type;
      bytes_to_process = lib::take(bytes, serde::serialized_sizeof<Element>);
      auto const element = deserialize_impl<Element>(bytes_to_process, cursor);
      result.push_back(element);
    }
    return result;
  }
  else if constexpr (std::is_class_v<T>)
  {
    auto bytes_to_process = lib::take(bytes, sizeof(Tag));
    check_and_advance<T>(deserialize<Tag>(bytes_to_process), cursor);
    T result;
    foreach_member_of<T>(
      [&](auto const pointer_to_member)
      {
        using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
        bytes_to_process = lib::take(bytes, serialized_sizeof<Member>);
        result.*pointer_to_member = deserialize_impl<Member>(bytes_to_process, cursor);
      });
    return result;
  }
  else
  {
    return deserialize_check_and_advance<T>(bytes, cursor);
  }
}

} // namespace detail

template <typename T>
std::vector<std::byte> serialize(T const& t)
{
  return detail::serialize_impl(t);
}

template <typename T>
T deserialize(std::span<std::byte const> bytes)
{
  size_t cursor = 0u;
  return detail::deserialize_impl<T>(bytes, cursor);
}

template <typename T>
T deserialize(std::vector<std::byte> const& buffer)
{
  return deserialize<T>(std::span(buffer.data(), buffer.size()));
}

} // namespace serde
