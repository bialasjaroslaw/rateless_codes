#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

#include "degree_distribution.h"

namespace Codes::Fountain {
class IdealSolitonDistribution : public DegreeDistribution
{
public:
    IdealSolitonDistribution();
    virtual ~IdealSolitonDistribution() = default;
    
    void set_seed(uint32_t seed) override;
    size_t symbol_degree(size_t input_symbols) override;
    
    std::default_random_engine _random_engine;
    std::uniform_real_distribution<double> _degree_dist;
};
} // namespace Codes::Fountain