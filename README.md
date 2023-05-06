# rateless_codes
Library that allows to encode any data with rateless codes.
Rateless codes is a class of codes that does not have fixed rate value, but it can (in theory) produce limitless number of encoded symbols. One example of such codes is a group of fountain codes. There is even a nice picture with fountain that produce drops. To get a full glass of water there is enough to collect some amount of them (in theory only a glass). In practice (due to imperfection of algorithms), we have to collect a little bit more (overhead)

## Motivation
In the past I have been working with rateless codes - to be specific with fountain codes. I want to have an open source implementation to use it one day for a specific task. I am aware that there may be many other solutions, that is why I want to keep that one as lean as it can be with so little external dependencies as it can be.

## Functionality
Currently any data that can be passed as char* is able to be encoded. There are no padding options, so before choosing symbol size make sure that it will divide whole message size without rest - to equal pieces. I am planning to fix that in the future releases.

## Security
It can be seen as en extra feature - each encoding/decoding operation requires to specify seed value used to mix input data. If it is not correct, a message will be decoded but in a way that will produce invalid output. There is no way of telling if this is correct or not (at least there is nothing that can be used for that in general) except using some CRC beforehand and append that to the original message.

## Overhead
If this will be used as ECC for live communication, there is no need for retransmission. Each packet of data will provide some information (with probability near to 100%). There is enough to send one bit of information - "stop", at the point where decoding is successful.

### RLF
Probability of successful decoding (in case of RLF) is $\approx 1 - 2^{(-k - 1)}$ where k is number of additional packets, i.e. extra pieces of information that exceed size of input data. If there is less information received data will not be decoded at all.

In case of LT codes, this is more complicated. There is Ideal Soliton Distribution and Robust Soliton Distribution.

### LT with ISD

Because it is ideal, there are some assumptions that are fulfilled only for ideal generator, but it is very hard to construct, so overhead could be quite large, i.e. ~50%. However this is good start to understand what is going on and how distribution should be designed. In theory there is enough to have one symbol with degree one to start decoding. It will reduce degree of other symbols, and there is high enough probability that another symbol of degree one will be available, and so on. Because this is ideal situation, sometimes there is not enough to have a one symbol with degree one, or even a bunch of them will not allow to perform further decoding.

### LT with RSD

Large overhead of practical use of ISD is why RSD have been designed to introduce more symbols with lower degree and additionally there is a spike in a probability distribution that will allow for all symbols to be "represented".
There are two parameters that can be used to customize code generation. They can be describe in literature as delta (fail probability) and C (some constant). There are many information and literature that can be used to customize them. Usually for higher C value, required overhead (on average) is also higher but variance is lower. This means that usually overhead is around 3-5%, but there might be messages with overhead as high as 20-30%. When it comes to delta, there is a claim that for certain C, $K'=K+2\ln(S/\delta)S$ is a number of packets required to decode original message (that consists of K packets) with probability $1-\delta$. Note that $S=C\ln(K/\delta)\sqrt K$. To summarize - lower the delta, higher the number of expected packets to be transmitted, but it can be tuned to your purposes with correct selection of C.

### Other distributions

There are some distributions fine tuned for short messages (small number of packets), as in general, lower the number of packets, bigger the overhead. There might be some distributions to lower bandwidth or encoder/decoder complexity. At the end everybody can create distribution that suits specific needs.

### Tornado/Raptor Codes

This class of codes assumes that there is a high chance that most of packets can be decoded with small overhead and rest of them, let's say 5% requires a lot of extra data to be transmitted. So to deal with that, another coding with known rate can be used for inner coding and fountain codes are used for outer coding. If we decide to use low complexity inner coding and combine that with low degree LT code (our assumption), we can achieve near linear decoding complexity.

## Performance
For fixed size channels RLF is quite good solutions as it can give really small overhead (20 extra symbols will give $10^{-6}$ probability of failure with overhead of 2% when symbol number is equal to 1000. Important thing is decoding and encoding complexity which in case of RLF is not meaningless. Encoder has a complexity of $O(N^2)$ where N is number of input symbols, however decoder have complexity $O(N^3)$. This can be a huge problem for large messages or messages with small symbols, therefore it might be better to split message into smaller chunks before encoding. LT codes decoder complexity is $\approx K \ln K$ (average packet degree times K) which is much better, and only drawback is higher bandwidth. However Raptor code use LT code with average degree $\hat{d}=3$. This is a linear complexity, but costs is a high chance of failure, because of some amount of undecoded packets. How large it is? It can be proven that this fraction is approximately $e^{-\hat{d}}$, which for $\hat{d}$ is 5%. This is not much and, for larger messages it is very probable that number of not decoded packets will be closer and closer to this value.
At the end trick is to first encode data using code with known erasure rate equal to 0.05, and then transmit this using weak LT code (Raptor code with degre 3). Usually an LDPC is used to perform "pre-coding", as it is well known and can provide low complexity. Idea behind LDPC is very similar to LT code, but its graph is precomputed and optimized to provide the same parameters no matter what part of the message is missing.

## Future work
There are other class of codes like LT codes, or Rapid Tornado codes that lower complexity of encoder and decoder to $\sim O(\log(N))$ using specific distribution when it comes to mixing input data at a cost of overhead. While RLF requires fixed number of extra symbols to give almost 100% chance of success, other variants could require up to 20-30% or even more in very specific cases. There are even dedicated distributions for short messages to lower that overhead.

* [x] Implement LT with Ideal Soliton Distribution
* [x] Implement Robust Soliton distribution
* [ ] Replace degree distribution PRNG by more portable solution
* [ ] Add Rapid Tornado Codes
  * [ ] Implement distribution for Rapid Tornado Codes
  * [ ] Add external coding to restore missing symbols (might require external library)
* [ ] Improve API
* [x] Improve CPU performance (better data structures) - Done for LT
* [x] Dedicated PRNG to avoid encoder/decoder mismatch
* [ ] Try to run as much encoding/decoding in parallel
* [ ] Investigate possibility to run encoding/decoding on the GPU (CUDA)
* [ ] Add option to use padding
* [ ] Add option to use simple checksums

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
// choose one of available encoders
Codes::Fountain::RLF encoder;
Codes::Fountain::LT encoder(new Codes::Fountain::RobustSolitonDistribution(0.05, 0.03));
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
// Use one of the available decoders, make sure LT params are the same
Codes::Fountain::RLF decoder;
Codes::Fountain::LT decoder(new Codes::Fountain::RobustSolitonDistribution(0.05, 0.03));
decoder.set_seed(seed);
decoder.set_input_data_size(total_data_size);
decoder.set_symbol_length(symbol_length);
auto already_decoded = false;
auto symbol_num = 0;
while(already_decoded)
{
  auto* ptr = fetch_symbol();
  // For RLF you can wait until at least some number of packets is received
  // But it might be better to start decoding while data is still transmitted
  decoder.feed_symbol(ptr, symbol_num, Memory::Owner, Decoding::Start);
  already_decoded = decoder.decode(true);
  // inform generator that there is enough symbols to decode
  client.set_enough(true);
}
auto* payload = decoder.decoded_buffer();
```
