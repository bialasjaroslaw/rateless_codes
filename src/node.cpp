#include "node.h"

#include <algorithm>
#include <cstring>

namespace Codes::Fountain {
Node::Node(char* ptr, size_t data_length, Memory mem)
{
    if (ptr && mem == Memory::MakeCopy)
    {
        _data.reset(new char[data_length]);
        memcpy(_data.get(), ptr, data_length);
    }
    else
        _data.reset(ptr);
    _owner = mem != Memory::View;
}

Node::~Node()
{
    if (!_owner)
        _data.release();
}

void Node::init_edges(std::vector<size_t> edges)
{
    this->_edges = std::move(edges);
}

const std::vector<size_t>& Node::edges() const
{
    return _edges;
}

char& Node::operator[](size_t idx)
{
    return _data.get()[idx];
}

void Node::erase_edge(size_t edge)
{
    auto it = std::find(_edges.begin(), _edges.end(), edge);
    if (it == _edges.end())
        return;
    _edges.erase(it);
}

void Node::add_edge(size_t edge)
{
    _edges.push_back(edge);
}

bool Node::is_known() const
{
    return _known;
}

char* Node::get_data() const
{
    return _data.get();
}

size_t Node::edges_num() const
{
    return _edges.size();
}

void Node::clear_edges()
{
    _edges.clear();
}

size_t Node::edge_at(size_t idx) const
{
    return _edges[idx];
}

void Node::make_known()
{
    _known = true;
}

void Node::swap_with(Node& other)
{
    std::swap(_data, other._data);
    std::swap(_owner, other._owner);
}
} // namespace Codes::Fountain