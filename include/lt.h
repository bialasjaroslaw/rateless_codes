#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <cstring>

#include "degree_distribution.h"
#include "node.h"
#include "well512.h"

namespace Codes::Fountain {

enum class Decoding
{
    Postpone,
    Start
};

class LT
{
public:
    explicit LT(DegreeDistribution* distribution);
    virtual ~LT();

    void set_input_data(char* ptr, size_t len, bool deep_copy = false);
    void set_symbol_length(size_t len);
    void set_input_data_size(size_t len);
    char* generate_symbol();
    void set_seed(uint32_t seed);
    size_t symbol_degree();
    void shuffle_input_symbols(bool discard = false);
    void select_symbols(size_t num, size_t max, bool discard = false);

    bool feed_symbol(char* ptr, size_t number, Memory mem = Memory::MakeCopy, Decoding dec = Decoding::Start);
    bool decode(bool allow_partial = false);
    void process_encoded_node(size_t num);
    void process_input_node(size_t num);

    char* decoded_buffer();

    void print_hash_matrix();

    std::unique_ptr<DegreeDistribution> _degree_dist;
    size_t _symbol_length = 0;
    size_t _input_symbols = 0;
    size_t _input_data_size = 0;
    char* _input_data = nullptr;
    bool _owner = false;

    std::vector<uint32_t> _current_hash_bits;
    std::vector<uint32_t> _samples;
    size_t _current_symbol = 0;

    well_512 _generator;

    std::vector<Node> _data_nodes;
    std::vector<Node> _encoded_nodes;

    std::vector<size_t> _data_queue;
    std::vector<size_t> _encoded_queue;

    size_t _unknown_blocks = 0;
};
} // namespace Codes::Fountain