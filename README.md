# rateless_codes
Library that allows to encode any data with rateless codes.
Rateless codes is a class of codes that does not have fixed rate value, but it can (in theory) produce limitless number of encoded symbols. One example of such codes is a group of fountain codes. There is even a nice picture with fountain that produce drops. To get a full glass of water there is enough to collect some amount of them (in theory only a glass). In practice (due to imperfection of algorithms), we have to collect a little bit more (overhead)

## Motivation
In the past I have been working with rateless codes - to be specific with fountain codes. I want to have an open source implementation to use it one day for a specific task. I am aware that there may be many other solutions, that is why I want to keep that one as lean as it can be with so little external dependencies as it can be.

## Functionality
Currently any data that can be passed as char* is able to be encoded. There are no padding options, so before choosing symbol size make sure that it will divide whole message size without rest - to equal pieces. I am planning to fix that in the future releases.

## Security
It can be seen as en extra feature - each encoding/decoding operation requires to specify seed value used to mix input data. If it is not correct, a message will be decoded but in a way that will produce invalid output. There is no way of telling if this is correct or not (at least there is nothing that can be used for that in general).

## Overhead
If this will be used as ECC for live communication, there is no need for retransmission. Each packet of data will provide some information (with probability near to 100%). There is enough to send one bit of information - "stop", at the point where decoding is successful.
Probability of successful decoding (in case of RLF) is $\approx 1 - 2^{(-k - 1)}$ where k is number of additional packets, i.e. extra pieces of information that exceed size of input data. If there is less information received data will not be decoded at all.

## Performance
For fixed size channels RLF is quite good solutions as it can give really small overhead (20 extra symbols will give $10^{-6}$ probability of failure with overhead of 2% when symbol number is equal to 1000. Important thing is decoding and encoding complexity which in case of RLF is not meaningless. Encoder has a complexity of $O(N^2)$ where N is number of input symbols, however decoder have complexity $O(N^3)$. This can be a huge problem for large messages or messages with small symbols, therefore it might be better to split message into smaller chunks before encoding.

## Future work
There are other class of codes like LT codes, or Rapid Tornado codes that lower complexity of encoder and decoder to $\sim O(\log(N))$ using specific distribution when it comes to mixing input data at a cost of overhead. While RLF requires fixed number of extra symbols to give almost 100% chance of success, other variants could require up to 20-30% or even more in very specific cases. There are even dedicated distributions for short messages to lower that overhead.

* [x] Implement LT with Ideal Soliton Distribution
* [ ] Implement Robust Soliton distribution
* [ ] Add Rapid Tornado Codes
  * [ ] Implement distribution for Rapid Tornado Codes
  * [ ] Add external coding to restore missing symbols (might require external library)
* [ ] Improve API
* [ ] Improve CPU performance (better data structures)
* [ ] Try to run as much encoding/decoding in parallel
* [ ] Investigate possibility to run encoding/decoding on the GPU (CUDA)

## How to use
This will probably change in the future to be simpler and more intuitive
### Encode
```c++
int seed = 13;
int symbol_length = 8;
// create data container, vector of char, just for convenience
std::vector<char> data;
// fill with own data and pass total size to decoder
total_data_size = 64; // 8 symbols with 8 bytes each
Codes::Fountain::RLF encoder;
encoder.set_seed(seed);
encoder.set_input_data(data.data(), data.size()); // no deep copy
encoder.set_symbol_length(symbol_length);
// generate as many symbols as you need
bool enough_symbols = false;
while(!enough_symbols)
{
  auto* symbol = encoder.generate_symbol(); // caller is owner
  // check if client already received enough
  enough_symbols = client.has_enough();
}
```

### Decode
```c++
Codes::Fountain::RLF decoder;
decoder.set_seed(seed);
decoder.set_input_data_size(total_data_size);
decoder.set_symbol_length(symbol_length);
auto already_decoded = false;
auto symbol_num = 0;
while(already_decoded)
{
  auto* ptr = fetch_symbol();
  decoder.feed_symbol(ptr, symbol_num, true); // true for deep copy
  already_decoded = decoder.decode(true);
  // inform generator that there is enough symbols to decode
  client.set_enough(true);
}
auto* payload = decoder.decoded_buffer();
```
