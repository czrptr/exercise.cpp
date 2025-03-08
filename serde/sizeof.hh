#pragma once

#include "lib/type_traits.hh"
#include "serde/descriptor.hh"
#include "serde/tag.hh"

namespace serde
{

namespace detail
{

template<typename T>
consteval size_t packed_sizeof_impl()
{
  if constexpr (std::is_class_v<T>)
  {
    size_t sum = 0u;
    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
      sum += packed_sizeof_impl<Member>();
    });
    return sum;
  }
  else
  {
    return sizeof(T);
  }
}

template<typename T>
consteval size_t serialized_sizeof_impl()
{
  if constexpr (std::is_class_v<T>)
  {
    size_t sum = sizeof(Tag);
    detail::foreach_member_of<T>([&](auto pointer_to_member)
    {
      using Member = lib::remove_member_pointer<typeof(pointer_to_member)>;
      sum += serialized_sizeof_impl<Member>();
    });
    return sum;
  }
  else
  {
    return packed_sizeof_impl<T>() + sizeof(Tag);
  }
}

} // namespace detail

template<typename T>
constexpr auto packed_sizeof = detail::packed_sizeof_impl<T>();

template<typename T>
constexpr auto serialized_sizeof = detail::serialized_sizeof_impl<T>();

} // namespace serde