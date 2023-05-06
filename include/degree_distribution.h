#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace Codes::Fountain {
class DegreeDistribution
{
public:
    DegreeDistribution() = default;
    virtual ~DegreeDistribution() = default;

    virtual void set_seed(uint32_t seed) = 0;
    virtual void set_input_size(size_t input_symbols) = 0;
    virtual size_t symbol_degree() = 0;
    virtual std::vector<double> expected_distribution(size_t input_symbols) = 0;
};
} // namespace Codes::Fountain