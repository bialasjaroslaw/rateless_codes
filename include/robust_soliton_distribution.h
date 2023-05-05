#pragma once

#include <cstddef>
#include <cstdint>
#include <random>

#include "degree_distribution.h"

namespace Codes::Fountain {
class RobustSolitonDistribution : public DegreeDistribution
{
public:
    RobustSolitonDistribution(double delta, double c);
    virtual ~RobustSolitonDistribution() = default;

    void set_seed(uint32_t seed) override;
    void set_input_size(size_t input_symbols) override;
    size_t symbol_degree() override;
    std::vector<double> expected_distribution(size_t input_symbols) override;

private:
    std::default_random_engine _random_engine;
    std::uniform_real_distribution<double> _degree_dist;
    size_t _input_size = 0;
    std::vector<double> _cumulative_probabilities;
    double _delta = 0.01;
    double _c = 0.75;
};
} // namespace Codes::Fountain