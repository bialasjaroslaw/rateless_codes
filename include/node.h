#pragma once

#include <memory>
#include <vector>

namespace Codes::Fountain {

enum class Memory{
    Owner,
    MakeCopy,
    View
};

struct Node
{
    Node() = default;
    Node(char* ptr, size_t data_length, Memory mem = Memory::MakeCopy);
    Node(Node&&) = default;
    ~Node();
    void init_edges(std::vector<size_t> edges);
    const std::vector<size_t>& edges() const;
    char& operator[](size_t idx);
    void erase_edge(size_t edge);
    void add_edge(size_t edge);
    bool is_known() const;
    char* get_data() const;
    size_t edges_num() const;
    void clear_edges();
    size_t edge_at(size_t idx) const;
    void make_known();
    void swap_with(Node& other);

private:
    std::vector<size_t> _edges;
    std::unique_ptr<char[]> _data;
    bool _known = false;
    bool _owner = true;
};
} // namespace Codes::Fountain