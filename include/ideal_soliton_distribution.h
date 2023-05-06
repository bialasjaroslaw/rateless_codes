#pragma once

#include <cstddef>
#include <cstdint>

#include "degree_distribution.h"
#include "well512.h"

namespace Codes::Fountain {
class IdealSolitonDistribution : public DegreeDistribution
{
public:
    IdealSolitonDistribution();
    virtual ~IdealSolitonDistribution() = default;

    void set_seed(uint32_t seed) override;
    void set_input_size(size_t input_symbols) override;
    size_t symbol_degree() override;
    std::vector<double> expected_distribution(size_t input_symbols) override;

private:
    well_512 _degree_dist;
    size_t _input_size = 0;
};
} // namespace Codes::Fountain