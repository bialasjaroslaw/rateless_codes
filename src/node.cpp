#include "node.h"

#include <cstring>

namespace Codes::Fountain {
Node::Node(char* ptr, size_t data_length, bool deep_copy)
{
    if (ptr && deep_copy)
    {
        data.reset(new char[data_length]);
        memcpy(data.get(), ptr, data_length);
        owner = true;
    }
    else
    {
        owner = false;
        data.reset(ptr);
    }
}

Node::~Node()
{
    if (!owner)
        data.release();
}

char& Node::operator[](size_t idx)
{
    return data.get()[idx];
}
} // namespace Codes::Fountain