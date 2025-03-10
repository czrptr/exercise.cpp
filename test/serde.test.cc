#include <gtest/gtest.h>

#include "serde/deserialize.hh"
#include "serde/serialize.hh"
#include "serde/tag.hh"

// TODO: test with MSVC

enum class YesOrNo
{
  Yes,
  No
};

struct Data
{
  char n;
  double d;
  Data* next;

  friend bool operator==(Data const& lhs, Data const& rhs)
  {
    return lhs.n == rhs.n && lhs.d == rhs.d &&
      ((lhs.next == nullptr && rhs.next == nullptr) || (*rhs.next == *rhs.next));
  }
};

template <>
constexpr auto descriptor_of<Data>()
{
  return std::make_tuple(&Data::n, &Data::d, &Data::next);
}

struct Node
{
  char a;
  int b;
  float c;
  Data d;
  YesOrNo e;

  friend bool operator==(Node const& lhs, Node const& rhs) = default;
};

template <>
constexpr auto descriptor_of<Node>()
{
  return std::make_tuple(&Node::a, &Node::b, &Node::c, &Node::d, &Node::e);
}

template <typename T>
T serialize_and_deserialize(T const& t)
{
  return serde::deserialize<T>(serde::serialize(t));
}

template <typename T>
std::pair<serde::Tag, T> serialize_and_deserialize_with_tag(T const& t)
{
  auto const bytes = serde::detail::serialize_raw_with_tag(t);
  auto span = std::span(bytes.data(), bytes.size());
  return serde::detail::deserialize_with_tag<T>(span);
}

TEST(Serde, builtins)
{
  EXPECT_EQ(24, serialize_and_deserialize(24));
  EXPECT_EQ('c', serialize_and_deserialize('c'));
  EXPECT_EQ(false, serialize_and_deserialize(false));
  EXPECT_EQ(24.24, serialize_and_deserialize(24.24));
}

TEST(Serde, builtins_with_tags)
{
  EXPECT_EQ(std::pair(serde::tag_of<int>(), 24), serialize_and_deserialize_with_tag(24));
  EXPECT_EQ(std::pair(serde::tag_of<char>(), 'c'), serialize_and_deserialize_with_tag('c'));
  EXPECT_EQ(std::pair(serde::tag_of<bool>(), false), serialize_and_deserialize_with_tag(false));
  EXPECT_EQ(std::pair(serde::tag_of<double>(), 24.24), serialize_and_deserialize_with_tag(24.24));
}

TEST(Serde, pointers)
{
  auto value1 = new int{3};
  EXPECT_EQ(*value1, *serialize_and_deserialize(value1));

  auto value2 = new double{7.0};
  EXPECT_EQ(*value2, *serialize_and_deserialize(value2));

  auto value3 = new Data{'z', 5.4321, nullptr};
  EXPECT_EQ(*value3, *serialize_and_deserialize(value3));
}

TEST(Serde, structs)
{
  auto a = Data{'z', 5.4321, nullptr};
  auto b = Data{'h', 43.66, &a};
  auto value1 = Data{'a', 999.999, &b};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  auto const value2 = Node{'c', 24, 63.0f, {'a', 999.999, &value1}, YesOrNo::Yes};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}

TEST(Serde, vectors_of_builtins)
{
  std::vector<int> const value1 = {0, 1, 2, 3, 4};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  std::vector<float> const value2 = {0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}

TEST(Serde, vectors_of_structs)
{
  auto c = Data{'l', 69.420, nullptr};
  auto d = Data{'d', 420.69, &c};
  std::vector<Data> const value1 = {d, d, d, d};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  auto const n = Node{'c', 24, 63.0f, {'a', 999.999, &d}, YesOrNo::Yes};
  std::vector<Node> const value2 = {n, n, n};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}

TEST(Serde, metadata_mismatch)
{
  auto const bytes = [&]()
  {
    using namespace serde;
    std::vector<std::vector<std::byte>> bytes;

    bytes.push_back(detail::serialize_raw(tag_of<Data>()));
    bytes.push_back(detail::serialize_raw(tag_of<char>()));
    bytes.push_back(detail::serialize_raw('a'));
    bytes.push_back(detail::serialize_raw(tag_of<int>()));
    bytes.push_back(detail::serialize_raw(999.999));

    return bytes | ranges::views::join | ranges::to<std::vector>;
  }();

  try
  {
    serde::deserialize<Data>(bytes);
    FAIL();
  }
  catch (std::logic_error error)
  {
    EXPECT_STREQ("Metadata mismatch: expecting 'double' but found 'int' starting at byte 17", error.what());
  }
}
