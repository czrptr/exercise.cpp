#pragma once

template<typename T>
struct underlying_member;

template<typename StructT, typename MemberT>
struct underlying_member<MemberT StructT::*>
{
  using type = MemberT;
};

template<typename StructT, typename MemberT>
struct underlying_member<MemberT StructT::* const>
{
  using type = MemberT;
};

template<typename T>
using underlying_member_t = underlying_member<T>::type;

template<typename T>
constexpr auto descriptor_of()
{
  static_assert(false, "descriptor_of must be specialized in order to be used");
}
