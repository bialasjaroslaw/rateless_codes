#include "rlf.h"

#include <cstring>
#include <span>

#include <spdlog/spdlog.h>

namespace Codes::Fountain {

RLF::RLF()
    : _uniform_dist(std::uniform_int_distribution<int>(0, 1))
{}

RLF::~RLF()
{
    if (_owner)
        delete[] _input_data;

    for (const auto* hash_seq : _hash_bits)
        delete[] hash_seq;

    for (int idx = 0 ; idx < _encoded_data.size() ; ++idx)
        if(_encoded_data_copy[idx])
            delete[] _encoded_data[idx];
}

void RLF::set_input_data(char* ptr, size_t len, bool deep_copy)
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

void RLF::set_symbol_length(size_t len)
{
    _symbol_length = len;
    _input_symbols = _input_data_size / _symbol_length;
}

void RLF::set_input_data_size(size_t len)
{
    _input_data_size = len;
}

char* RLF::generate_symbol()
{
    auto* ptr = new char[_symbol_length];
    memset(ptr, 0, _symbol_length);
    auto* input = _input_data;

    // debug only

    auto hash_sequence = new uint8_t[_input_symbols];
    for (auto idx = 0; idx < _input_symbols; ++idx)
        hash_sequence[idx] = _uniform_dist(_random_engine);
    ++_current_symbol;

    _hash_bits.push_back(hash_sequence);


    for (auto idx = 0; idx < _input_symbols; ++idx)
    {
        // change this after debug remove
        if (hash_sequence[idx])
            for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                ptr[sym_idx] ^= input[sym_idx];
        input += _symbol_length;
    };

    return ptr;
}

void RLF::set_seed(uint32_t seed)
{
    _random_engine.seed(seed);
}

void RLF::feed_symbol(char* ptr, size_t number, bool deep_copy)
{
    auto symbol = ptr;
    if (deep_copy)
    {
        symbol = new char[_symbol_length];
        memcpy(symbol, ptr, _symbol_length);
    }
    _encoded_data_copy.push_back(deep_copy);
    _encoded_data.push_back(symbol);

    while (_current_symbol != number)
    {
        for (auto idx = 0; idx < _input_symbols; ++idx)
            _uniform_dist(_random_engine);
        ++_current_symbol;
    }

    auto hash_sequence = new uint8_t[_input_symbols];
    for (auto idx = 0; idx < _input_symbols; ++idx)
        hash_sequence[idx] = _uniform_dist(_random_engine);
    ++_current_symbol;

    _hash_bits.push_back(hash_sequence);
}

bool RLF::decode(bool allow_partial)
{
    if (allow_partial == false && _hash_bits.size() < _input_symbols)
    {
        spdlog::trace("Partial decode not allowed and not enough symbols to perform full decode");
        return false;
    }

    for (auto idx = 0; idx < std::min(_input_symbols, _hash_bits.size()); ++idx)
    {
        // replace symbol to have 1 at nth position
        if (_hash_bits[idx][idx] != 1)
        {
            int swap_idx = -1;
            for (auto candidate_idx = idx + 1; candidate_idx < _hash_bits.size(); ++candidate_idx)
            {
                if (_hash_bits[candidate_idx][idx] != 0)
                {
                    swap_idx = candidate_idx;
                    break;
                }
            }

            if (swap_idx == -1)
            {
                spdlog::trace("Can not find symbol to reduce complexity for idx {}", idx);
                return false;
            }


            std::swap(_encoded_data[swap_idx], _encoded_data[idx]);
            std::swap(_hash_bits[swap_idx], _hash_bits[idx]);
        }

        // remove ones for all symbols that follows current

        for (auto following_idx = idx + 1; following_idx < _hash_bits.size(); ++following_idx)
        {
            if (_hash_bits[following_idx][idx])
            {
                for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                    _encoded_data[following_idx][sym_idx] ^= _encoded_data[idx][sym_idx];
                for (auto hash_idx = 0; hash_idx < _input_symbols; ++hash_idx)
                    _hash_bits[following_idx][hash_idx] ^= _hash_bits[idx][hash_idx];
            }
        }
    }

    spdlog::trace("after triangle");
    print_hash_matrix();

    auto valid_traingle_matrix = _encoded_data.size() >= _input_symbols;

    for (auto idx = size_t{0}; idx < std::min(_encoded_data.size(), _input_symbols); ++idx)
    {
        if (_hash_bits[idx][idx] != 1)
        {
            spdlog::trace("Invalid triangle matrix at idx {}", idx);
            valid_traingle_matrix = false;
            break;
        }
        for (auto preceding_idx = 0; preceding_idx < std::min(_encoded_data.size(), idx); ++preceding_idx)
        {
            if (_hash_bits[preceding_idx][idx])
            {
                for (auto sym_idx = 0; sym_idx < _symbol_length; ++sym_idx)
                    _encoded_data[preceding_idx][sym_idx] ^= _encoded_data[idx][sym_idx];
                for (auto hash_idx = 0; hash_idx < _input_symbols; ++hash_idx)
                    _hash_bits[preceding_idx][hash_idx] ^= _hash_bits[idx][hash_idx];
            }
        }
    }

    spdlog::trace("after back subs");
    print_hash_matrix();

    return valid_traingle_matrix;
}

char* RLF::decoded_buffer()
{
    auto buffer = new char[_input_data_size];
    for (auto idx = 0; idx < _input_symbols; ++idx)
        memcpy(buffer + idx * _symbol_length, _encoded_data[idx], _symbol_length);
    return buffer;
}

void RLF::print_hash_matrix()
{
    for (auto idx = 0; idx < _hash_bits.size(); ++idx)
        spdlog::trace("{} {} ", idx, fmt::join(std::span(_hash_bits[idx], _input_symbols), ", "));
}
} // namespace Codes::Fountain