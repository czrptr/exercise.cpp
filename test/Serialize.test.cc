#include <gtest/gtest.h>
#include "source/Serialize.hh"

struct SerializeTest : public testing::Test
{
  template<typename T>
  static T serialize_and_deserialize(T const& t)
  {
    return deserialize<T>(serialize(t));
  }
};

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
  Node* next;

  friend auto operator<=>(Node const& lhs, Node const& rhs) = default;
};

template<>
constexpr auto descriptor_of<Node>()
{
  return std::make_tuple(&Node::a, &Node::b, &Node::c, &Node::d, &Node::next);
}

TEST_F(SerializeTest, Builtin_Types)
{
  auto value = 24;
  EXPECT_EQ(value, serialize_and_deserialize(value));
}

TEST_F(SerializeTest, User_Defined_Types)
{
  auto value = Data { '6', 24.0 };
  EXPECT_EQ(value, serialize_and_deserialize(value));
}

TEST_F(SerializeTest, Nested_User_Defined_Types)
{
  auto node = Node {};
  auto value = Node { 'b', 32, 123.4f, { '9', 9.9999 }, &node };

  EXPECT_EQ(value, serialize_and_deserialize(value));
}
