# codec

> **Alphabet transition decided 2026-09-26:** The current codec implements
> base-50. The [primary address decision](../../../docs/address-encoding-transition.md)
> adopts the RFC 4648 §5 URL-safe 64-symbol alphabet for a later code and
> data migration. This README describes current executable behaviour.

The address / token_id codec: the primitive every higher database-kernel routine
uses to convert between a token's address and its canonical string
token_id, and to handle prefix-delta (context-relative) addressing.

Pure functions, no state, no I/O, no DB. Standard library only.

## Files

- `codec.h` / `codec.cpp` -- the module.
- `codec_test.cpp` -- unit tests (standalone; own tiny check harness, no
  external test framework).

## Build & run the tests

```
g++ -std=c++17 -O2 -Wall -Wextra -o /tmp/codec_test codec.cpp codec_test.cpp && /tmp/codec_test
```

Exits 0 and prints `PASS codec_test` if every check passes; prints `FAIL`
lines for anything that didn't and exits non-zero.

## Alphabet

Base-50: `A`-`Z` and `a`-`z` (52 characters) minus `o` and `O` (excluded --
easily confused with `0`). The 50 surviving characters and their order
(alphabet index 0..49) are the single source of truth, defined once as
`codec::kAlphabet` in `codec.cpp`: the 25 surviving uppercase letters
(A-N, P-Z) followed by the 25 surviving lowercase letters (a-n, p-z).
Conversion functions use `alphabet_index()` / `kAlphabet`. Changing the
alphabet or base also affects the `kAlphabetSize` constant, successor and
range planning, schema ordering assumptions, legacy translation and tests;
it is not only a one-table replacement.

## Representation choices

- **Couplet code**: `uint16_t` in `[0, 2500)` -- `first_index * 50 +
  second_index`. Matches the spec's required interface exactly
  (`couplet_to_code` / `code_to_couplet`).
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
    (couplet code 0).
  - `AA.AA.AA.AA.A*` -- the 17-entry root: four full couplets plus a
    partial trailing element, addressing the context/prefix node rather
    than one leaf.

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
  character.
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

// Delta-by-position
std::optional<Address> compute_delta(const Address &context, const Address &full);
Address reconstruct_address(const Address &context, const Address &delta);
```
