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
    _samples.reserve(_input_symbols);

    for (size_t idx = 0; idx < _input_symbols; ++idx)
    {
        _data_nodes.emplace_back(nullptr, _symbol_length);
        _samples.push_back(idx);
    }

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
    select_symbols(symbol_degree(), _input_symbols, discard);
}

void LT::select_symbols(size_t num, size_t max, bool discard)
{
    _current_hash_bits.clear();
    std::sample(_samples.cbegin(), _samples.cend(), std::back_inserter(_current_hash_bits), num, _random_engine);

    /*
    if (!discard)
    {
        _current_hash_bits.resize(num);
        for (auto idx = 0 ; idx < max ; ++idx)
            _current_hash_bits[idx] = idx;
    }

    std::uniform_int_distribution<int> uniform_dist(0, max - 1);
    for (auto idx = 0 ; idx < max; ++idx)
    {
        auto out_idx = uniform_dist(_random_engine);
        if (!discard)
            std::swap(_current_hash_bits[out_idx], _current_hash_bits[idx]);
    }
    */
}

void LT::feed_symbol(char* ptr, size_t number, bool deep_copy, bool start_decoding)
{
    spdlog::trace("=== FEED ===");
    print_hash_matrix();
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

    spdlog::trace("Received symbol {} connected to {}", number, fmt::join(_current_hash_bits, ", "));
    spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr+1)));

    Node node(ptr, _symbol_length, deep_copy);
    node.edges = std::set<size_t>(_current_hash_bits.cbegin(), _current_hash_bits.cend());
    ptr = node.data.get();
    for (const auto& input_node_num : _current_hash_bits)
    {
        if (_data_nodes[input_node_num].known)
        {
            spdlog::trace("Reducing symbol {}, data {} already known", number, input_node_num);
            spdlog::trace("Befeore {} connected with {}", number, fmt::join(node.edges, ", "));
            spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr + 1)));
            node.edges.erase(input_node_num);
            for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                ptr[sym_idx] ^= _data_nodes[input_node_num][sym_idx];
            spdlog::trace("After {} connected with {}", number, fmt::join(node.edges, ", "));
            spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr+1)));
        }
        _data_nodes[input_node_num].edges.insert(number);
    }

    if (node.edges.size() == 1)
    {
        spdlog::trace("Symbol {} has degree 1, schedule decoding", number);
        _encoded_queue.push_back(_encoded_nodes.size());
    }
    _encoded_nodes.push_back(std::move(node));

    spdlog::trace("=== FEED END ===");
    print_hash_matrix();
    spdlog::trace("=== FEED DONE ===");

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
            {
                spdlog::trace("=== STEP ===");
                process_encoded_node(idx);
                print_hash_matrix();
                spdlog::trace("=== END ===");
            }
            for (auto idx : tmp_data_queue)
            {
                spdlog::trace("=== STEP ===");
                process_input_node(idx);
                print_hash_matrix();
                spdlog::trace("=== END ===");
            }
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
    spdlog::trace("Releasing encoded {}, connected to {}", num, edge);
    node.edges.clear();
    if (_data_nodes[edge].known)
    {
        spdlog::trace("Data {} already known, skip", edge);
        return;
    }

    spdlog::trace("Decoding data at {}", edge);
    spdlog::trace("Befeore, data {} connected with {}", edge, fmt::join(_data_nodes[edge].edges, ", "));
    std::swap(_data_nodes[edge].data, node.data);
    std::swap(_data_nodes[edge].owner, node.owner);
    _data_nodes[edge].known = true;
    _data_nodes[edge].edges.erase(num);
    spdlog::trace("After, data {} connected with {}", edge, fmt::join(_data_nodes[edge].edges, ", "));
    auto dd = _data_nodes[edge].data.get();
    spdlog::trace("Decoded data {}: {:#x}, {:#x}", edge,
        static_cast<unsigned char>(*dd), static_cast<unsigned char>(*(dd+1)));
    --_unknown_blocks;
    spdlog::trace("Schedule data {} release", edge);
    _data_queue.push_back(edge);
}

void LT::process_input_node(size_t num)
{
    spdlog::trace("Data {} connected with {}", num, fmt::join(_data_nodes[num].edges, ", "));
    for (const auto& edge : _data_nodes[num].edges)
    {
        auto& droplet = _encoded_nodes[edge];
        auto droplet_degree = droplet.edges.size();
        auto ptr = droplet.data.get();
        spdlog::trace("Reducing encoded {} degree from {} to {}", edge, droplet_degree + 1, droplet_degree);
        spdlog::trace("Before: {}", fmt::join(droplet.edges, ", "));
        spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr + 1)));
        droplet.edges.erase(num);

        if (droplet.edges.size() == 0)
        {
            spdlog::trace("Encoded {} not connected with anything, skip", edge);
            continue;
        }
        else
        {
            for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                droplet[sym_idx] ^= _data_nodes[num][sym_idx];
        }
        spdlog::trace("After: {}", fmt::join(droplet.edges, ", "));
        spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr + 1)));

        if (_encoded_nodes[edge].edges.size() == 1)
        {
            spdlog::trace("Encoded {} with degree 1, schedule for release", num);
            _encoded_queue.push_back(edge);
        }
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
    spdlog::trace("Input nodes");
    for (auto idx = 0; idx < _data_nodes.size(); ++idx)
    {
        auto dd = _data_nodes[idx].data.get();
        if(_data_nodes[idx].known)
            spdlog::trace("{} K {:#x} {:#x}", idx, static_cast<unsigned char>(*dd), static_cast<unsigned char>(*(dd+1)));
        else
            spdlog::trace("{} {}", idx, fmt::join(_data_nodes[idx].edges, ", "));
    }
    spdlog::trace("Encoded nodes");
    for (auto idx = 0; idx < _encoded_nodes.size(); ++idx)
    {
        auto dd = _encoded_nodes[idx].data.get();
        if (dd != nullptr)
            spdlog::trace("{} {} {:#x} {:#x}", idx, fmt::join(_encoded_nodes[idx].edges, ", "), static_cast<unsigned char>(*dd), static_cast<unsigned char>(*(dd + 1)));
        else
            spdlog::trace("{} {}", idx, fmt::join(_encoded_nodes[idx].edges, ", "));
    }
}
} // namespace Codes::Fountain