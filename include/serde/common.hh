#pragma once

#include <bit>
#include <unordered_map>

#include <range/v3/all.hpp>

#include "serde/tag.hh"

namespace serde::detail
{

enum class SerializedPointer : uint8_t
{
  IsNull = 0u,
  IsPresent = 1u,
};

std::unordered_map<Tag, std::string>& typename_of_tag();

inline constexpr auto little_endian_order()
{
  if constexpr (std::endian::native == std::endian::little)
  {
    return ranges::views::all;
  }
  else // std::endian::native == std::endian::big
  {
    return ranges::views::reverse;
  }
}

} // namespace serde::detail