#include <cstdint>
#include <cstdio>
#include "encoding_populate.h"

int main() {
  std::uint64_t count = 0;
  for (std::uint32_t cp = 0; cp <= 0x10FFFF; ++cp) {
    if (cp >= 0xD800 && cp <= 0xDFFF) continue;
    auto address = cp < 128 ? dbk::bootstrap::byte_code_address(cp)
                           : dbk::bootstrap::character_address(cp);
    auto encoded = codec::encode_token_id(address);
    if (!encoded) return 1;
    std::printf("%u\t%s\t", cp, encoded->c_str());
    for (auto byte : dbk::bootstrap::utf8_encode(cp)) std::printf("%02X", byte);
    std::printf("\n");
    ++count;
  }
  return count == 1112064 ? 0 : 2;
}
