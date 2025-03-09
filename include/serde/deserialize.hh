#pragma once

#include <cassert>
#include <cstddef>
#include <stdexcept>
#include <vector>

#include <fmt/core.h>

#include <range/v3/all.hpp>

#include "lib/nameof.hh"
#include "lib/span.hh"
#include "lib/type_traits.hh"

#include "serde/common.hh"
#include "serde/descriptor.hh"
#include "serde/sizeof.hh"
#include "serde/tag.hh"

namespace serde
{
namespace detail
{

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
T deserialize_impl(std::span<std::byte const> bytes, size_t& cursor);

template <typename T>
T deserialize_vector(std::span<std::byte const> bytes, size_t& cursor)
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

template <typename T>
T deserialize_class(std::span<std::byte const> bytes, size_t& cursor)
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

template <typename T>
T deserialize_impl(std::span<std::byte const> bytes, size_t& cursor)
{
  if constexpr (lib::is_vector<T>)
  {
    return deserialize_vector<T>(bytes, cursor);
  }
  else if constexpr (std::is_class_v<T>)
  {
    return deserialize_class<T>(bytes, cursor);
  }
  else
  {
    return deserialize_check_and_advance<T>(bytes, cursor);
  }
}

} // namespace detail

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
