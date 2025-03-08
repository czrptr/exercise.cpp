#pragma once

#include <span>
#include <utility>

namespace lib
{

template <typename T>
std::pair<std::span<T>, std::span<T>> split(std::span<T> span, size_t index)
{
  return {span.subspan(0, index), span.subspan(index)};
}

template <typename T>
std::span<T> take(std::span<T>& span, size_t index)
{
  auto const result = span.subspan(0, index);
  span = span.subspan(index);
  return result;
}

} // namespace lib