#pragma once

#include <cstddef>
#include <cstdint>

namespace Codes::Fountain {
class DegreeDistribution
{
public:
    DegreeDistribution() = default;
    virtual ~DegreeDistribution() = default;
    
    virtual void set_seed(uint32_t seed) = 0;
    virtual size_t symbol_degree(size_t input_symbols) = 0;
};
} // namespace Codes::Fountain