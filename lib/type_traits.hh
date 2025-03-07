#pragma once

#include <type_traits> // IWYU pragma: export

namespace lib
{

template<typename T>
struct remove_member_pointer;

template<typename StructT, typename MemberT>
struct remove_member_pointer<MemberT StructT::*>
{
  using type = MemberT;
};

template<typename StructT, typename MemberT>
struct remove_member_pointer<MemberT StructT::* const>
{
  using type = MemberT;
};

template<typename T>
using remove_member_pointer_t = remove_member_pointer<T>::type;

} // namespace lib