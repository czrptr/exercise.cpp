#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <range/v3/all.hpp>
#include <fmt/core.h>

#include "lib/type_traits.hh"
#include "lib/demangle.hh"
#include "lib/span.hh"

#include "serde/descriptor.hh"
#include "serde/sizeof.hh"
#include "serde/tag.hh"

namespace serde
{
namespace detail
{

template<typename T>
std::vector<uint8_t> serialize(T const& t)
{
  auto const begin = reinterpret_cast<uint8_t const*>(&t);
  auto const bytes = ranges::subrange(begin, begin + sizeof(T));
  return bytes | ranges::to<std::vector>;
}

template<typename T>
std::vector<uint8_t> serialize_with_tag(T const& t)
{
  return
    ranges::concat_view(serialize(tag_of<T>), serialize(t))
    | ranges::to<std::vector>;
}

template<typename T>
T deserialize(std::span<uint8_t const> bytes)
{
  assert(bytes.size() == sizeof(T));

  T result;
  ranges::copy(bytes, reinterpret_cast<uint8_t*>(&result));
  return result;
}

template<typename T>
std::pair<Tag, T> deserialize_with_tag(std::span<uint8_t const> bytes)
{
  auto [tag_bytes, value_bytes] = lib::split(bytes, sizeof(Tag));
  return {
    deserialize<Tag>(tag_bytes),
    deserialize<T>(value_bytes),
  };
}

template<typename T>
void check(Tag tag, size_t index)
{
  if (tag != tag_of<T>)
  {
    // TODO: print type names instead of tag names if RTTI is present
    throw std::logic_error(
      fmt::format(
        "Metadata mismatch: expecting '{}' but found '{}' at byte {}",
        tag_of<T>, tag, index));
  }
}

template<typename T>
T deserialize_and_check_tag(std::span<uint8_t const> bytes, size_t index)
{
  auto [tag, value] = deserialize_with_tag<T>(bytes);
  check<T>(tag, index);
  return value;
}

template<typename T>
std::vector<uint8_t> serialize_impl(T const& t)
{
  std::vector<std::vector<uint8_t>> bytes;
  if constexpr (std::is_class_v<T>)
  {
    bytes.push_back(serialize(tag_of<T>));
    foreach_member_of<T>([&](auto pointer_to_member)
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

template<typename T>
T deserialize_impl(std::span<uint8_t const> bytes, size_t index)
{
  if constexpr (std::is_class_v<T>)
  {
    auto [bytes_to_process, bytes_left] = lib::split(bytes, sizeof(Tag));
    check<T>(deserialize<Tag>(bytes_to_process), index);
    bytes = bytes_left;
    index += bytes_to_process.size();
    T result;
    foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer_t<typeof(pointer_to_member)>;
      auto [bytes_to_process, bytes_left] = lib::split(bytes, serialized_sizeof<Member>());
      result.*pointer_to_member = deserialize_impl<Member>(bytes_to_process, index);
      bytes = bytes_left;
      index += bytes_to_process.size();
    });
    return result;
  }
  else
  {
    return deserialize_and_check_tag<T>(bytes, index);
  }
}

} // namespace detail

template<typename T>
std::vector<uint8_t> serialize(T const& t)
{
  return detail::serialize_impl(t);
}
template<typename T>
T deserialize(std::span<uint8_t const> bytes)
{
  return detail::deserialize_impl<T>(bytes, 0);
}

template<typename T>
T deserialize(std::vector<uint8_t> const& buffer)
{
  return deserialize<T>(std::span(buffer.data(), buffer.size()));
}

} // namespace serde
