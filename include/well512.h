#pragma once

#include <cstdint>
#include <cstring>

// https://www.lomont.org/papers/2008/Lomont_PRNG_2008.pdf
struct well_512
{
    // Naive way of setting seed, but nothing better is needed for now
    void set_seed(uint32_t seed)
    {
        auto state_mem = reinterpret_cast<char*>(state);
        for (auto idx = 0u; idx < sizeof(state); idx += sizeof(seed))
            memcpy(state_mem + idx, &seed, sizeof(seed));
        index = 0;
    }

    unsigned long state[16] = {0};
    unsigned int index = 0;

    unsigned long operator()()
    {
        auto a = state[index];
        auto c = state[(index + 13) & 15];
        auto b = a ^ c ^ (a << 16) ^ (c << 15);
        c = state[(index + 9) & 15];
        c ^= (c >> 11);
        a = state[index] = b ^ c;
        auto d = a ^ ((a << 5) & 0xDA442D24UL);
        index = (index + 15) & 15;
        a = state[index];
        state[index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
        return state[index];
    }
};