#include <gtest/gtest.h>

#include "serde/serde.hh"

struct Data
{
  char n;
  double d;

  friend auto operator<=>(Data const& lhs, Data const& rhs) = default;
};

template<>
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

  friend auto operator<=>(Node const& lhs, Node const& rhs) = default;
};

template<>
constexpr auto descriptor_of<Node>()
{
  return std::make_tuple(&Node::a, &Node::b, &Node::c, &Node::d);
}

template<typename T>
T serialize_and_deserialize(T const& t)
{
  return serde::deserialize<T>(serde::serialize(t));
}

template<typename T>
std::pair<serde::Tag, T> serialize_and_deserialize_with_tag(T const& t)
{
  return serde::detail::deserialize_with_tag<T>(serde::detail::serialize_with_tag(t));
}

TEST(Serde, packed_sizeof)
{
  EXPECT_EQ(
    sizeof(char) + sizeof(double),
    serde::packed_sizeof<Data>);

  EXPECT_EQ(
    2 * sizeof(char) + sizeof(int) + sizeof(float) + sizeof(double),
    serde::packed_sizeof<Node>);
}

TEST(Serde, serialized_sizeof)
{
  EXPECT_EQ(
    sizeof(char) + sizeof(serde::Tag),
    serde::serialized_sizeof<char>);

  EXPECT_EQ(
    sizeof(char) + sizeof(double) + 3 * sizeof(serde::Tag),
    serde::serialized_sizeof<Data>);

  EXPECT_EQ(
    2 * sizeof(char) + sizeof(int) + sizeof(float) + sizeof(double) + 7 * sizeof(serde::Tag),
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
  EXPECT_EQ(std::pair(serde::tag_of<int>, 24), serialize_and_deserialize_with_tag(24));
  EXPECT_EQ(std::pair(serde::tag_of<char>, 'c'), serialize_and_deserialize_with_tag('c'));
  EXPECT_EQ(std::pair(serde::tag_of<bool>, false), serialize_and_deserialize_with_tag(false));
  EXPECT_EQ(std::pair(serde::tag_of<double>, 24.24), serialize_and_deserialize_with_tag(24.24));
}

TEST(Serde, serialize_and_deserialize_structs)
{
  auto value1 = Data { 'a', 999.999 };
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  auto value2 = Node { 'c', 24, 63.0f, { 'a', 999.999 } };
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}