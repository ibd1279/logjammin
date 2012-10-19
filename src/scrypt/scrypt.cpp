/*-
 * Copyright 2009 Colin Percival
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file was originally written by Colin Percival as part of the Tarsnap
 * online backup system.
 */

#include <sys/types.h>
#include <sys/mman.h>

#include <emmintrin.h>
#include <errno.h>
#include <cstdint>
#include <stdlib.h>
#include <cstring>

#include "scrypt/scrypt.h"

namespace
{
    inline uint32_t
    le32dec(const void *pp)
    {
            const uint8_t *p = (uint8_t const *)pp;

            return ((uint32_t)(p[0]) + ((uint32_t)(p[1]) << 8) +
                ((uint32_t)(p[2]) << 16) + ((uint32_t)(p[3]) << 24));
    }

    inline void
    le32enc(void *pp, uint32_t x)
    {
            uint8_t * p = (uint8_t *)pp;

            p[0] = x & 0xff;
            p[1] = (x >> 8) & 0xff;
            p[2] = (x >> 16) & 0xff;
            p[3] = (x >> 24) & 0xff;
    }

    inline uint32_t
    be32dec(const void *pp)
    {
            const uint8_t *p = (uint8_t const *)pp;

            return ((uint32_t)(p[3]) + ((uint32_t)(p[2]) << 8) +
                ((uint32_t)(p[1]) << 16) + ((uint32_t)(p[0]) << 24));
    }

    inline void
    be32enc(void *pp, uint32_t x)
    {
            uint8_t * p = (uint8_t *)pp;

            p[3] = x & 0xff;
            p[2] = (x >> 8) & 0xff;
            p[1] = (x >> 16) & 0xff;
            p[0] = (x >> 24) & 0xff;
    }

    /*
     * Encode a length len/4 vector of (uint32_t) into a length len vector of
     * (unsigned char) in big-endian form.  Assumes len is a multiple of 4.
     */
    void
    be32enc_vect(unsigned char *dst, const uint32_t *src, size_t len)
    {
            size_t i;

            for (i = 0; i < len / 4; i++)
                    be32enc(dst + i * 4, src[i]);
    }

    /*
     * Decode a big-endian length len vector of (unsigned char) into a length
     * len/4 vector of (uint32_t).  Assumes len is a multiple of 4.
     */
    void
    be32dec_vect(uint32_t *dst, const unsigned char *src, size_t len)
    {
            size_t i;

            for (i = 0; i < len / 4; i++)
                    dst[i] = be32dec(src + i * 4);
    }
};

//! Scrypt stuff.
typedef struct SHA256Context {
        uint32_t state[8]; //!< Scrypt stuff.
        uint32_t count[2]; //!< Scrypt stuff.
        unsigned char buf[64]; //!< Scrypt stuff.
} SHA256_CTX;

//! Scrypt stuff.
typedef struct HMAC_SHA256Context {
        SHA256_CTX ictx; //!< Scrypt stuff.
        SHA256_CTX octx; //!< Scrypt stuff.
} HMAC_SHA256_CTX;

/* Elementary functions used by SHA256 */
#define Ch(x, y, z)     ((x & (y ^ z)) ^ z)
#define Maj(x, y, z)    ((x & (y | z)) | (y & z))
#define SHR(x, n)       (x >> n)
#define ROTR(x, n)      ((x >> n) | (x << (32 - n)))
#define S0(x)           (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S1(x)           (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))
#define s0(x)           (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define s1(x)           (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

/* SHA256 round function */
#define RND(a, b, c, d, e, f, g, h, k)                  \
        t0 = h + S1(e) + Ch(e, f, g) + k;               \
        t1 = S0(a) + Maj(a, b, c);                      \
        d += t0;                                        \
        h  = t0 + t1;

/* Adjusted round function for rotating state */
#define RNDr(S, W, i, k)                        \
        RND(S[(64 - i) % 8], S[(65 - i) % 8],   \
            S[(66 - i) % 8], S[(67 - i) % 8],   \
            S[(68 - i) % 8], S[(69 - i) % 8],   \
            S[(70 - i) % 8], S[(71 - i) % 8],   \
            W[i] + k)

/*
 * SHA256 block compression function.  The 256-bit state is transformed via
 * the 512-bit input block to produce a new state.
 */
static void
SHA256_Transform(uint32_t * state, const unsigned char block[64])
{
        uint32_t W[64];
        uint32_t S[8];
        uint32_t t0, t1;
        int i;

        /* 1. Prepare message schedule W. */
        be32dec_vect(W, block, 64);
        for (i = 16; i < 64; i++)
                W[i] = s1(W[i - 2]) + W[i - 7] + s0(W[i - 15]) + W[i - 16];

        /* 2. Initialize working variables. */
        memcpy(S, state, 32);

        /* 3. Mix. */
        RNDr(S, W, 0, 0x428a2f98);
        RNDr(S, W, 1, 0x71374491);
        RNDr(S, W, 2, 0xb5c0fbcf);
        RNDr(S, W, 3, 0xe9b5dba5);
        RNDr(S, W, 4, 0x3956c25b);
        RNDr(S, W, 5, 0x59f111f1);
        RNDr(S, W, 6, 0x923f82a4);
        RNDr(S, W, 7, 0xab1c5ed5);
        RNDr(S, W, 8, 0xd807aa98);
        RNDr(S, W, 9, 0x12835b01);
        RNDr(S, W, 10, 0x243185be);
        RNDr(S, W, 11, 0x550c7dc3);
        RNDr(S, W, 12, 0x72be5d74);
        RNDr(S, W, 13, 0x80deb1fe);
        RNDr(S, W, 14, 0x9bdc06a7);
        RNDr(S, W, 15, 0xc19bf174);
        RNDr(S, W, 16, 0xe49b69c1);
        RNDr(S, W, 17, 0xefbe4786);
        RNDr(S, W, 18, 0x0fc19dc6);
        RNDr(S, W, 19, 0x240ca1cc);
        RNDr(S, W, 20, 0x2de92c6f);
        RNDr(S, W, 21, 0x4a7484aa);
        RNDr(S, W, 22, 0x5cb0a9dc);
        RNDr(S, W, 23, 0x76f988da);
        RNDr(S, W, 24, 0x983e5152);
        RNDr(S, W, 25, 0xa831c66d);
        RNDr(S, W, 26, 0xb00327c8);
        RNDr(S, W, 27, 0xbf597fc7);
        RNDr(S, W, 28, 0xc6e00bf3);
        RNDr(S, W, 29, 0xd5a79147);
        RNDr(S, W, 30, 0x06ca6351);
        RNDr(S, W, 31, 0x14292967);
        RNDr(S, W, 32, 0x27b70a85);
        RNDr(S, W, 33, 0x2e1b2138);
        RNDr(S, W, 34, 0x4d2c6dfc);
        RNDr(S, W, 35, 0x53380d13);
        RNDr(S, W, 36, 0x650a7354);
        RNDr(S, W, 37, 0x766a0abb);
        RNDr(S, W, 38, 0x81c2c92e);
        RNDr(S, W, 39, 0x92722c85);
        RNDr(S, W, 40, 0xa2bfe8a1);
        RNDr(S, W, 41, 0xa81a664b);
        RNDr(S, W, 42, 0xc24b8b70);
        RNDr(S, W, 43, 0xc76c51a3);
        RNDr(S, W, 44, 0xd192e819);
        RNDr(S, W, 45, 0xd6990624);
        RNDr(S, W, 46, 0xf40e3585);
        RNDr(S, W, 47, 0x106aa070);
        RNDr(S, W, 48, 0x19a4c116);
        RNDr(S, W, 49, 0x1e376c08);
        RNDr(S, W, 50, 0x2748774c);
        RNDr(S, W, 51, 0x34b0bcb5);
        RNDr(S, W, 52, 0x391c0cb3);
        RNDr(S, W, 53, 0x4ed8aa4a);
        RNDr(S, W, 54, 0x5b9cca4f);
        RNDr(S, W, 55, 0x682e6ff3);
        RNDr(S, W, 56, 0x748f82ee);
        RNDr(S, W, 57, 0x78a5636f);
        RNDr(S, W, 58, 0x84c87814);
        RNDr(S, W, 59, 0x8cc70208);
        RNDr(S, W, 60, 0x90befffa);
        RNDr(S, W, 61, 0xa4506ceb);
        RNDr(S, W, 62, 0xbef9a3f7);
        RNDr(S, W, 63, 0xc67178f2);

        /* 4. Mix local working variables into global state */
        for (i = 0; i < 8; i++)
                state[i] += S[i];

        /* Clean the stack. */
        memset(W, 0, 256);
        memset(S, 0, 32);
        t0 = t1 = 0;
}

static unsigned char PAD[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Add padding and terminating bit-count. */
void
SHA256_Update(SHA256_CTX * ctx, const void *in, size_t len);

static void
SHA256_Pad(SHA256_CTX * ctx)
{
        unsigned char len[8];
        uint32_t r, plen;

        /*
         * Convert length to a vector of bytes -- we do this now rather
         * than later because the length will change after we pad.
         */
        be32enc_vect(len, ctx->count, 8);

        /* Add 1--64 bytes so that the resulting length is 56 mod 64 */
        r = (ctx->count[1] >> 3) & 0x3f;
        plen = (r < 56) ? (56 - r) : (120 - r);
        SHA256_Update(ctx, PAD, (size_t)plen);

        /* Add the terminating bit-count */
        SHA256_Update(ctx, len, 8);
}

/* SHA-256 initialization.  Begins a SHA-256 operation. */
void
SHA256_Init(SHA256_CTX * ctx)
{

        /* Zero bits processed so far */
        ctx->count[0] = ctx->count[1] = 0;

        /* Magic initialization constants */
        ctx->state[0] = 0x6A09E667;
        ctx->state[1] = 0xBB67AE85;
        ctx->state[2] = 0x3C6EF372;
        ctx->state[3] = 0xA54FF53A;
        ctx->state[4] = 0x510E527F;
        ctx->state[5] = 0x9B05688C;
        ctx->state[6] = 0x1F83D9AB;
        ctx->state[7] = 0x5BE0CD19;
}

/* Add bytes into the hash */
void
SHA256_Update(SHA256_CTX * ctx, const void *in, size_t len)
{
        uint32_t bitlen[2];
        uint32_t r;
        const unsigned char *src = static_cast<const unsigned char*>(in);

        /* Number of bytes left in the buffer from previous updates */
        r = (ctx->count[1] >> 3) & 0x3f;

        /* Convert the length into a number of bits */
        bitlen[1] = ((uint32_t)len) << 3;
        bitlen[0] = (uint32_t)(len >> 29);

        /* Update number of bits */
        if ((ctx->count[1] += bitlen[1]) < bitlen[1])
                ctx->count[0]++;
        ctx->count[0] += bitlen[0];

        /* Handle the case where we don't need to perform any transforms */
        if (len < 64 - r) {
                memcpy(&ctx->buf[r], src, len);
                return;
        }

        /* Finish the current block */
        memcpy(&ctx->buf[r], src, 64 - r);
        SHA256_Transform(ctx->state, ctx->buf);
        src += 64 - r;
        len -= 64 - r;

        /* Perform complete blocks */
        while (len >= 64) {
                SHA256_Transform(ctx->state, src);
                src += 64;
                len -= 64;
        }

        /* Copy left over data into buffer */
        memcpy(ctx->buf, src, len);
}

/*
 * SHA-256 finalization.  Pads the input data, exports the hash value,
 * and clears the context state.
 */
void
SHA256_Final(unsigned char digest[32], SHA256_CTX * ctx)
{

        /* Add padding */
        SHA256_Pad(ctx);

        /* Write the hash */
        be32enc_vect(digest, ctx->state, 32);

        /* Clear the context state */
        memset((void *)ctx, 0, sizeof(*ctx));
}

/* Initialize an HMAC-SHA256 operation with the given key. */
void
HMAC_SHA256_Init(HMAC_SHA256_CTX * ctx, const void * _K, size_t Klen)
{
        unsigned char pad[64];
        unsigned char khash[32];
        const unsigned char * K = static_cast<const unsigned char*>(_K);
        size_t i;

        /* If Klen > 64, the key is really SHA256(K). */
        if (Klen > 64) {
                SHA256_Init(&ctx->ictx);
                SHA256_Update(&ctx->ictx, K, Klen);
                SHA256_Final(khash, &ctx->ictx);
                K = khash;
                Klen = 32;
        }

        /* Inner SHA256 operation is SHA256(K xor [block of 0x36] || data). */
        SHA256_Init(&ctx->ictx);
        memset(pad, 0x36, 64);
        for (i = 0; i < Klen; i++)
                pad[i] ^= K[i];
        SHA256_Update(&ctx->ictx, pad, 64);

        /* Outer SHA256 operation is SHA256(K xor [block of 0x5c] || hash). */
        SHA256_Init(&ctx->octx);
        memset(pad, 0x5c, 64);
        for (i = 0; i < Klen; i++)
                pad[i] ^= K[i];
        SHA256_Update(&ctx->octx, pad, 64);

        /* Clean the stack. */
        memset(khash, 0, 32);
}

/* Add bytes to the HMAC-SHA256 operation. */
void
HMAC_SHA256_Update(HMAC_SHA256_CTX * ctx, const void *in, size_t len)
{

        /* Feed data to the inner SHA256 operation. */
        SHA256_Update(&ctx->ictx, in, len);
}

/* Finish an HMAC-SHA256 operation. */
void
HMAC_SHA256_Final(unsigned char digest[32], HMAC_SHA256_CTX * ctx)
{
        unsigned char ihash[32];

        /* Finish the inner SHA256 operation. */
        SHA256_Final(ihash, &ctx->ictx);

        /* Feed the inner hash to the outer SHA256 operation. */
        SHA256_Update(&ctx->octx, ihash, 32);

        /* Finish the outer SHA256 operation. */
        SHA256_Final(digest, &ctx->octx);

        /* Clean the stack. */
        memset(ihash, 0, 32);
}

/**
 * PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, c, buf, dkLen):
 * Compute PBKDF2(passwd, salt, c, dkLen) using HMAC-SHA256 as the PRF, and
 * write the output to buf.  The value dkLen must be at most 32 * (2^32 - 1).
 */
void
PBKDF2_SHA256(const uint8_t * passwd, size_t passwdlen, const uint8_t * salt,
    size_t saltlen, uint64_t c, uint8_t * buf, size_t dkLen)
{
        HMAC_SHA256_CTX PShctx, hctx;
        size_t i;
        uint8_t ivec[4];
        uint8_t U[32];
        uint8_t T[32];
        uint64_t j;
        int k;
        size_t clen;

        /* Compute HMAC state after processing P and S. */
        HMAC_SHA256_Init(&PShctx, passwd, passwdlen);
        HMAC_SHA256_Update(&PShctx, salt, saltlen);

        /* Iterate through the blocks. */
        for (i = 0; i * 32 < dkLen; i++) {
                /* Generate INT(i + 1). */
                be32enc(ivec, (uint32_t)(i + 1));

                /* Compute U_1 = PRF(P, S || INT(i)). */
                memcpy(&hctx, &PShctx, sizeof(HMAC_SHA256_CTX));
                HMAC_SHA256_Update(&hctx, ivec, 4);
                HMAC_SHA256_Final(U, &hctx);

                /* T_i = U_1 ... */
                memcpy(T, U, 32);

                for (j = 2; j <= c; j++) {
                        /* Compute U_j. */
                        HMAC_SHA256_Init(&hctx, passwd, passwdlen);
                        HMAC_SHA256_Update(&hctx, U, 32);
                        HMAC_SHA256_Final(U, &hctx);

                        /* ... xor U_j ... */
                        for (k = 0; k < 32; k++)
                                T[k] ^= U[k];
                }

                /* Copy as many bytes as necessary into buf. */
                clen = dkLen - i * 32;
                if (clen > 32)
                        clen = 32;
                memcpy(&buf[i * 32], T, clen);
        }

        /* Clean PShctx, since we never called _Final on it. */
        memset(&PShctx, 0, sizeof(HMAC_SHA256_CTX));
}

static void blkcpy(void *, void *, size_t);
static void blkxor(void *, void *, size_t);
static void salsa20_8(__m128i *);
static void blockmix_salsa8(__m128i *, __m128i *, __m128i *, size_t);
static uint64_t integerify(void *, size_t);
static void smix(uint8_t *, size_t, uint64_t, void *, void *);

static void
blkcpy(void * dest, void * src, size_t len)
{
        __m128i * D = static_cast<__m128i*>(dest);
        __m128i * S = static_cast<__m128i*>(src);
        size_t L = len / 16;
        size_t i;

        for (i = 0; i < L; i++)
                D[i] = S[i];
}

static void
blkxor(void * dest, void * src, size_t len)
{
        __m128i * D = static_cast<__m128i*>(dest);
        __m128i * S = static_cast<__m128i*>(src);
        size_t L = len / 16;
        size_t i;

        for (i = 0; i < L; i++)
                D[i] = _mm_xor_si128(D[i], S[i]);
}

/**
 * salsa20_8(B):
 * Apply the salsa20/8 core to the provided block.
 */
static void
salsa20_8(__m128i B[4])
{
        __m128i X0, X1, X2, X3;
        __m128i T;
        size_t i;

        X0 = B[0];
        X1 = B[1];
        X2 = B[2];
        X3 = B[3];

        for (i = 0; i < 8; i += 2) {
                /* Operate on "columns". */
                T = _mm_add_epi32(X0, X3);
                X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 7));
                X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 25));
                T = _mm_add_epi32(X1, X0);
                X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
                X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
                T = _mm_add_epi32(X2, X1);
                X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 13));
                X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 19));
                T = _mm_add_epi32(X3, X2);
                X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
                X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

                /* Rearrange data. */
                X1 = _mm_shuffle_epi32(X1, 0x93);
                X2 = _mm_shuffle_epi32(X2, 0x4E);
                X3 = _mm_shuffle_epi32(X3, 0x39);

                /* Operate on "rows". */
                T = _mm_add_epi32(X0, X1);
                X3 = _mm_xor_si128(X3, _mm_slli_epi32(T, 7));
                X3 = _mm_xor_si128(X3, _mm_srli_epi32(T, 25));
                T = _mm_add_epi32(X3, X0);
                X2 = _mm_xor_si128(X2, _mm_slli_epi32(T, 9));
                X2 = _mm_xor_si128(X2, _mm_srli_epi32(T, 23));
                T = _mm_add_epi32(X2, X3);
                X1 = _mm_xor_si128(X1, _mm_slli_epi32(T, 13));
                X1 = _mm_xor_si128(X1, _mm_srli_epi32(T, 19));
                T = _mm_add_epi32(X1, X2);
                X0 = _mm_xor_si128(X0, _mm_slli_epi32(T, 18));
                X0 = _mm_xor_si128(X0, _mm_srli_epi32(T, 14));

                /* Rearrange data. */
                X1 = _mm_shuffle_epi32(X1, 0x39);
                X2 = _mm_shuffle_epi32(X2, 0x4E);
                X3 = _mm_shuffle_epi32(X3, 0x93);
        }

        B[0] = _mm_add_epi32(B[0], X0);
        B[1] = _mm_add_epi32(B[1], X1);
        B[2] = _mm_add_epi32(B[2], X2);
        B[3] = _mm_add_epi32(B[3], X3);
}

/**
 * blockmix_salsa8(Bin, Bout, X, r):
 * Compute Bout = BlockMix_{salsa20/8, r}(Bin).  The input Bin must be 128r
 * bytes in length; the output Bout must also be the same size.  The
 * temporary space X must be 64 bytes.
 */
static void
blockmix_salsa8(__m128i * Bin, __m128i * Bout, __m128i * X, size_t r)
{
        size_t i;

        /* 1: X <-- B_{2r - 1} */
        blkcpy(X, &Bin[8 * r - 4], 64);

        /* 2: for i = 0 to 2r - 1 do */
        for (i = 0; i < r; i++) {
                /* 3: X <-- H(X \xor B_i) */
                blkxor(X, &Bin[i * 8], 64);
                salsa20_8(X);

                /* 4: Y_i <-- X */
                /* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
                blkcpy(&Bout[i * 4], X, 64);

                /* 3: X <-- H(X \xor B_i) */
                blkxor(X, &Bin[i * 8 + 4], 64);
                salsa20_8(X);

                /* 4: Y_i <-- X */
                /* 6: B' <-- (Y_0, Y_2 ... Y_{2r-2}, Y_1, Y_3 ... Y_{2r-1}) */
                blkcpy(&Bout[(r + i) * 4], X, 64);
        }
}

/**
 * integerify(B, r):
 * Return the result of parsing B_{2r-1} as a little-endian integer.
 */
static uint64_t
integerify(void * B, size_t r)
{
        uint32_t * X = reinterpret_cast<uint32_t*>((uintptr_t)(B) + (2 * r - 1) * 64);

        return (((uint64_t)(X[13]) << 32) + X[0]);
}

/**
 * smix(B, r, N, V, XY):
 * Compute B = SMix_r(B, N).  The input B must be 128r bytes in length;
 * the temporary storage V must be 128rN bytes in length; the temporary
 * storage XY must be 256r + 64 bytes in length.  The value N must be a
 * power of 2 greater than 1.  The arrays B, V, and XY must be aligned to a
 * multiple of 64 bytes.
 */
static void
smix(uint8_t * B, size_t r, uint64_t N, void * V, void * XY)
{
        __m128i * X = static_cast<__m128i*>(XY);
        __m128i * Y = reinterpret_cast<__m128i*>((uintptr_t)(XY) + 128 * r);
        __m128i * Z = reinterpret_cast<__m128i*>((uintptr_t)(XY) + 256 * r);
        uint32_t * X32 = reinterpret_cast<uint32_t*>(X);
        uint64_t i, j;
        size_t k;

        /* 1: X <-- B */
        for (k = 0; k < 2 * r; k++) {
                for (i = 0; i < 16; i++) {
                        X32[k * 16 + i] =
                            le32dec(&B[(k * 16 + (i * 5 % 16)) * 4]);
                }
        }

        /* 2: for i = 0 to N - 1 do */
        for (i = 0; i < N; i += 2) {
                /* 3: V_i <-- X */
                blkcpy((void *)((uintptr_t)(V) + i * 128 * r), X, 128 * r);

                /* 4: X <-- H(X) */
                blockmix_salsa8(X, Y, Z, r);

                /* 3: V_i <-- X */
                blkcpy((void *)((uintptr_t)(V) + (i + 1) * 128 * r),
                    Y, 128 * r);

                /* 4: X <-- H(X) */
                blockmix_salsa8(Y, X, Z, r);
        }

        /* 6: for i = 0 to N - 1 do */
        for (i = 0; i < N; i += 2) {
                /* 7: j <-- Integerify(X) mod N */
                j = integerify(X, r) & (N - 1);

                /* 8: X <-- H(X \xor V_j) */
                blkxor(X, (void *)((uintptr_t)(V) + j * 128 * r), 128 * r);
                blockmix_salsa8(X, Y, Z, r);

                /* 7: j <-- Integerify(X) mod N */
                j = integerify(Y, r) & (N - 1);

                /* 8: X <-- H(X \xor V_j) */
                blkxor(Y, (void *)((uintptr_t)(V) + j * 128 * r), 128 * r);
                blockmix_salsa8(Y, X, Z, r);
        }

        /* 10: B' <-- X */
        for (k = 0; k < 2 * r; k++) {
                for (i = 0; i < 16; i++) {
                        le32enc(&B[(k * 16 + (i * 5 % 16)) * 4],
                            X32[k * 16 + i]);
                }
        }
}

/**
 * crypto_scrypt(passwd, passwdlen, salt, saltlen, N, r, p, buf, buflen):
 * Compute scrypt(passwd[0 .. passwdlen - 1], salt[0 .. saltlen - 1], N, r,
 * p, buflen) and write the result into buf.  The parameters r, p, and buflen
 * must satisfy r * p < 2^30 and buflen <= (2^32 - 1) * 32.  The parameter N
 * must be a power of 2 greater than 1.
 *
 * Return 0 on success; or -1 on error.
 */
int
crypto_scrypt(const uint8_t * passwd, size_t passwdlen,
    const uint8_t * salt, size_t saltlen, uint64_t N, uint32_t r, uint32_t p,
    uint8_t * buf, size_t buflen)
{
        void * B0, * V0, * XY0;
        uint8_t * B;
        uint32_t * V;
        uint32_t * XY;
        uint32_t i;

        /* Sanity-check parameters. */
#if SIZE_MAX > UINT32_MAX
        if (buflen > (((uint64_t)(1) << 32) - 1) * 32) {
                errno = EFBIG;
                goto err0;
        }
#endif
        if ((uint64_t)(r) * (uint64_t)(p) >= (1 << 30)) {
                errno = EFBIG;
                goto err0;
        }
        if (((N & (N - 1)) != 0) || (N == 0)) {
                errno = EINVAL;
                goto err0;
        }
        if ((r > SIZE_MAX / 128 / p) ||
#if SIZE_MAX / 256 <= UINT32_MAX
            (r > (SIZE_MAX - 64) / 256) ||
#endif
            (N > SIZE_MAX / 128 / r)) {
                errno = ENOMEM;
                goto err0;
        }

        /* Allocate memory. */
        if ((errno = posix_memalign(&B0, 64, 128 * r * p)) != 0)
                goto err0;
        B = (uint8_t *)(B0);
        if ((errno = posix_memalign(&XY0, 64, 256 * r + 64)) != 0)
                goto err1;
        XY = (uint32_t *)(XY0);
#ifndef MAP_ANON
        if ((errno = posix_memalign(&V0, 64, 128 * r * N)) != 0)
                goto err2;
        V = (uint32_t *)(V0);
#endif
#ifdef MAP_ANON
        if ((V0 = mmap(NULL, 128 * r * N, PROT_READ | PROT_WRITE,
#ifdef MAP_NOCORE
            MAP_ANON | MAP_PRIVATE | MAP_NOCORE,
#else
            MAP_ANON | MAP_PRIVATE,
#endif
            -1, 0)) == MAP_FAILED)
                goto err2;
        V = (uint32_t *)(V0);
#endif

        /* 1: (B_0 ... B_{p-1}) <-- PBKDF2(P, S, 1, p * MFLen) */
        PBKDF2_SHA256(passwd, passwdlen, salt, saltlen, 1, B, p * 128 * r);

        /* 2: for i = 0 to p - 1 do */
        for (i = 0; i < p; i++) {
                /* 3: B_i <-- MF(B_i, N) */
                smix(&B[i * 128 * r], r, N, V, XY);
        }

        /* 5: DK <-- PBKDF2(P, B, 1, dkLen) */
        PBKDF2_SHA256(passwd, passwdlen, B, p * 128 * r, 1, buf, buflen);

        /* Free memory. */
#ifdef MAP_ANON
        if (munmap(V0, 128 * r * N))
                goto err2;
#else
        free(V0);
#endif
        free(XY0);
        free(B0);

        /* Success! */
        return (0);

err2:
        free(XY0);
err1:
        free(B0);
err0:
        /* Failure! */
        return (-1);
}

