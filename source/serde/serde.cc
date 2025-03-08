#include "serde/serde.hh"

namespace serde::detail
{

#ifdef RTTI_PRESENT
std::unordered_map<Tag, std::string>& typename_of_tag()
{
  static std::unordered_map<Tag, std::string> map;
  return map;
}
#endif

} // namespace serde::detail