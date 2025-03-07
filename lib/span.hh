#pragma once

#include <span>
#include <utility>

namespace lib
{

template<typename T>
std::pair<std::span<T>, std::span<T>> split(std::span<T> span, size_t index)
{
  return {
    span.subspan(0, index),
    span.subspan(index)
  };
}

} // namespace lib