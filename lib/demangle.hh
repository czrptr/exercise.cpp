#pragma once

#include <string>
#include <typeinfo>

namespace lib
{

std::string demangle(char const* mangled_name);

template <typename T>
inline std::string nameof()
{
  return demangle(typeid(T).name());
}

} // namespace lib