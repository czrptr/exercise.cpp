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
T deserialize(std::span<std::byte const>& bytes)
{
  T result;
  ranges::copy(bytes | ranges::views::take(sizeof(T)) | little_endian_order(), reinterpret_cast<std::byte*>(&result));
  bytes = bytes | ranges::views::drop(sizeof(T));
  return result;
}

template <typename T>
std::pair<Tag, T> deserialize_with_tag(std::span<std::byte const>& bytes)
{
  return {deserialize<Tag>(bytes), deserialize<T>(bytes)};
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
T deserialize_check_and_advance(std::span<std::byte const>& bytes, size_t& cursor)
{
  auto const [tag, value] = deserialize_with_tag<T>(bytes);
  check_and_advance<T>(tag, cursor);
  cursor += sizeof(T);
  return value;
}

template <typename T>
T deserialize_impl(std::span<std::byte const>& bytes, size_t& cursor);

template <typename T>
T deserialize_vector(std::span<std::byte const>& bytes, size_t& cursor)
{
  check_and_advance<T>(deserialize<Tag>(bytes), cursor);
  using SizeType = typename T::size_type;
  auto const size = deserialize_check_and_advance<SizeType>(bytes, cursor);
  T result;
  result.reserve(size);
  for (size_t it = 0u; it < size; it += 1u)
  {
    using Element = typename T::value_type;
    auto const element = deserialize_impl<Element>(bytes, cursor);
    result.push_back(element);
  }
  return result;
}

template <typename T>
T deserialize_class(std::span<std::byte const>& bytes, size_t& cursor)
{
  check_and_advance<T>(deserialize<Tag>(bytes), cursor);
  T result;
  foreach_member_of<T>(
    [&](auto const pointer_to_member)
    {
      using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
      result.*pointer_to_member = deserialize_impl<Member>(bytes, cursor);
    });
  return result;
}

template <typename T>
T deserialize_pointer(std::span<std::byte const>& bytes, size_t& cursor)
{
  check_and_advance<T>(deserialize<Tag>(bytes), cursor);
  auto const info = deserialize<SerializedPointer>(bytes);

  if (info == SerializedPointer::IsNull)
    return nullptr;

  using Data = std::remove_pointer_t<T>;
  auto const data = deserialize_impl<Data>(bytes, cursor);
  return new Data(data);
}

template <typename T>
T deserialize_impl(std::span<std::byte const>& bytes, size_t& cursor)
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
T deserialize(std::vector<std::byte> const& bytes)
{
  return deserialize<T>(std::span(bytes.data(), bytes.size()));
}

} // namespace serde
