#include "ideal_soliton_distribution.h"

#include <algorithm>

namespace Codes::Fountain {

IdealSolitonDistribution::IdealSolitonDistribution()
    : _degree_dist(std::uniform_real_distribution<double>())
{}

void IdealSolitonDistribution::set_seed(uint32_t seed)
{
    _random_engine.seed(seed);
}

void IdealSolitonDistribution::set_input_size(size_t input_symbols)
{
    _input_size = input_symbols;
}

size_t IdealSolitonDistribution::symbol_degree()
{
    auto value = 1.0 / (1.0 - _degree_dist(_random_engine));
    return value < _input_size ? std::ceil(value) : 1;
}

std::vector<double> IdealSolitonDistribution::expected_distribution(size_t input_symbols)
{
    std::vector<double> expected;
    expected.resize(input_symbols);

    expected[0] = 1.0 / input_symbols;
    for (auto idx = 1u; idx < input_symbols; ++idx)
        expected[idx] = 1.0 / (idx + 1) / idx;
    return expected;
}
} // namespace Codes::Fountain