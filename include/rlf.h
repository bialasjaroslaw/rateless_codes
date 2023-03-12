#pragma once

#include <cstdint>
#include <random>


namespace Codes {
namespace Fountain {
class RLF
{
public:
    RLF();
    virtual ~RLF();

    void set_input_data(char* ptr, size_t len, bool deep_copy = false);
    void set_symbol_length(size_t len);
    void set_input_data_size(size_t len);
    char* generate_symbol();
    void set_seed(uint32_t seed);
    void shuffle_input_symbols(bool discard = false);

    void feed_symbol(char* ptr, size_t number, bool deep_copy = false);
    bool decode(bool allow_partial = false);

    char* decoded_buffer();

    void print_hash_matrix();


    size_t _symbol_length = 0;
    size_t _input_symbols = 0;
    size_t _input_data_size = 0;
    char* _input_data = nullptr;
    bool _owner = false;

    std::vector<char*> _encoded_data;
    std::vector<bool> _encoded_data_copy;
    std::vector<uint8_t*> _hash_bits;
    std::vector<uint8_t> _current_hash_bits;
    size_t _current_symbol = 0;


    std::default_random_engine _random_engine;
    std::uniform_int_distribution<int> _uniform_dist;
};
} // namespace Fountain
} // namespace Codes