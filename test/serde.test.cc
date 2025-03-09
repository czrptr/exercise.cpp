#include <gtest/gtest.h>

#include "serde/deserialize.hh"
#include "serde/serialize.hh"
#include "serde/sizeof.hh"
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

  friend auto operator<=>(Data const& lhs, Data const& rhs) = default;
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
  auto const bytes = serde::detail::serialize_with_tag(t);
  auto span = std::span(bytes.data(), bytes.size());
  return serde::detail::deserialize_with_tag<T>(span);
}

TEST(Serde, packed_sizeof)
{
  EXPECT_EQ(sizeof(char) + sizeof(double) + sizeof(Data*), serde::packed_sizeof<Data>);

  EXPECT_EQ(
    2 * sizeof(char) + sizeof(int) + sizeof(float) + sizeof(double) + sizeof(Data*) + sizeof(YesOrNo),
    serde::packed_sizeof<Node>);
}

TEST(Serde, serialized_sizeof)
{
  EXPECT_EQ(sizeof(char) + sizeof(serde::Tag), serde::serialized_sizeof<char>);

  EXPECT_EQ(sizeof(char) + sizeof(double) + sizeof(Data*) + 4 * sizeof(serde::Tag), serde::serialized_sizeof<Data>);

  EXPECT_EQ(
    2 * sizeof(char) + sizeof(int) + sizeof(float) + sizeof(double) + sizeof(Data*) + sizeof(YesOrNo) +
      9 * sizeof(serde::Tag),
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
  auto a = Data{'z', 5.4321, nullptr};
  auto const value1 = Data{'a', 999.999, &a};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  auto const value2 = Node{'c', 24, 63.0f, {'a', 999.999}, YesOrNo::Yes};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}

TEST(Serde, serialize_length_calculation)
{
  auto const bytes1 = serde::serialize(Data{'a', 999.999, nullptr});
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

TEST(Serde, serialize_vector_of_builtins)
{
  std::vector<int> const value1 = {0, 1, 2, 3, 4};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  std::vector<float> const value2 = {0.5f, 1.5f, 2.5f, 3.5f, 4.5f, 5.5f, 6.5f};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}

TEST(Serde, serialize_vector_of_structs)
{
  auto const d = Data{'d', 420.69, nullptr};
  std::vector<Data> const value1 = {d, d, d, d};
  EXPECT_EQ(value1, serialize_and_deserialize(value1));

  auto const n = Node{'c', 24, 63.0f, {'a', 999.999}, YesOrNo::Yes};
  std::vector<Node> const value2 = {n, n, n};
  EXPECT_EQ(value2, serialize_and_deserialize(value2));
}