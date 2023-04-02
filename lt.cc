#include "lt.h"

#include <span>

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace testing;

TEST(LT, DegreeDistribution)
{
    spdlog::set_level(spdlog::level::debug);
    auto total_data_size = 100u;
    auto sample_size = 10u;
    auto total_samples = 100'000u;
    auto symbol_length = 1u;
    auto distribution_len = total_data_size / symbol_length;
    auto seed = 13u;
    Codes::Fountain::LT encoder;
    encoder.set_seed(seed);
    encoder.set_input_data_size(total_data_size);
    encoder.set_symbol_length(symbol_length);

    auto expected_success_probability = static_cast<double>(sample_size) / total_data_size;
    auto expected_success_tolerance = expected_success_probability * 0.05; // Five percent diviation

    std::vector<size_t> distribution(total_data_size, 0);
    for (auto iter = 0u; iter < total_samples; ++iter)
    {
        encoder.select_symbols(10, total_data_size);
        for (const auto& val : std::span(encoder._current_hash_bits.begin(), sample_size))
            ++distribution[val];
    }

    for (auto idx = 0u; idx < distribution_len; ++idx)
        EXPECT_THAT(distribution[idx] / static_cast<double>(total_samples), DoubleNear(expected_success_probability, expected_success_tolerance));
}

TEST(LT, SymbolDistribution)
{
    spdlog::set_level(spdlog::level::debug);
    auto total_data_size = 10u;
    auto total_samples = 1'000'000u;
    auto symbol_length = 1u;
    auto distribution_len = total_data_size / symbol_length;
    auto seed = 13u;
    Codes::Fountain::LT encoder;
    encoder.set_seed(seed);
    encoder.set_input_data_size(total_data_size);
    encoder.set_symbol_length(symbol_length);

    std::vector<size_t> distribution(distribution_len, 0);
    std::vector<double> expected;
    expected.resize(distribution_len);

    expected[0] = 1.0 / distribution_len;
    for (auto idx = 1u; idx < distribution_len; ++idx)
        expected[idx] = 1.0 / (idx + 1) / idx;

    for (auto idx = 0u; idx < total_samples; ++idx)
        ++distribution[encoder.symbol_degree() - 1];

    for (auto idx = 0u; idx < distribution_len; ++idx)
        EXPECT_THAT(distribution[idx] / static_cast<double>(total_samples), DoubleNear(expected[idx], 0.001));
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
            unsigned char raw_data[] = { 0xDE, 0xAD, 0xBE, 0xEF };

            data.resize(total_data_size);
            for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
                memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

            Codes::Fountain::LT encoder;
            encoder.set_seed(seed);
            encoder.set_input_data(data.data(), data.size(), true);
            encoder.set_symbol_length(symbol_length);


            for (auto enc_num = 0u; enc_num < encode_number; ++enc_num)
                encoded_symbols.push_back(encoder.generate_symbol());

            encoder.print_hash_matrix();
        }
        {
            Codes::Fountain::LT decoder;
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
    auto single_data_size = 4u;
    auto multiple_data = 4u;
    auto total_data_size = single_data_size * multiple_data;
    auto symbol_length = 2u;
    auto seed = 100u;
    auto retries = 1000;
    while (retries--)
    {
        std::vector<char*> encoded_symbols;
        {
            std::vector<char> data{};
            unsigned char raw_data[] = { 0xDE, 0xAD, 0xBE, 0xEF };

            data.resize(total_data_size);
            for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
                memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

            Codes::Fountain::LT encoder;
            encoder.set_seed(seed);
            encoder.set_input_data(data.data(), data.size(), true);
            encoder.set_symbol_length(symbol_length);

            Codes::Fountain::LT decoder;
            decoder.set_seed(seed);
            decoder.set_input_data_size(total_data_size);
            decoder.set_symbol_length(symbol_length);
            auto already_decoded = false;
            auto enc_num = 0u;

            while (true)
            {
                auto symbol = encoder.generate_symbol();
                encoded_symbols.push_back(symbol);
                decoder.feed_symbol(symbol, enc_num);
                already_decoded = decoder.decode(true);
                if (already_decoded)
                    break;
                ++enc_num;
            }

            ASSERT_TRUE(already_decoded);
            auto* payload = decoder.decoded_buffer();
            std::vector<char> decoded;
            decoded.resize(total_data_size);
            memcpy(decoded.data(), payload, total_data_size);
            delete[] payload;

            ASSERT_THAT(decoded, Eq(data));
        }
        for (const auto* encoded_symbol : encoded_symbols)
            delete[] encoded_symbol;
        ++seed;
    }
}
