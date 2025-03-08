#include "lib/demangle.hh"

#include <string>
#include <cstdlib>

#if defined(__clang__) || defined(__GNUC__)
#include <cxxabi.h>
#elif defined(_MSC_VER)
#include <dbghelp.h>
#endif

namespace lib
{

std::string demangle(const char* mangled_name)
{
#if defined(__clang__) || defined(__GNUC__)
  int status = 0;
  char* real_name = abi::__cxa_demangle(mangled_name, nullptr, nullptr, &status);
  if (status == 0)
  {
    std::string result(real_name);
    std::free(real_name);
    return result;
  }
  return mangled_name;
#elif defined(_MSC_VER)
  char buffer[1024] = {0};
  if (UnDecorateSymbolName(mangled_name, buffer, sizeof(buffer), UNDNAME_COMPLETE))
  {
    return buffer;
  }
  return mangled_name;
#else
  return mangled_name;
#endif
}

} // namespace lib