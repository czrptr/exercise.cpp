#pragma once

#include <cstddef>
#include <stdexcept>
#include <vector>

#include <fmt/core.h>

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
T deserialize_raw(std::span<std::byte const>& bytes)
{
  T result;
  ranges::copy(bytes | ranges::views::take(sizeof(T)) | little_endian_order(), reinterpret_cast<std::byte*>(&result));
  bytes = bytes | ranges::views::drop(sizeof(T));
  return result;
}

template <typename T>
void check(Tag tag, size_t& cursor)
{
  Tag const expected_tag = tag_of<T>();
  if (tag == expected_tag)
  {
    cursor += sizeof(Tag);
    return;
  }

  auto const found = typename_of_tag().contains(tag) ? fmt::format("'{}'", typename_of_tag()[tag]) : "no metadata";
  throw std::logic_error(
    fmt::format("Metadata mismatch: expecting '{}' but found {} starting at byte {}", lib::nameof<T>(), found, cursor));
}

template <typename T>
T deserialize_raw_and_check(std::span<std::byte const>& bytes, size_t& cursor)
{
  auto const tag = deserialize_raw<Tag>(bytes);
  auto const value = deserialize_raw<T>(bytes);
  check<T>(tag, cursor);
  cursor += sizeof(T);
  return value;
}

template <typename T>
T dispatch_deserialize(std::span<std::byte const>& bytes, size_t& cursor);

template <typename T>
T deserialize_vector(std::span<std::byte const>& bytes, size_t& cursor)
{
  auto const deserialize_element = [&](...)
  {
    return dispatch_deserialize<typename T::value_type>(bytes, cursor);
  };
  check<T>(deserialize_raw<Tag>(bytes), cursor);
  auto const size = deserialize_raw_and_check<typename T::size_type>(bytes, cursor);
  return ranges::views::iota(0uz, size) | ranges::views::transform(deserialize_element) | ranges::to<std::vector>;
}

template <typename T>
T deserialize_class(std::span<std::byte const>& bytes, size_t& cursor)
{
  T result;
  auto const deserialize_member = [&](auto const pointer_to_member)
  {
    using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
    result.*pointer_to_member = dispatch_deserialize<Member>(bytes, cursor);
  };
  check<T>(deserialize_raw<Tag>(bytes), cursor);
  foreach_member_of<T>(deserialize_member);
  return result;
}

template <typename T>
T deserialize_pointer(std::span<std::byte const>& bytes, size_t& cursor)
{
  check<T>(deserialize_raw<Tag>(bytes), cursor);
  auto const info = deserialize_raw<SerializedPointer>(bytes);

  if (info == SerializedPointer::IsNull)
    return nullptr;

  using Data = std::remove_pointer_t<T>;
  return new Data(dispatch_deserialize<Data>(bytes, cursor));
}

template <typename T>
T dispatch_deserialize(std::span<std::byte const>& bytes, size_t& cursor)
{
  if constexpr (lib::is_vector<T>)
  {
    return deserialize_vector<T>(bytes, cursor);
  }
  else if constexpr (std::is_class_v<T>)
  {
    return deserialize_class<T>(bytes, cursor);
  }
  else if constexpr (std::is_pointer_v<T>)
  {
    return deserialize_pointer<T>(bytes, cursor);
  }
  else
  {
    return deserialize_raw_and_check<T>(bytes, cursor);
  }
}

} // namespace detail

template <typename T>
T deserialize(std::span<std::byte const> bytes)
{
  size_t cursor = 0u;
  return detail::dispatch_deserialize<T>(bytes, cursor);
}

template <typename T>
T deserialize(std::vector<std::byte> const& bytes)
{
  return deserialize<T>(std::span(bytes.data(), bytes.size()));
}

} // namespace serde
