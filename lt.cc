#include "lt.h"
#include "ideal_soliton_distribution.h"
#include "robust_soliton_distribution.h"

#include <numeric>
#include <span>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

template <typename T>
T variance(const std::vector<T>& data)
{
    const auto data_size = data.size();
    if (data_size <= 1)
        return 0.0;
    const T mean_value = std::accumulate(data.begin(), data.end(), 0.0) / T(data_size);
    return std::accumulate(data.begin(), data.end(), 0.0, [&mean_value, &data_size](T accumulator, const T& value) {
        return accumulator + ((value - mean_value) * (value - mean_value) / (T(data_size) - T(1)));
    });
}

template <typename T>
T average(const std::vector<T>& data)
{
    const auto data_size = data.size();
    if (data_size == 0)
        return 0.0;
    return std::accumulate(data.begin(), data.end(), 0.0) / T(data_size);
}

struct Timer
{
    std::chrono::time_point<std::chrono::high_resolution_clock> start_point;
    static auto now()
    {
        return std::chrono::high_resolution_clock::now();
    }
    void start()
    {
        start_point = now();
    }
    template <typename Unit = std::chrono::microseconds>
    auto stop()
    {
        return std::chrono::duration_cast<Unit>(now() - start_point);
    }
};

using namespace testing;

TEST(LT, SymbolDistribution)
{
    spdlog::set_level(spdlog::level::debug);
    auto total_data_size = 100u;
    auto sample_size = 10u;
    auto total_samples = 100'000u;
    auto symbol_length = 1u;
    auto seed = 13u;
    Codes::Fountain::LT encoder(new Codes::Fountain::IdealSolitonDistribution);
    encoder.set_seed(seed);
    encoder.set_input_data_size(total_data_size);
    encoder.set_symbol_length(symbol_length);

    auto expected_selection_probability = static_cast<double>(sample_size) / total_data_size;
    auto expected_selection_tolerance = expected_selection_probability * 0.05; // Five percent deviation

    std::vector<size_t> distribution(total_data_size, 0);
    for (auto iter = 0u; iter < total_samples; ++iter)
    {
        encoder.select_symbols(sample_size, total_data_size);
        for (const auto& val : std::span(encoder._current_hash_bits.begin(), sample_size))
            ++distribution[val];
    }

    for (auto distribution_val : distribution)
        EXPECT_THAT(static_cast<double>(distribution_val) / static_cast<double>(total_samples),
                    DoubleNear(expected_selection_probability, expected_selection_tolerance));
}

TEST(LT, IdealDegreeDistribution)
{
    using DistributionType = Codes::Fountain::IdealSolitonDistribution;
    spdlog::set_level(spdlog::level::debug);
    auto total_data_size = 10u;
    auto total_samples = 1'000'000u;
    auto symbol_length = 1u;
    auto distribution_len = total_data_size / symbol_length;
    auto seed = 13u;
    Codes::Fountain::LT encoder(new DistributionType);
    encoder.set_seed(seed);
    encoder.set_input_data_size(total_data_size);
    encoder.set_symbol_length(symbol_length);

    std::vector<size_t> distribution(distribution_len, 0);
    std::vector<double> expected = DistributionType().expected_distribution(distribution_len);

    for (auto idx = 0u; idx < total_samples; ++idx)
        ++distribution[encoder.symbol_degree() - 1];

    for (auto idx = 0u; idx < distribution_len; ++idx)
        EXPECT_THAT(static_cast<double>(distribution[idx]) / static_cast<double>(total_samples),
                    DoubleNear(expected[idx], 0.001));
}

TEST(LT, RobustDegreeDistribution)
{
    using DistributionType = Codes::Fountain::RobustSolitonDistribution;
    spdlog::set_level(spdlog::level::debug);
    auto total_data_size = 10u;
    auto total_samples = 1'000'000u;
    auto symbol_length = 1u;
    auto distribution_len = total_data_size / symbol_length;
    auto seed = 13u;
    Codes::Fountain::LT encoder(new DistributionType(0.05, 0.03));
    encoder.set_seed(seed);
    encoder.set_input_data_size(total_data_size);
    encoder.set_symbol_length(symbol_length);

    std::vector<size_t> distribution(distribution_len, 0);
    std::vector<double> expected = DistributionType(0.05, 0.03).expected_distribution(distribution_len);

    for (auto idx = 0u; idx < total_samples; ++idx)
        ++distribution[encoder.symbol_degree() - 1];

    for (auto idx = 0u; idx < distribution_len; ++idx)
        EXPECT_THAT(static_cast<double>(distribution[idx]) / static_cast<double>(total_samples),
                    DoubleNear(expected[idx], 0.001));
}

TEST(LT, EncodeSimpleIdealSolition)
{
    spdlog::set_level(spdlog::level::debug);
    auto single_data_size = 4u;
    auto multiple_data = 4u;
    auto total_data_size = single_data_size * multiple_data;
    auto symbol_length = 2u;
    auto input_symbol_num = total_data_size / symbol_length;
    auto seed = 100u;
    auto encode_number = input_symbol_num + 100; // Some cases require a lot of extra packets
    auto retries = 1000;

    while (retries--)
    {
        std::vector<char> data{};
        std::vector<char*> encoded_symbols;
        {
            unsigned char raw_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

            data.resize(total_data_size);
            for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
                memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

            Codes::Fountain::LT encoder(new Codes::Fountain::IdealSolitonDistribution);
            encoder.set_seed(seed);
            encoder.set_input_data(data.data(), data.size(), true);
            encoder.set_symbol_length(symbol_length);


            for (auto enc_num = 0u; enc_num < encode_number; ++enc_num)
                encoded_symbols.push_back(encoder.generate_symbol());

            encoder.print_hash_matrix();
        }
        {
            Codes::Fountain::LT decoder(new Codes::Fountain::IdealSolitonDistribution);
            decoder.set_seed(seed);
            decoder.set_input_data_size(total_data_size);
            decoder.set_symbol_length(symbol_length);

            for (auto enc_num = 0u; enc_num < encoded_symbols.size(); ++enc_num)
                decoder.feed_symbol(encoded_symbols[enc_num], enc_num, true, false);

            decoder.print_hash_matrix();

            ASSERT_TRUE(decoder.decode());
            auto* payload = decoder.decoded_buffer();
            std::vector<char> decoded;
            decoded.resize(total_data_size);
            memcpy(decoded.data(), payload, total_data_size);
            delete[] payload;

            ASSERT_THAT(data, Eq(decoded));
        }

        for (const auto* encoded_symbol : encoded_symbols)
            delete[] encoded_symbol;
        ++seed;
    }
}

TEST(LT, EncodeSimpleRobustSolition)
{
    spdlog::set_level(spdlog::level::debug);
    auto single_data_size = 4u;
    auto multiple_data = 4u;
    auto total_data_size = single_data_size * multiple_data;
    auto symbol_length = 2u;
    auto input_symbol_num = total_data_size / symbol_length;
    auto seed = 100u;
    auto encode_number = input_symbol_num + 100; // Some cases require a lot of extra packets
    auto retries = 1000;

    while (retries--)
    {
        std::vector<char> data{};
        std::vector<char*> encoded_symbols;
        {
            unsigned char raw_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

            data.resize(total_data_size);
            for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
                memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

            Codes::Fountain::LT encoder(new Codes::Fountain::RobustSolitonDistribution(0.05, 0.03));
            encoder.set_seed(seed);
            encoder.set_input_data(data.data(), data.size(), true);
            encoder.set_symbol_length(symbol_length);


            for (auto enc_num = 0u; enc_num < encode_number; ++enc_num)
                encoded_symbols.push_back(encoder.generate_symbol());

            encoder.print_hash_matrix();
        }
        {
            Codes::Fountain::LT decoder(new Codes::Fountain::RobustSolitonDistribution(0.05, 0.03));
            decoder.set_seed(seed);
            decoder.set_input_data_size(total_data_size);
            decoder.set_symbol_length(symbol_length);

            for (auto enc_num = 0u; enc_num < encoded_symbols.size(); ++enc_num)
                decoder.feed_symbol(encoded_symbols[enc_num], enc_num, true, false);

            decoder.print_hash_matrix();

            ASSERT_TRUE(decoder.decode());
            auto* payload = decoder.decoded_buffer();
            std::vector<char> decoded;
            decoded.resize(total_data_size);
            memcpy(decoded.data(), payload, total_data_size);
            delete[] payload;

            ASSERT_THAT(data, Eq(decoded));
        }

        for (const auto* encoded_symbol : encoded_symbols)
            delete[] encoded_symbol;
        ++seed;
    }
}

TEST(LT, EncodeOnTheFlyIdealSolition)
{
    spdlog::set_level(spdlog::level::debug);
    auto symbol_length = 2u;
    auto seed = 100u;
    auto retries_max = 10u;

    std::vector<double> overhead(retries_max, 0);
    std::vector<char> data{};
    unsigned char raw_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto single_data_size = sizeof(raw_data);
    auto multiple_data = 5000u;
    auto total_data_size = single_data_size * multiple_data;
    auto input_symbols = total_data_size / symbol_length;

    data.resize(total_data_size);
    for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
        memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

    Timer tmr;
    tmr.start();
    auto retries = 0u;
    while (retries < retries_max)
    {
        Codes::Fountain::LT encoder(new Codes::Fountain::IdealSolitonDistribution);
        encoder.set_seed(seed);
        encoder.set_input_data(data.data(), data.size(), true);
        encoder.set_symbol_length(symbol_length);

        Codes::Fountain::LT decoder(new Codes::Fountain::IdealSolitonDistribution);
        decoder.set_seed(seed);
        decoder.set_input_data_size(total_data_size);
        decoder.set_symbol_length(symbol_length);
        auto already_decoded = false;
        auto enc_num = 0u;

        while (!already_decoded)
        {
            auto symbol = encoder.generate_symbol();
            already_decoded = decoder.feed_symbol(symbol, enc_num);
            ++enc_num;
        }

        overhead[retries] = double(enc_num) / double(input_symbols) - 1.0;

        ASSERT_TRUE(already_decoded);
        std::unique_ptr<char> payload(decoder.decoded_buffer());
        std::vector<char> decoded(total_data_size);
        memcpy(decoded.data(), payload.get(), total_data_size);
        ASSERT_THAT(decoded, Eq(data));

        ++seed;
        ++retries;
    }
    auto average_symbol_num = average(overhead);
    auto average_symbol_dev = std::sqrt(variance(overhead));
    auto duration = tmr.stop();
    spdlog::debug("Iteration time {:.2f}ms, Avg/dev: {:.3f}/{:.3f}", double(duration.count()) / 1000.0 / retries_max,
                  average_symbol_num, average_symbol_dev);
}

TEST(LT, EncodeOnTheFlyRobustSolition)
{
    spdlog::set_level(spdlog::level::debug);
    auto symbol_length = 2u;
    auto seed = 100u;
    auto retries_max = 10u;

    std::vector<double> overhead(retries_max, 0);
    std::vector<char> data{};
    unsigned char raw_data[] = {0xDE, 0xAD, 0xBE, 0xEF};
    auto single_data_size = sizeof(raw_data);
    auto multiple_data = 5000u;
    auto total_data_size = single_data_size * multiple_data;
    auto input_symbols = total_data_size / symbol_length;

    data.resize(total_data_size);
    for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
        memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

    Timer tmr;
    tmr.start();
    auto retries = 0u;
    while (retries < retries_max)
    {
        Codes::Fountain::LT encoder(new Codes::Fountain::RobustSolitonDistribution(0.05, 0.03));
        encoder.set_seed(seed);
        encoder.set_input_data(data.data(), data.size(), true);
        encoder.set_symbol_length(symbol_length);

        Codes::Fountain::LT decoder(new Codes::Fountain::RobustSolitonDistribution(0.05, 0.03));
        decoder.set_seed(seed);
        decoder.set_input_data_size(total_data_size);
        decoder.set_symbol_length(symbol_length);
        auto already_decoded = false;
        auto enc_num = 0u;

        while (!already_decoded)
        {
            auto symbol = encoder.generate_symbol();
            already_decoded = decoder.feed_symbol(symbol, enc_num);
            ++enc_num;
        }

        overhead[retries] = double(enc_num) / double(input_symbols) - 1.0;

        ASSERT_TRUE(already_decoded);
        std::unique_ptr<char> payload(decoder.decoded_buffer());
        std::vector<char> decoded(total_data_size);
        memcpy(decoded.data(), payload.get(), total_data_size);
        ASSERT_THAT(decoded, Eq(data));
        ++seed;
        ++retries;
    }
    auto average_symbol_num = average(overhead);
    auto average_symbol_dev = std::sqrt(variance(overhead));
    auto duration = tmr.stop();
    spdlog::debug("Iteration time {:.2f}ms, Avg/dev: {:.3f}/{:.3f}", double(duration.count()) / 1000.0 / retries_max,
                  average_symbol_num, average_symbol_dev);
}
