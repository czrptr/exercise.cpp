# Serde C++

A type-safe, reflection-based serialization and deserialization library for C++23.

## Overview

This library provides automatic serialization and deserialization of C++ types using compile-time reflection. It supports built-in types, user-defined structs, pointers, and vectors with built-in type safety through metadata tagging.

## Features

- **Automatic serialization/deserialization** of:
  - Built-in types (int, char, float, double, bool, etc.)
  - User-defined structs
  - Pointers (with null handling)
  - Vectors (including nested vectors)
- **Type safety**: Metadata tags ensure deserializing into the wrong type is caught at runtime
- **Endianness handling**: Automatically converts to/from little-endian format
- **Compile-time reflection**: Uses descriptor pattern for zero-overhead type introspection
- **Detailed error messages**: Clear error reporting when type mismatches occur

## Requirements

- C++23 compiler (Clang with libc++)
- Bazel build system
- Dependencies:
  - [range-v3](https://github.com/ericniebler/range-v3) (v0.12.0)
  - [fmt](https://github.com/fmtlib/fmt) (v11.1.4)
- Development dependencies:
  - [GoogleTest](https://github.com/google/googletest) (v1.16.0) for testing

## Building

```bash
# Build the library
bazel build //:serde
```

## Usage

### Defining Serializable Types

To make a struct serializable, define a `descriptor_of` specialization:

```cpp
#include "serde/serialize.hh"
#include "serde/deserialize.hh"

struct Person
{
  char initial;
  int age;
  double height;
};

// Define the descriptor
template <>
constexpr auto descriptor_of<Person>()
{
  return std::make_tuple(&Person::initial, &Person::age, &Person::height);
}
```

### Serialization

```cpp
Person person{'J', 30, 5.9};

// Serialize to bytes
std::vector<std::byte> bytes = serde::serialize(person);
```

### Deserialization

```cpp
// Deserialize from bytes
Person restored = serde::deserialize<Person>(bytes);
```

### Working with Complex Types

```cpp
struct Node
{
  int value;
  Node* next;
  std::vector<int> data;
};

template <>
constexpr auto descriptor_of<Node>() {
  return std::make_tuple(&Node::value, &Node::next, &Node::data);
}

// Create a linked list
Node* second = new Node{2, nullptr, {20, 21, 22}};
Node first{1, second, {10, 11, 12}};

// Serialize and deserialize
auto bytes = serde::serialize(first);
Node restored = serde::deserialize<Node>(bytes);
```

## Type Safety

The library uses hash-based type tags to ensure type safety during deserialization:

```cpp
auto bytes = serde::serialize(42);

// This will throw std::logic_error with a descriptive message
auto wrong = serde::deserialize<double>(bytes);
// Error: "Metadata mismatch: expecting 'double' but found 'int' starting at byte X"
```

## Implementation Details

### Binary Format

Each serialized value is prefixed with:
1. A type tag (64-bit hash of the type)
2. The actual data in little-endian format

For complex types:
- **Structs**: Tag + serialized members in order
- **Vectors**: Tag + size (with tag) + serialized elements
- **Pointers**: Tag + null flag (0 or 1) + serialized pointee (if non-null)

### Type Tags

Type tags are computed at compile-time using:
- String hashing for built-in types and vectors
- Recursive member hashing for user-defined structs

This ensures consistent tagging across compilation units while maintaining type safety.

## Testing

The test suite covers:
- Built-in types (int, char, bool, double)
- Pointers (including null pointers)
- User-defined structs (simple and nested)
- Vectors of built-ins and structs
- Type mismatch detection
- Missing metadata detection

Run tests with:
```bash
bazel test //:test
```