#include "serde/serde.hh"

#include <gtest/gtest.h>

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

  friend auto operator<=>(Data const& lhs, Data const& rhs) = default;
};

template <>
constexpr auto descriptor_of<Data>()
{
  return std::make_tuple(&Data::n, &Data::d);
}

struct Node
{
  char a;
  int b;
  float c;
  Data d;
  YesOrNo e;

  friend auto operator<=>(Node const& lhs, Node const& rhs) = default;
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
  return serde::detail::deserialize_with_tag<T>(serde::detail::serialize_with_tag(t));
}

TEST(Serde, packed_sizeof)
{
  EXPECT_EQ(sizeof(char) + sizeof(double), serde::packed_sizeof<Data>);

  EXPECT_EQ(
    2 * sizeof(char) + sizeof(int) + sizeof(float) + sizeof(double) + sizeof(YesOrNo), serde::packed_sizeof<Node>);
}

TEST(Serde, serialized_sizeof)
{
  EXPECT_EQ(sizeof(char) + sizeof(serde::Tag), serde::serialized_sizeof<char>);

  EXPECT_EQ(sizeof(char) + sizeof(double) + 3 * sizeof(serde::Tag), serde::serialized_sizeof<Data>);

  EXPECT_EQ(
    2 * sizeof(char) + sizeof(int) + sizeof(float) + sizeof(double) + sizeof(YesOrNo) + 8 * sizeof(serde::Tag),
    serde::serialized_sizeof<Node>);
}

TEST(Serde, serialize_and_deserialize_builtins)
{
  EXPECT_EQ(24, serialize_and_deserialize(24));
  EXPECT_EQ('c', serialize_and_deserialize('c'));
  EXPECT_EQ(false, serialize_and_deserialize(false));
  EXPECT_EQ(24.24, serialize_and_deserialize(24.24));
}

TEST(Serde, serialize_and_deserialize_builtins_with_tags)
{
  EXPECT_EQ(std::pair(serde::tag_of<int>(), 24), serialize_and_deserialize_with_tag(24));
  EXPECT_EQ(std::pair(serde::tag_of<char>(), 'c'), serialize_and_deserialize_with_tag('c'));
  EXPECT_EQ(std::pair(serde::tag_of<bool>(), false), serialize_and_deserialize_with_tag(false));
  EXPECT_EQ(std::pair(serde::tag_of<double>(), 24.24), serialize_and_deserialize_with_tag(24.24));
}

TEST(Serde, serialize_and_deserialize_structs)
{
  auto const value1 = Data{'a', 999.999};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  auto const value2 = Node{'c', 24, 63.0f, {'a', 999.999}, YesOrNo::Yes};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}

TEST(Serde, serialize_length_calculation)
{
  auto const bytes1 = serde::serialize(Data{'a', 999.999});
  EXPECT_EQ(serde::serialized_sizeof<Data>, bytes1.size());

  auto const bytes2 = serde::serialize(Node{'c', 24, 63.0f, {'a', 999.999}, YesOrNo::Yes});
  EXPECT_EQ(serde::serialized_sizeof<Node>, bytes2.size());
}

TEST(Serde, metadata_mismatch)
{
  auto const bytes = [&]()
  {
    using namespace serde;
    std::vector<std::vector<std::byte>> bytes;

    bytes.push_back(detail::serialize(tag_of<Data>()));
    bytes.push_back(detail::serialize(tag_of<char>()));
    bytes.push_back(detail::serialize('a'));
    bytes.push_back(detail::serialize(tag_of<int>()));
    bytes.push_back(detail::serialize(999.999));

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
