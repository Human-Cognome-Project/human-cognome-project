# codec

> **Alphabet (2026-09-26):** The codec implements the RFC 4648 §5 URL-safe
> 64-symbol alphabet. See the [address decision](../../../docs/address-encoding-transition.md),
> including why storage keys are numeric pair codes rather than text. Until
> the pair-code storage step lands, the record tier stores only the letter
> subset (see [controller](../controller/README.md)).

The address / token_id codec: the primitive every higher database-kernel
routine uses to convert between a token's address, its canonical string
token_id, and its storage key, and to handle prefix-delta (context-relative)
addressing.

Pure functions, no state, no I/O, no DB. Standard library only.

## Files

- `codec.h` / `codec.cpp` -- the module.
- `codec_test.cpp` -- unit tests.
- `codec_advtest.cpp` -- adversarial tests (every byte, every pair code,
  malformed strings, boundary indices).

Both are standalone, with their own tiny check harness and no external test
framework.

## Build & run the tests

```
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/codec_test codec.cpp codec_test.cpp && /tmp/codec_test
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/codec_advtest codec.cpp codec_advtest.cpp && /tmp/codec_advtest
```

Each exits 0 and prints `PASS <name>` if every check passes; prints `FAIL`
lines for anything that didn't and exits non-zero.

## Alphabet

RFC 4648 §5, in RFC value order, defined once as `codec::kAlphabet`:

```
ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_
```

A symbol's alphabet index **is** its RFC value (`A` = 0, `a` = 26, `0` = 52,
`-` = 62, `_` = 63). `=` (octet-Base64 padding), standard Base64's `+` and
`/`, the delimiter `.` and the partial marker `*` are not address symbols.

Addresses use the RFC's alphabet and value mapping, **not** its
octet-to-symbol encoding. An address is a sequence of independent radix-64
digits grouped into pairs, and a generic Base64url byte decoder will not
round-trip it. Any byte-level serialization must define its own explicit
mapping (see the decision record).

## Representation choices

- **Couplet (pair) code**: `uint16_t` in `[0, 4096)` --
  `first_value * 64 + second_value`, twelve bits. Numeric code order is
  value order (`couplet_to_code` / `code_to_couplet`).
- **Address element**: a small struct, not a bare couplet code --

  ```cpp
  struct AddressElement {
    uint8_t first;   // alphabet index of the first character
    uint8_t second;  // alphabet index of the second character (if !partial)
    bool partial;    // true => only `first` is meaningful
  };
  using Address = std::vector<AddressElement>;
  ```

  A flat `vector<uint16_t>` of couplet codes was the other option the spec
  offered, but it has no room for a **partial trailing element** -- an
  address whose last element specifies only its first character (see
  below), used for a prefix/context node rather than a leaf token. The
  struct handles both full and partial elements as one type without a
  second, parallel address representation.
- **token_id**: elements joined by `.`. A full element serializes as its
  two characters (e.g. `AA`); a partial element serializes as its one
  character followed by the literal wildcard marker `*` (e.g. `A*`),
  valid only as the address's last element. The empty address encodes to
  the empty string and vice versa.

  Base cases (both covered in `codec_test.cpp`):
  - `AA.AA.AA.AA.AA` -- the `0x` particle: five full couplets, all `AA`
    (couplet code 0; the same string as under base-50, a different
    alphabet table behind it).
  - `AA.AA.AA.AA.A*` -- the 17-entry root: four full couplets plus a
    partial trailing element, addressing the context/prefix node rather
    than one leaf.

## Storage key

The record tier's identity key is the address as an array of pair codes
(`codec::PairKey`, a `std::vector<uint16_t>`, stored as PostgreSQL
`smallint[]`). Comparing keys lexicographically compares addresses in value
order for every symbol, so an ordered index over the key keeps wildcard and
`FROM..TO` reads as single contiguous range scans. Comparing token_id text
does not do this: in byte order, digits, `-` and `_` fall between or before
the letters (`test_key_order_is_value_order` shows the difference).

- `to_pair_key(address)` gives the key of a full address. A partial address
  has no single key.
- `from_pair_key(key)` gives the address back; any code `>= 4096` is rejected.
- `partial_code_range(element)` gives the inclusive code interval
  `[first*64, first*64+63]` a partial element stands for. That is one
  contiguous range in key order.

## OPEN decisions

The spec was silent on the exact token_id delimiter and on how a partial
/ context address should be represented in that string form. Simplest
correct choices made here, each marked `// OPEN:` at its definition in
`codec.h`:

- **`// OPEN: token_id delimiter is '.'`** -- read directly off the
  worked examples in the spec (`AA.AA.AA.AA.AA`); no other delimiter is
  implied, so no alternative scheme was invented.
- **`// OPEN: partial-element marker is the literal '*'`** -- a partial
  element (one character, no second couplet character) serializes as
  `<char>*`. This is the simplest string form that (a) round-trips
  losslessly, (b) matches the `AA.AA.AA.AA.A*` worked example verbatim,
  and (c) needs no escaping since `*` is never a valid alphabet
  character (nor is `.`; both are outside RFC 4648 §5).
- **`// OPEN: delta is positional, not content-verified`** -- the spec
  states the shared prefix "is assumed by position." `compute_delta`
  therefore does not compare `context`'s element values against `full`'s;
  it only checks `context.size() <= full.size()` and slices the tail.
  This is intentional, not an oversight -- see
  `test_delta_round_trip`'s "delta is taken by position, not verified
  against context content" case, where a context that disagrees with the
  address's real prefix still yields the tail past its own length.

No other scheme (varint packing, run-length delta, a second marker
character, etc.) was introduced anywhere the spec left a gap.

## Public interface

```cpp
// Alphabet
int alphabet_index(char c);                    // index or -1
bool is_valid_char(char c);

// Couplet <-> code
bool is_valid_couplet(char first, char second);
std::optional<uint16_t> couplet_to_code(char first, char second);
std::optional<std::pair<char, char>> code_to_couplet(uint16_t code);

// Address elements
AddressElement make_full_element(uint16_t couplet_code);
AddressElement make_partial_element(uint8_t first_index);
bool is_valid_element(const AddressElement &e);
bool is_valid_address(const Address &address);

// Address <-> token_id
std::optional<std::string> encode_token_id(const Address &address);
std::optional<Address> decode_token_id(const std::string &token_id);

// Storage key
std::optional<PairKey> to_pair_key(const Address &address);
std::optional<Address> from_pair_key(const PairKey &key);
std::optional<std::pair<uint16_t, uint16_t>> partial_code_range(const AddressElement &e);

// Delta-by-position
std::optional<Address> compute_delta(const Address &context, const Address &full);
Address reconstruct_address(const Address &context, const Address &delta);
```
