// Compact MD5 (RFC 1321 derived, public-domain style implementation).
// Used for collapse keys only — must produce identical hex to Postgres md5().
#pragma once
#include <cstdint>
#include <cstring>
#include <string>

namespace hcp::md5 {

struct Ctx { uint32_t a0=0x67452301, b0=0xefcdab89, c0=0x98badcfe, d0=0x10325476; };

inline uint32_t rotl(uint32_t x, int c) { return (x << c) | (x >> (32 - c)); }

inline void processBlock(Ctx& ctx, const uint8_t* p)
{
    static const uint32_t K[64] = {
        0xd76aa478,0xe8c7b756,0x242070db,0xc1bdceee,0xf57c0faf,0x4787c62a,0xa8304613,0xfd469501,
        0x698098d8,0x8b44f7af,0xffff5bb1,0x895cd7be,0x6b901122,0xfd987193,0xa679438e,0x49b40821,
        0xf61e2562,0xc040b340,0x265e5a51,0xe9b6c7aa,0xd62f105d,0x02441453,0xd8a1e681,0xe7d3fbc8,
        0x21e1cde6,0xc33707d6,0xf4d50d87,0x455a14ed,0xa9e3e905,0xfcefa3f8,0x676f02d9,0x8d2a4c8a,
        0xfffa3942,0x8771f681,0x6d9d6122,0xfde5380c,0xa4beea44,0x4bdecfa9,0xf6bb4b60,0xbebfbc70,
        0x289b7ec6,0xeaa127fa,0xd4ef3085,0x04881d05,0xd9d4d039,0xe6db99e5,0x1fa27cf8,0xc4ac5665,
        0xf4292244,0x432aff97,0xab9423a7,0xfc93a039,0x655b59c3,0x8f0ccc92,0xffeff47d,0x85845dd1,
        0x6fa87e4f,0xfe2ce6e0,0xa3014314,0x4e0811a1,0xf7537e82,0xbd3af235,0x2ad7d2bb,0xeb86d391 };
    static const int S[64] = {
        7,12,17,22,7,12,17,22,7,12,17,22,7,12,17,22,
        5, 9,14,20,5, 9,14,20,5, 9,14,20,5, 9,14,20,
        4,11,16,23,4,11,16,23,4,11,16,23,4,11,16,23,
        6,10,15,21,6,10,15,21,6,10,15,21,6,10,15,21 };
    uint32_t M[16];
    std::memcpy(M, p, 64);
    uint32_t A = ctx.a0, B = ctx.b0, C = ctx.c0, D = ctx.d0;
    for (int i = 0; i < 64; ++i) {
        uint32_t F; int g;
        if (i < 16)      { F = (B & C) | (~B & D);  g = i; }
        else if (i < 32) { F = (D & B) | (~D & C);  g = (5*i + 1) % 16; }
        else if (i < 48) { F = B ^ C ^ D;           g = (3*i + 5) % 16; }
        else             { F = C ^ (B | ~D);        g = (7*i) % 16; }
        F = F + A + K[i] + M[g];
        A = D; D = C; C = B;
        B = B + rotl(F, S[i]);
    }
    ctx.a0 += A; ctx.b0 += B; ctx.c0 += C; ctx.d0 += D;
}

inline std::string hex(const std::string& msg)
{
    Ctx ctx;
    size_t n = msg.size();
    size_t i = 0;
    for (; i + 64 <= n; i += 64) processBlock(ctx, reinterpret_cast<const uint8_t*>(msg.data()) + i);
    uint8_t tail[128] = {0};
    size_t rem = n - i;
    std::memcpy(tail, msg.data() + i, rem);
    tail[rem] = 0x80;
    size_t tlen = (rem < 56) ? 64 : 128;
    uint64_t bits = static_cast<uint64_t>(n) * 8;
    std::memcpy(tail + tlen - 8, &bits, 8);
    processBlock(ctx, tail);
    if (tlen == 128) processBlock(ctx, tail + 64);
    uint8_t out[16];
    std::memcpy(out + 0,  &ctx.a0, 4); std::memcpy(out + 4,  &ctx.b0, 4);
    std::memcpy(out + 8,  &ctx.c0, 4); std::memcpy(out + 12, &ctx.d0, 4);
    static const char* h = "0123456789abcdef";
    std::string s(32, '0');
    for (int k = 0; k < 16; ++k) { s[2*k] = h[out[k] >> 4]; s[2*k+1] = h[out[k] & 15]; }
    return s;
}

} // namespace hcp::md5
