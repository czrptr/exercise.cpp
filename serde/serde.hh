#include <cassert>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <range/v3/all.hpp>

#include "lib/span.hh"

#include "descriptor.hh"
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
  uint8_t const * begin = reinterpret_cast<uint8_t const*>(&t);
  auto bytes = ranges::subrange(begin, begin + sizeof(T));

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
  return {
    deserialize<Tag>(bytes.subspan(0, sizeof(Tag))),
    deserialize<T>(bytes.subspan(sizeof(Tag), sizeof(T))),
  };
}

template<typename T>
T deserialize_and_check_tag(std::span<uint8_t const> bytes)
{
  auto [tag, value] = detail::deserialize_with_tag<T>(bytes);
  if (tag != tag_of<T>)
  {
    // TODO: improve error message
    throw std::logic_error("Metadata mismatch");
  }
  return value;
}

} // namesapce detail

template<typename T>
std::vector<uint8_t> serialize(T const& t)
{
  std::vector<std::vector<uint8_t>> bytes;
  if constexpr (std::is_class_v<T>)
  {
    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      bytes.push_back(detail::serialize_with_tag(t.*pointer_to_member));
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
    T result;
    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer_t<typeof(pointer_to_member)>;
      auto [bytes_to_process, bytes_left] = lib::split(bytes, serialized_sizeof<Member>);
      result.*pointer_to_member = detail::deserialize_and_check_tag<Member>(bytes_to_process);
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
