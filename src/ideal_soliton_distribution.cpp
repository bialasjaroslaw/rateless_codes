#include "ideal_soliton_distribution.h"

namespace Codes::Fountain {

IdealSolitonDistribution::IdealSolitonDistribution()
    : _degree_dist(std::uniform_real_distribution<double>())
{}

void IdealSolitonDistribution::set_seed(uint32_t seed)
{
    _random_engine.seed(seed);
}

size_t IdealSolitonDistribution::symbol_degree(size_t input_symbols)
{
    auto value = 1.0 / (1.0 - _degree_dist(_random_engine));
    return value < input_symbols ? std::ceil(value) : 1;
}
} // namespace Codes::Fountain