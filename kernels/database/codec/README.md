# Address codec

The C++ codec uses literal two-character Base62 address pairs. Its alphabet is exactly `0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz`, in the byte order used by PostgreSQL's built-in `COLLATE "C"`. See [the address decision](../../../docs/address-encoding-transition.md) for the reason PR #77's numeric storage proposal was reversed.

Addresses in C++ are sequences of `AddressElement {first, second, partial}`. `first` and `second` are indices into `codec::kAlphabet`; the controller stores each full pair as the corresponding two-character element of a `text[] COLLATE "C"` identity. The dotted rendering (`AA.0b`) is a display/interchange form, not a substitute key. The pair code `first * 62 + second` in `[0, 3844)` is useful for temporary arithmetic, **not** a `smallint[]` storage key.

A last element can be partial (`A*`) when naming a wildcard range for `gather()`. Partial addresses are query-only; they cannot be stored as identities. `.` and `*` are outside the address alphabet. The empty address round-trips through the codec, but the record tier refuses it as an identity.

The codec also has `compute_delta(context, full)` and `reconstruct_address(context, delta)` for positional prefix-relative addressing. The caller supplies the shared context: `compute_delta` checks length, not equality of prefix content. That contract matters when a shared root is stored once for a compressed tree and full addresses are reconstructed from its continuations; this codec does not itself implement compressed PostgreSQL storage. A wildcard fixes that root to enumerate a range instead of reconstructing an individual member.

## Files and checks

- `codec.h` / `codec.cpp`: pure codec functions, no storage or I/O.
- `codec_test.cpp`: alphabet, pair and address round trips, byte ordering, delta.
- `codec_advtest.cpp`: every input byte, every pair and edge cases.

From this directory:

```sh
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/codec_test codec.cpp codec_test.cpp && /tmp/codec_test
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/codec_advtest codec.cpp codec_advtest.cpp && /tmp/codec_advtest
```

## Public interface

```cpp
int alphabet_index(char c);                    // index or -1
bool is_valid_char(char c);
bool is_valid_couplet(char first, char second);
std::optional<uint16_t> couplet_to_code(char first, char second);
std::optional<std::pair<char, char>> code_to_couplet(uint16_t code);
AddressElement make_full_element(uint16_t couplet_code);
AddressElement make_partial_element(uint8_t first_index);
bool is_valid_element(const AddressElement &e);
bool is_valid_address(const Address &address);
std::optional<std::string> encode_token_id(const Address &address);
std::optional<Address> decode_token_id(const std::string &token_id);
std::optional<Address> compute_delta(const Address &context, const Address &full);
Address reconstruct_address(const Address &context, const Address &delta);
```
