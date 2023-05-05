#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>
#include <random>

#include "node.h"

namespace Codes::Fountain {

class LT
{
public:
    LT();
    virtual ~LT();

    void set_input_data(char* ptr, size_t len, bool deep_copy = false);
    void set_symbol_length(size_t len);
    void set_input_data_size(size_t len);
    char* generate_symbol();
    void set_seed(uint32_t seed);
    size_t symbol_degree();
    void shuffle_input_symbols(bool discard = false);
    void select_symbols(size_t num, size_t max, bool discard = false);

    void feed_symbol(char* ptr, size_t number, bool deep_copy = false, bool start_decoding = true);
    bool decode(bool allow_partial = false);
    void process_encoded_node(size_t num);
    void process_input_node(size_t num);

    char* decoded_buffer();

    void print_hash_matrix();


    size_t _symbol_length = 0;
    size_t _input_symbols = 0;
    size_t _input_data_size = 0;
    char* _input_data = nullptr;
    bool _owner = false;

    std::vector<uint8_t*> _hash_bits;
    std::vector<uint32_t> _current_hash_bits;
    std::vector<uint32_t> _samples;
    size_t _current_symbol = 0;

    std::default_random_engine _random_engine;
    std::uniform_real_distribution<double> _degree_dist;

    std::vector<Node> _data_nodes;
    std::vector<Node> _encoded_nodes;

    std::vector<size_t> _data_queue;
    std::vector<size_t> _encoded_queue;

    size_t _unknown_blocks = 0;
};
} // namespace Codes::Fountain