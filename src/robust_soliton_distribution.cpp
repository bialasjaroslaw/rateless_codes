#include "robust_soliton_distribution.h"

#include "ideal_soliton_distribution.h"

namespace Codes::Fountain {

RobustSolitonDistribution::RobustSolitonDistribution()
    : _degree_dist(std::uniform_real_distribution<double>())
{}

void RobustSolitonDistribution::set_seed(uint32_t seed)
{
    _random_engine.seed(seed);
}

void RobustSolitonDistribution::set_input_size(size_t input_symbols)
{
    _input_size = input_symbols;
    auto probabilities = expected_distribution(_input_size);
    _cumulative_probabilities.resize(_input_size);
    auto cumulative_value = 0.0;
    for (auto idx = 0u; idx < _input_size; ++idx)
    {
        cumulative_value += probabilities[idx];
        _cumulative_probabilities[idx] = cumulative_value;
    }
}

size_t RobustSolitonDistribution::symbol_degree()
{
    auto value = _degree_dist(_random_engine);
    auto it = std::lower_bound(_cumulative_probabilities.cbegin(), _cumulative_probabilities.cend(), value);
    return it == _cumulative_probabilities.cend() ? _cumulative_probabilities.size()
                                                  : std::distance(_cumulative_probabilities.cbegin(), it) + 1;
}

std::vector<double> RobustSolitonDistribution::expected_distribution(size_t input_symbols)
{
    std::vector<double> expected = IdealSolitonDistribution().expected_distribution(input_symbols);

    auto failure_prob = 0.01;
    auto some_const = 0.1;
    auto R = some_const * std::log(input_symbols / failure_prob) * std::sqrt(input_symbols);
    auto spike_loc = static_cast<size_t>(std::ceil(input_symbols / R));

    for (auto idx = 0u; idx < spike_loc; ++idx)
        expected[idx] += R / input_symbols / (idx + 1);
    expected[spike_loc] += R * std::log(R / failure_prob) / input_symbols;

    auto distribution_sum = std::accumulate(expected.cbegin(), expected.cend(), 0.0, std::plus<double>());
    for (auto& value : expected)
        value /= distribution_sum;

    return expected;
}
} // namespace Codes::Fountain