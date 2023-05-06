#include "rlf.h"

#include <gmock/gmock-matchers.h>
#include <gmock/gmock-more-matchers.h>
#include <gtest/gtest.h>
#include <spdlog/spdlog.h>

using namespace testing;

TEST(Well512, BitDistribution)
{
    spdlog::set_level(spdlog::level::debug);
    well_512 generator;
    generator.set_seed(13u);

    auto total_samples = 10'000'000u;
    auto sum_values = 0u;

    for (auto idx = 0u; idx < total_samples; ++idx)
    {
        auto val = generator.rand_bit();
        sum_values += val;
    }
    auto avg = double(sum_values) / double(total_samples);
    EXPECT_THAT(avg, DoubleNear(0.5, 0.0001));
}

TEST(RLF, EncodeSimple)
{
    spdlog::set_level(spdlog::level::debug);
    std::vector<char*> encoded_symbols;
    auto single_data_size = 4u;
    auto multiple_data = 500u;
    auto total_data_size = single_data_size * multiple_data;
    auto symbol_length = 2u;
    auto input_symbol_num = total_data_size / symbol_length;
    auto seed = 13u;
    auto encode_number = input_symbol_num + 10;
    std::vector<char> data{};
    {
        unsigned char raw_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

        data.resize(total_data_size);
        for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
            memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

        Codes::Fountain::RLF encoder;
        encoder.set_seed(seed);
        encoder.set_input_data(data.data(), data.size(), true);
        encoder.set_symbol_length(symbol_length);


        for (auto enc_num = 0u; enc_num < encode_number; ++enc_num)
            encoded_symbols.push_back(encoder.generate_symbol());
    }
    {
        Codes::Fountain::RLF decoder;
        decoder.set_seed(seed);
        decoder.set_input_data_size(total_data_size);
        decoder.set_symbol_length(symbol_length);

        for (auto enc_num = 0u; enc_num < encoded_symbols.size(); ++enc_num)
            decoder.feed_symbol(encoded_symbols[enc_num], enc_num, true);

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
}

TEST(RLF, EncodeOnTheFly)
{
    spdlog::set_level(spdlog::level::debug);
    std::vector<char*> encoded_symbols;
    auto single_data_size = 4u;
    auto multiple_data = 500u;
    auto total_data_size = single_data_size * multiple_data;
    auto symbol_length = 2u;
    auto input_symbol_num = total_data_size / symbol_length;
    auto seed = 13u;
    auto encode_number = input_symbol_num + 10;
    std::vector<char> data{};
    {
        unsigned char raw_data[] = {0xDE, 0xAD, 0xBE, 0xEF};

        data.resize(total_data_size);
        for (auto copy_num = 0u; copy_num < multiple_data; ++copy_num)
            memcpy(data.data() + single_data_size * copy_num, raw_data, single_data_size);

        Codes::Fountain::RLF encoder;
        encoder.set_seed(seed);
        encoder.set_input_data(data.data(), data.size(), true);
        encoder.set_symbol_length(symbol_length);


        for (auto enc_num = 0u; enc_num < encode_number; ++enc_num)
            encoded_symbols.push_back(encoder.generate_symbol());
    }
    {
        Codes::Fountain::RLF decoder;
        decoder.set_seed(seed);
        decoder.set_input_data_size(total_data_size);
        decoder.set_symbol_length(symbol_length);
        auto already_decoded = false;

        for (auto enc_num = 0u; enc_num < encoded_symbols.size(); ++enc_num)
        {
            decoder.feed_symbol(encoded_symbols[enc_num], enc_num, true);
            already_decoded = decoder.decode(true);
            if (already_decoded)
                break;
        }

        ASSERT_TRUE(already_decoded);
        auto* payload = decoder.decoded_buffer();
        std::vector<char> decoded;
        decoded.resize(total_data_size);
        memcpy(decoded.data(), payload, total_data_size);
        delete[] payload;

        ASSERT_THAT(data, Eq(decoded));

        for (const auto* encoded_symbol : encoded_symbols)
            delete[] encoded_symbol;
    }
}
