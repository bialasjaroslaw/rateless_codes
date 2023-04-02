#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <random>
#include <set>


namespace Codes {
namespace Fountain {

struct Node
{
    Node() = default;
    Node(char* ptr, size_t data_length, bool deep_copy = false)
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
    Node(Node&&) = default;
    ~Node()
    {
        if (!owner)
            data.release();
    }
    char& operator[](size_t idx)
    {
        return data.get()[idx];
    }
    std::set<size_t> edges;
    std::unique_ptr<char[]> data;
    bool known = false;
    bool owner = true;
};

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
} // namespace Fountain
} // namespace Codes