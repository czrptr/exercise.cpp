#include <vector>
#include <utility>
#include <type_traits>
#include <span>

#include <cstdint>
#include <cstring>

template<typename T>
std::vector<uint8_t> serialize(T const& t)
{
  if constexpr (std::is_class_v<T>)
  {
    static_assert(false, "serialized must be specialized in order to be used");
    std::unreachable();
  }

  size_t const size_of_write { sizeof(T) };
  std::vector<uint8_t> buffer {};
  buffer.reserve(size_of_write);

  uint8_t const * begin = reinterpret_cast<uint8_t const*>(&t);
  uint8_t const * end = begin + size_of_write;

  buffer.insert(buffer.begin(), begin, end);
  return buffer;
}

template<typename T>
T deserialize(std::span<uint8_t> const& buffer)
{
  if constexpr (std::is_class_v<T>)
  {
    static_assert(false, "deserialized must be specialized in order to be used");
    std::unreachable();
  }

  T result;

  std::memcpy(
    static_cast<void*>(&result),
    static_cast<void const*>(buffer.data()),
    sizeof(T));

  return result;
}

struct Serializer final
{
public:
  template<typename T>
  Serializer& write(T const& t)
  {
    std::vector<uint8_t> const to_write = serialize(t);
    m_buffer.insert(m_buffer.end(), to_write.begin(), to_write.end());
    return *this;
  }

  template<typename... Args>
  Serializer& write(Args&& ... args)
  {
    // call write on each argument
    const auto unroller = [](...){};
    unroller((write(std::forward(args)), 0)...);

    return *this;
  }

  std::vector<uint8_t> result() const
  {
    return m_buffer;
  }

private:
  std::vector<uint8_t> m_buffer {};
};

struct Deserializer final
{
public:
  explicit Deserializer(std::span<uint8_t> buffer)
    : m_buffer(buffer)
  {}

  template<typename T>
  T read()
  {
    T result = deserialize<T>(m_buffer.subspan(m_cursor));
    m_cursor += sizeof(T);
    return result;
  }

private:
  std::size_t m_cursor { 0u };
  std::span<uint8_t> m_buffer;
};

/* ============================ */

struct Data
{
  char name[4];
};

struct Node
{
  char a;
  int b;
  float c;
  Node* next;
};

template<>
std::vector<uint8_t> serialize<Node>(Node const& node)
{
  return Serializer{}
    .write(node.a)
    .write(node.b)
    .write(node.c)
    .write(node.next)
    .result();
}

template<>
Node deserialize(std::span<uint8_t> const& buffer)
{
  Deserializer d(buffer);
  return {
    d.read<char>(),
    d.read<int>(),
    d.read<float>(),
    d.read<Node*>(),
  };
}

// int main()
// {
//     auto node1 = Node { 'a', 69, 420.0f, nullptr };
//     auto node2 = Node { 'b', 32, 123.4f, &node1 };

//     auto buffer = serialize(node2);
//     std::cout << node2.a << " " << node2.b << " " << node2.c << " " << node2.next << "\n";

//     for (auto const& byte : buffer)
//     {
//         std::cout << int(byte) << " ";
//     }

//     auto read = deserialize<Node>(buffer);
//     std::cout << "\n" << read.a << " " << read.b << " " << read.c << " " << read.next;

//     // T Node::*
//     // auto ref_a = &Node::a;
//     // node.*ref_a = 'b';

//     // serialize(Data{});
// }

//   auto ref_a = &Node::a;
//   node.*ref_a = 'b';

//   get<&Node::a>(node);

// template<typename ReturnT, typename ClassT, ReturnT ClassT::*member>
// template<auto Member, typename T>
// auto get(T const& t)
// {
//     return t.*Member;
// }