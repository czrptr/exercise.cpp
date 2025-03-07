#include <stdexcept>
#include <type_traits>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <span>

#include "source/Tag.hh"

template<typename T>
std::vector<uint8_t> serialize(T const& t);

template<typename T>
T deserialize(std::span<uint8_t const> buffer);

namespace detail
{

template<typename T>
consteval size_t serialized_sizeof()
{
  if constexpr (std::is_class_v<T>)
  {
    size_t sum = 0u;
    std::apply([&](auto&& ... args) {
      ((sum += sizeof(underlying_member_t<typeof(args)>)), ...);
    }, descriptor_of<T>());

    return sum;
  }
  return sizeof(T);
}

struct Serializer final
{
public:
  template<typename T>
  void write(T const& t)
  {
    std::vector<uint8_t> const to_write = serialize(t);
    m_buffer.insert(m_buffer.end(), to_write.cbegin(), to_write.cend());
  }

  std::vector<uint8_t> result() const
  {
    return m_buffer;
  }

private:
  std::vector<uint8_t> m_buffer {};
};

struct Deserializer final
{
public:
  explicit Deserializer(std::span<uint8_t const> buffer)
    : m_buffer(buffer)
  {}

  template<typename T, typename MemberT>
  void read(T& t, MemberT T::* member)
  {
    t.*member = deserialize<MemberT>(m_buffer);
    m_buffer = m_buffer.subspan(serialized_sizeof<MemberT>() + sizeof(uint32_t));
  }

private:
  std::span<uint8_t const> m_buffer;
};

} // namespace detail

template<typename T>
std::vector<uint8_t> serialize(T const& t)
{
  if constexpr (std::is_class_v<T>)
  {
    detail::Serializer s;

    std::apply([&](auto&& ... args) {
      ((s.write(t.*args)), ...);
    }, descriptor_of<T>());

    return s.result();
  }
  else
  {
    uint32_t const tag = tag_of<T>();
    std::vector<uint8_t> buffer {};
    buffer.reserve(sizeof(tag) + sizeof(T));

    uint8_t const * begin = reinterpret_cast<uint8_t const*>(&tag);
    uint8_t const * end = begin + sizeof(tag);

    buffer.insert(buffer.end(), begin, end);

    begin = reinterpret_cast<uint8_t const*>(&t);
    end = begin + sizeof(T);

    buffer.insert(buffer.end(), begin, end);

    return buffer;
  }
}

template<typename T>
T deserialize(std::span<uint8_t const> buffer)
{
  if constexpr (std::is_class_v<T>)
  {
    detail::Deserializer d(buffer);

    T result;
    std::apply([&](auto&& ... args) {
      ((d.read(result, args)), ...);
    }, descriptor_of<T>());

    return result;
  }
  else
  {
    uint32_t tag;
    auto begin = buffer.begin();

    std::copy(
      begin,
      begin + sizeof(uint32_t),
      reinterpret_cast<uint8_t*>(&tag));

    if (tag != tag_of<T>())
    {
      // TODO better error message
      throw std::logic_error("Mismatching metadata");
    }

    T result;
    begin += sizeof(uint32_t);

    std::copy(
      begin,
      begin + sizeof(T),
      reinterpret_cast<uint8_t*>(&result));

    return result;
  }
}

template<typename T>
T deserialize(std::vector<uint8_t> const& buffer)
{
  return deserialize<T>(std::span(buffer.data(), buffer.size()));
}
