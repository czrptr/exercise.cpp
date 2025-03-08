#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include <range/v3/all.hpp>
#include <fmt/core.h>

#include "lib/type_traits.hh"
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
void check(Tag tag)
{
  if (tag != tag_of<T>)
  {
    // TODO: improve error message
    throw std::logic_error("Metadata mismatch");
  }
}

template<typename T>
T deserialize_and_check_tag(std::span<uint8_t const> bytes)
{
  auto [tag, value] = detail::deserialize_with_tag<T>(bytes);
  check<T>(tag);
  return value;
}

} // namespace detail

template<typename T>
std::vector<uint8_t> serialize(T const& t)
{
  std::vector<std::vector<uint8_t>> bytes;
  if constexpr (std::is_class_v<T>)
  {
    bytes.push_back(detail::serialize(tag_of<T>));
    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      bytes.push_back(serialize(t.*pointer_to_member));
    });
    return bytes | ranges::views::join | ranges::to<std::vector>;
  }
  else
  {
    return detail::serialize_with_tag(t);
  }
}

template<typename T>
T deserialize(std::span<uint8_t const> bytes)
{
  if constexpr (std::is_class_v<T>)
  {
    auto [bytes_to_process, bytes_left] = lib::split(bytes, sizeof(Tag));
    detail::check<T>(detail::deserialize<Tag>(bytes_to_process));
    bytes = bytes_left;
    T result;
    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer_t<typeof(pointer_to_member)>;
      auto [bytes_to_process, bytes_left] = lib::split(bytes, serialized_sizeof<Member>);
      result.*pointer_to_member = deserialize<Member>(bytes_to_process);
      bytes = bytes_left;
    });
    return result;
  }
  else
  {
    return detail::deserialize_and_check_tag<T>(bytes);
  }
}

template<typename T>
T deserialize(std::vector<uint8_t> const& buffer)
{
  return deserialize<T>(std::span(buffer.data(), buffer.size()));
}

} // namespace serde
