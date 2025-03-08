#pragma once

#if defined(__GXX_RTTI) || defined(_HAS_STATIC_RTTI)
#define RTTI_PRESENT
#endif

namespace lib
{

#ifdef RTTI_PRESENT
bool const rtti_present = true;
#else
bool const rtti_present = false;
#endif

} // namespace lib