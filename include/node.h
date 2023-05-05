#pragma once

#include <memory>
#include <set>

namespace Codes::Fountain {
struct Node
{
    Node() = default;
    Node(char* ptr, size_t data_length, bool deep_copy = false);
    Node(Node&&) = default;
    ~Node();
    char& operator[](size_t idx);
    std::set<size_t> edges;
    std::unique_ptr<char[]> data;
    bool known = false;
    bool owner = true;
};
} // namespace Codes::Fountain