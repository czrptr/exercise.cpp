#pragma once

#include <bit>
#include <unordered_map>

#include <range/v3/all.hpp>

#include "serde/tag.hh"

namespace serde::detail
{

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