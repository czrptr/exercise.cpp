#include "serde/common.hh"

namespace serde::detail
{

std::unordered_map<Tag, std::string>& typename_of_tag()
{
  static std::unordered_map<Tag, std::string> map;
  return map;
}

} // namespace serde::detail