#pragma once

#include "lib/type_traits.hh"
#include "serde/descriptor.hh"
#include "serde/tag.hh"
#include <tuple>

namespace serde
{

namespace detail
{

template<typename T>
consteval size_t packed_sizeof()
{
  if constexpr (std::is_class_v<T>)
  {
    size_t sum = 0u;
    detail::foreach_member_of<T>([&](auto&& pointer_to_member)
    {
      using Member = lib::remove_member_pointer_t<typeof(pointer_to_member)>;
      sum += packed_sizeof<Member>();
    });
    return sum;
  }
  else
  {
    return sizeof(T);
  }
}

template<typename T>
consteval size_t serialized_sizeof()
{
  if constexpr (std::is_class_v<T>)
  {
    auto const descriptor = descriptor_of<T>();
    size_t const member_count = std::tuple_size_v<typeof(descriptor)>;
    return packed_sizeof<T>() + sizeof(Tag) * member_count;
  }
  else
  {
    return packed_sizeof<T>() + sizeof(Tag);
  }
}

template<typename T>
struct packed_sizeof_impl
{
  static constexpr auto value = detail::packed_sizeof<T>();
};

template<typename T>
struct serialized_sizeof_impl
{
  static constexpr auto value = detail::serialized_sizeof<T>();
};

} // namespace detail

template<typename T>
constexpr auto packed_sizeof = detail::packed_sizeof_impl<T>::value;

template<typename T>
constexpr auto serialized_sizeof = detail::serialized_sizeof_impl<T>::value;

} // namespace serde