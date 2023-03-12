#include "lt.h"

#include <cstring>
#include <set>
#include <span>

#include <spdlog/spdlog.h>

namespace Codes::Fountain {

LT::LT()
    : _degree_dist(std::uniform_real_distribution<double>())
{}

LT::~LT()
{
    if (_owner)
        delete[] _input_data;

    for (const auto* hash_seq : _hash_bits)
        delete[] hash_seq;
}

void LT::set_input_data(char* ptr, size_t len, bool deep_copy)
{
    if (deep_copy)
    {
        _owner = true;
        _input_data = new char[len];
        memcpy(_input_data, ptr, len);
    }
    else
        _input_data = ptr;

    set_input_data_size(len);
}

void LT::set_symbol_length(size_t len)
{
    _symbol_length = len;
    _input_symbols = _input_data_size / _symbol_length;
    _data_nodes.reserve(_input_symbols);

    for (size_t idx = 0; idx < _input_symbols; ++idx)
        _data_nodes.emplace_back(nullptr, _symbol_length);
    _unknown_blocks = _input_symbols;
}

void LT::set_input_data_size(size_t len)
{
    _input_data_size = len;
}

char* LT::generate_symbol()
{
    auto* ptr = new char[_symbol_length];
    memset(ptr, 0, _symbol_length);
    char* input = nullptr;

    shuffle_input_symbols();

    // debug only
    auto hash_sequence = new uint8_t[_input_symbols];
    memset(hash_sequence, 0, _input_symbols);
    for (auto idx = 0; idx < _current_hash_bits.size(); ++idx)
        hash_sequence[_current_hash_bits[idx]] = 1;
    ++_current_symbol;

    _hash_bits.push_back(hash_sequence);


    for (auto idx = 0; idx < _current_hash_bits.size(); ++idx)
    {
        // change this after debug remove
        input = _input_data + _current_hash_bits[idx] * _symbol_length;
        for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
            ptr[sym_idx] ^= input[sym_idx];
    }

    return ptr;
}

void LT::set_seed(uint32_t seed)
{
    _random_engine.seed(seed);
}

size_t LT::symbol_degree()
{
    auto value = 1.0 / (1.0 - _degree_dist(_random_engine));
    return value < _input_symbols ? std::ceil(value) : 1;
}

void LT::shuffle_input_symbols(bool discard)
{
    size_t degree = symbol_degree();
    auto idx = 0;

    if (!discard)
    {
        _current_hash_bits.resize(degree);
        for (; idx < degree; ++idx)
            _current_hash_bits[idx] = idx;
    }

    std::uniform_int_distribution<int> uniform_dist(0, degree);
    for (; idx < _input_symbols; ++idx)
    {
        auto out_idx = uniform_dist(_random_engine);
        if (!discard)
            _current_hash_bits[out_idx] = idx;
    }
}

void LT::feed_symbol(char* ptr, size_t number, bool deep_copy, bool start_decoding)
{
    auto hash_sequence = new uint8_t[_input_symbols];
    while (_current_symbol != number + 1)
    {
        shuffle_input_symbols(_current_symbol != number);
        ++_current_symbol;
    }
    // debug only
    memset(hash_sequence, 0, _input_symbols);
    for (auto idx = 0; idx < _current_hash_bits.size(); ++idx)
        hash_sequence[_current_hash_bits[idx]] = 1;
    _hash_bits.push_back(hash_sequence);

    Node node(ptr, _symbol_length, deep_copy);
    node.edges = std::set<size_t>(_current_hash_bits.cbegin(), _current_hash_bits.cend());
    for (const auto& input_node_num : _current_hash_bits)
        _data_nodes[input_node_num].edges.insert(number);

    _encoded_nodes.push_back(std::move(node));
    if (_current_hash_bits.size() == 1)
        _encoded_queue.push_back(_encoded_nodes.size() - 1);

    if (!start_decoding)
        return;

    decode();
}

bool LT::decode(bool)
{
    if (_unknown_blocks != 0)
    {
        while (!_data_queue.empty() || !_encoded_queue.empty())
        {
            std::vector<size_t> tmp_encoded_queue;
            std::vector<size_t> tmp_data_queue;
            std::swap(_encoded_queue, tmp_encoded_queue);
            std::swap(_data_queue, tmp_data_queue);

            for (auto idx : tmp_encoded_queue)
                process_encoded_node(idx);
            for (auto idx : tmp_data_queue)
                process_input_node(idx);
        }
    }
    return _unknown_blocks == 0;
}

void LT::process_encoded_node(size_t num)
{
    Node& node = _encoded_nodes[num];
    if (node.edges.size() != 1)
        return;
    auto edge = *node.edges.begin();
    node.edges.clear();
    if (_data_nodes[edge].known)
        return;

    std::swap(_data_nodes[edge].data, node.data);
    _data_nodes[edge].known = true;
    _data_nodes[edge].owner = node.owner;

    --_unknown_blocks;
    _data_queue.push_back(edge);
}

void LT::process_input_node(size_t num)
{
    for (const auto& edge : _data_nodes[num].edges)
    {
        auto& droplet = _encoded_nodes[edge];
        droplet.edges.erase(num);
        if (droplet.edges.size() == 0)
            continue;
        else
        {
            for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                droplet[sym_idx] ^= _data_nodes[num][sym_idx];
        }

        if (_encoded_nodes[edge].edges.size() == 1)
            _encoded_queue.push_back(edge);
    }
    _data_nodes[num].edges.clear();
}

char* LT::decoded_buffer()
{
    auto buffer = new char[_input_data_size];
    for (auto idx = 0; idx < _input_symbols; ++idx)
        memcpy(buffer + idx * _symbol_length, _data_nodes[idx].data.get(), _symbol_length);
    return buffer;
}

void LT::print_hash_matrix()
{
    for (auto idx = 0; idx < _hash_bits.size(); ++idx)
        spdlog::trace("{} {} ", idx, fmt::join(std::span(_hash_bits[idx], _input_symbols), ", "));
}
} // namespace Codes::Fountain