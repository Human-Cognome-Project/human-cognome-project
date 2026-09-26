#include "codec.h"

namespace codec {

namespace {

// Reverse lookup: ASCII code -> alphabet index, or -1. Built once from
// kAlphabet so the alphabet table stays the single source of truth.
std::array<int, 256> build_reverse_lookup() {
  std::array<int, 256> table{};
  for (auto &slot : table) {
    slot = -1;
  }
  for (int i = 0; i < kAlphabetSize; ++i) {
    table[static_cast<unsigned char>(kAlphabet[i])] = i;
  }
  return table;
}

const std::array<int, 256> &reverse_lookup() {
  static const std::array<int, 256> table = build_reverse_lookup();
  return table;
}

}  // namespace

// RFC 4648 section 5, in RFC value order: index == value.
const std::array<char, kAlphabetSize> kAlphabet = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
    'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '-', '_',
};

int alphabet_index(char c) {
  return reverse_lookup()[static_cast<unsigned char>(c)];
}

bool is_valid_char(char c) {
  return alphabet_index(c) >= 0;
}

bool is_valid_couplet(char first, char second) {
  return is_valid_char(first) && is_valid_char(second);
}

std::optional<uint16_t> couplet_to_code(char first, char second) {
  const int a = alphabet_index(first);
  const int b = alphabet_index(second);
  if (a < 0 || b < 0) {
    return std::nullopt;
  }
  return static_cast<uint16_t>(a * kAlphabetSize + b);
}

std::optional<std::pair<char, char>> code_to_couplet(uint16_t code) {
  if (code >= kCoupletSpace) {
    return std::nullopt;
  }
  const int a = code / kAlphabetSize;
  const int b = code % kAlphabetSize;
  return std::make_pair(kAlphabet[a], kAlphabet[b]);
}

AddressElement make_full_element(uint16_t couplet_code) {
  AddressElement e;
  e.partial = false;
  e.first = static_cast<uint8_t>(couplet_code / kAlphabetSize);
  e.second = static_cast<uint8_t>(couplet_code % kAlphabetSize);
  return e;
}

AddressElement make_partial_element(uint8_t first_index) {
  AddressElement e;
  e.partial = true;
  e.first = first_index;
  e.second = 0;
  return e;
}

bool is_valid_element(const AddressElement &e) {
  if (e.first >= kAlphabetSize) {
    return false;
  }
  if (e.partial) {
    return true;
  }
  return e.second < kAlphabetSize;
}

bool is_valid_address(const Address &address) {
  for (std::size_t i = 0; i < address.size(); ++i) {
    if (!is_valid_element(address[i])) {
      return false;
    }
    const bool is_last = (i + 1 == address.size());
    if (address[i].partial && !is_last) {
      return false;
    }
  }
  return true;
}

std::optional<std::string> encode_token_id(const Address &address) {
  if (!is_valid_address(address)) {
    return std::nullopt;
  }
  std::string out;
  for (std::size_t i = 0; i < address.size(); ++i) {
    if (i > 0) {
      out.push_back(kDelimiter);
    }
    const AddressElement &e = address[i];
    out.push_back(kAlphabet[e.first]);
    out.push_back(e.partial ? kPartialMarker : kAlphabet[e.second]);
  }
  return out;
}

std::optional<Address> decode_token_id(const std::string &token_id) {
  if (token_id.empty()) {
    return Address{};
  }

  // Split on kDelimiter. Every part must be exactly two characters, so a
  // stray leading/trailing/doubled delimiter surfaces as an empty or
  // short part and is rejected below rather than silently skipped.
  std::vector<std::string> parts;
  std::size_t start = 0;
  while (true) {
    const std::size_t pos = token_id.find(kDelimiter, start);
    if (pos == std::string::npos) {
      parts.push_back(token_id.substr(start));
      break;
    }
    parts.push_back(token_id.substr(start, pos - start));
    start = pos + 1;
  }

  Address address;
  address.reserve(parts.size());
  for (std::size_t i = 0; i < parts.size(); ++i) {
    const std::string &part = parts[i];
    if (part.size() != 2) {
      return std::nullopt;
    }
    const char c0 = part[0];
    const char c1 = part[1];
    const bool is_last = (i + 1 == parts.size());

    if (c1 == kPartialMarker) {
      if (!is_last) {
        return std::nullopt;  // partial marker only valid on the last element
      }
      const int a = alphabet_index(c0);
      if (a < 0) {
        return std::nullopt;
      }
      address.push_back(make_partial_element(static_cast<uint8_t>(a)));
      continue;
    }

    const auto code = couplet_to_code(c0, c1);
    if (!code.has_value()) {
      return std::nullopt;
    }
    address.push_back(make_full_element(*code));
  }
  return address;
}

std::optional<PairKey> to_pair_key(const Address &address) {
  if (!is_valid_address(address)) {
    return std::nullopt;
  }
  PairKey key;
  key.reserve(address.size());
  for (const AddressElement &e : address) {
    if (e.partial) {
      return std::nullopt;
    }
    key.push_back(static_cast<uint16_t>(e.first * kAlphabetSize + e.second));
  }
  return key;
}

std::optional<Address> from_pair_key(const PairKey &key) {
  Address address;
  address.reserve(key.size());
  for (uint16_t code : key) {
    if (code >= kCoupletSpace) {
      return std::nullopt;
    }
    address.push_back(make_full_element(code));
  }
  return address;
}

std::optional<std::pair<uint16_t, uint16_t>> partial_code_range(
    const AddressElement &e) {
  if (!e.partial || !is_valid_element(e)) {
    return std::nullopt;
  }
  const uint16_t low = static_cast<uint16_t>(e.first * kAlphabetSize);
  return std::make_pair(low, static_cast<uint16_t>(low + kAlphabetSize - 1));
}

std::optional<Address> compute_delta(const Address &context,
                                      const Address &full) {
  if (context.size() > full.size()) {
    return std::nullopt;
  }
  return Address(full.begin() + static_cast<std::ptrdiff_t>(context.size()),
                 full.end());
}

Address reconstruct_address(const Address &context, const Address &delta) {
  Address out = context;
  out.insert(out.end(), delta.begin(), delta.end());
  return out;
}

}  // namespace codec
