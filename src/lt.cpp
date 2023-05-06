#include "lt.h"

#include <cstring>
#include <set>
#include <span>

#include <spdlog/spdlog.h>

namespace Codes::Fountain {

LT::LT(DegreeDistribution* distribution)
    : _degree_dist(distribution)
{}

LT::~LT()
{
    if (_owner)
        delete[] _input_data;
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
    _encoded_nodes.reserve(1.2 * _input_symbols);
    _samples.reserve(_input_symbols);
    _current_hash_bits.reserve(_input_symbols);
    _degree_dist->set_input_size(_input_symbols);

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
    ++_current_symbol;

    for (auto idx = 0; idx < _current_hash_bits.size(); ++idx)
    {
        input = _input_data + _current_hash_bits[idx] * _symbol_length;
        for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
            ptr[sym_idx] ^= input[sym_idx];
    }

    return ptr;
}

void LT::set_seed(uint32_t seed)
{
    _generator.set_seed(seed);
    _degree_dist->set_seed(seed);
}

size_t LT::symbol_degree()
{
    return _degree_dist->symbol_degree();
}

void LT::shuffle_input_symbols(bool discard)
{
    select_symbols(symbol_degree(), _input_symbols, discard);
}

void LT::select_symbols(size_t num, size_t max, bool discard)
{
    _current_hash_bits.clear();
    std::set<uint32_t> choosen;
    while (choosen.size() < num)
    {
        auto value = _generator() % _samples.size();
        choosen.insert(value);
    }
    for (const auto& val : choosen)
        _current_hash_bits.push_back(val);
}

bool LT::feed_symbol(char* ptr, size_t number, Memory mem, Decoding dec)
{
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("=== FEED BEGIN ===");
    print_hash_matrix();
#endif
    while (_current_symbol != number + 1)
    {
        shuffle_input_symbols(_current_symbol != number);
        ++_current_symbol;
    }
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("Received symbol {} connected to {}", number, fmt::join(_current_hash_bits, ", "));
    spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr + 1)));
#endif
    Node node(ptr, _symbol_length, mem);
    node.init_edges(std::vector<size_t>(_current_hash_bits.cbegin(), _current_hash_bits.cend()));
    ptr = node.get_data();
    for (const auto& input_node_num : _current_hash_bits)
    {
        if (_data_nodes[input_node_num].is_known())
        {
#if defined(ENABLE_TRACE_LOG)
            spdlog::trace("Reducing symbol {}, data {} already known", number, input_node_num);
            spdlog::trace("Befeore {} connected with {}", number, fmt::join(node.edges, ", "));
            spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr),
                          static_cast<unsigned char>(*(ptr + 1)));
#endif
            node.erase_edge(input_node_num);
            for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                ptr[sym_idx] ^= _data_nodes[input_node_num][sym_idx];
#if defined(ENABLE_TRACE_LOG)
            spdlog::trace("After {} connected with {}", number, fmt::join(node.edges, ", "));
            spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr),
                          static_cast<unsigned char>(*(ptr + 1)));
#endif
        }
        _data_nodes[input_node_num].add_edge(number);
    }

    if (node.edges_num() == 1)
    {
#if defined(ENABLE_TRACE_LOG)
        spdlog::trace("Symbol {} has degree 1, schedule decoding", number);
#endif
        _encoded_queue.push_back(_encoded_nodes.size());
    }
    _encoded_nodes.push_back(std::move(node));
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("=== FEED END ===");
    print_hash_matrix();
    spdlog::trace("=== FEED DONE ===");
#endif
    return dec == Decoding::Start && decode();
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
#if defined(ENABLE_TRACE_LOG)
                spdlog::trace("=== STEP ===");
#endif
                process_encoded_node(idx);
#if defined(ENABLE_TRACE_LOG)
                print_hash_matrix();
                spdlog::trace("=== END ===");
#endif
            }
            for (auto idx : tmp_data_queue)
            {
#if defined(ENABLE_TRACE_LOG)
                spdlog::trace("=== STEP ===");
#endif
                process_input_node(idx);
#if defined(ENABLE_TRACE_LOG)
                print_hash_matrix();
                spdlog::trace("=== END ===");
#endif
            }
        }
    }
    return _unknown_blocks == 0;
}

void LT::process_encoded_node(size_t num)
{
    Node& node = _encoded_nodes[num];
    if (node.edges_num() != 1)
        return;
    auto edge = node.edge_at(0);
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("Releasing encoded {}, connected to {}", num, edge);
#endif
    node.clear_edges();
    if (_data_nodes[edge].is_known())
    {
#if defined(ENABLE_TRACE_LOG)
        spdlog::trace("Data {} already known, skip", edge);
#endif
        return;
    }

#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("Decoding data at {}", edge);
    spdlog::trace("Befeore, data {} connected with {}", edge, fmt::join(_data_nodes[edge].edges, ", "));
#endif
    _data_nodes[edge].swap_with(node);
    _data_nodes[edge].make_known();
    _data_nodes[edge].erase_edge(num);
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("After, data {} connected with {}", edge, fmt::join(_data_nodes[edge].edges, ", "));
    auto dd = _data_nodes[edge].data.get();
    spdlog::trace("Decoded data {}: {:#x}, {:#x}", edge, static_cast<unsigned char>(*dd),
                  static_cast<unsigned char>(*(dd + 1)));
#endif
    --_unknown_blocks;
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("Schedule data {} release", edge);
#endif
    _data_queue.push_back(edge);
}

void LT::process_input_node(size_t num)
{
#if defined(ENABLE_TRACE_LOG)
    spdlog::trace("Data {} connected with {}", num, fmt::join(_data_nodes[num].edges, ", "));
#endif
    for (const auto& edge : _data_nodes[num].edges())
    {
        auto& droplet = _encoded_nodes[edge];
#if defined(ENABLE_TRACE_LOG)
        auto droplet_degree = droplet.edges.size();
        auto ptr = droplet.data.get();
        spdlog::trace("Reducing encoded {} degree from {} to {}", edge, droplet_degree + 1, droplet_degree);
        spdlog::trace("Before: {}", fmt::join(droplet.edges, ", "));
        spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr + 1)));
#endif
        droplet.erase_edge(num);

        if (droplet.edges_num() == 0)
        {
#if defined(ENABLE_TRACE_LOG)
            spdlog::trace("Encoded {} not connected with anything, skip", edge);
#endif
            continue;
        }
        else
        {
            for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                droplet[sym_idx] ^= _data_nodes[num][sym_idx];
        }
#if defined(ENABLE_TRACE_LOG)
        spdlog::trace("After: {}", fmt::join(droplet.edges, ", "));
        spdlog::trace("Data: {:#x} {:#x}", static_cast<unsigned char>(*ptr), static_cast<unsigned char>(*(ptr + 1)));
#endif

        if (_encoded_nodes[edge].edges_num() == 1)
        {
#if defined(ENABLE_TRACE_LOG)
            spdlog::trace("Encoded {} with degree 1, schedule for release", num);
#endif
            _encoded_queue.push_back(edge);
        }
    }
    _data_nodes[num].clear_edges();
}

char* LT::decoded_buffer()
{
    auto buffer = new char[_input_data_size];
    for (auto idx = 0; idx < _input_symbols; ++idx)
        memcpy(buffer + idx * _symbol_length, _data_nodes[idx].get_data(), _symbol_length);
    return buffer;
}

void LT::print_hash_matrix()
{
    spdlog::trace("Input nodes");
    for (auto idx = 0; idx < _data_nodes.size(); ++idx)
    {
        auto dd = _data_nodes[idx].get_data();
        if (_data_nodes[idx].is_known())
            spdlog::trace("{} K {:#x} {:#x}", idx, static_cast<unsigned char>(*dd),
                          static_cast<unsigned char>(*(dd + 1)));
        else
            spdlog::trace("{} {}", idx, fmt::join(_data_nodes[idx].edges(), ", "));
    }
    spdlog::trace("Encoded nodes");
    for (auto idx = 0; idx < _encoded_nodes.size(); ++idx)
    {
        auto dd = _encoded_nodes[idx].get_data();
        if (dd != nullptr)
            spdlog::trace("{} {} {:#x} {:#x}", idx, fmt::join(_encoded_nodes[idx].edges(), ", "),
                          static_cast<unsigned char>(*dd), static_cast<unsigned char>(*(dd + 1)));
        else
            spdlog::trace("{} {}", idx, fmt::join(_encoded_nodes[idx].edges(), ", "));
    }
}
} // namespace Codes::Fountain