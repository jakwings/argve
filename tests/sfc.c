/*\
 / http://pracrand.sourceforge.net/
 / Released to the Public Domain by Chris Doty-Humphrey
 / Adapted by J.W https://github.com/jakwings
\*/

#include <stdint.h>

typedef struct { uint64_t a, b, c, counter; } sfc64;

static uint64_t sfc64_raw(sfc64 *rng)
{
    enum { BARREL_SHIFT = 24, RSHIFT = 11, LSHIFT = 3 };
    uint64_t a = rng->a, b = rng->b, c = rng->c;
    uint64_t tmp = a + b + rng->counter++;
    rng->a = b ^ (b >> RSHIFT);
    rng->b = c + (c << LSHIFT);
    rng->c = ((c << BARREL_SHIFT) | (c >> (64 - BARREL_SHIFT))) + tmp;
    return tmp;
}

static uint64_t sfc64_rand(sfc64 *rng, uint64_t max)
{
    uint64_t mask, tmp;
    max -= 1;
    mask = max;
    mask |= mask >> 1; mask |= mask >>  2; mask |= mask >>  4;
    mask |= mask >> 8; mask |= mask >> 16; mask |= mask >> 32;
    do { tmp = sfc64_raw(rng) & mask; } while (tmp > max);
    return tmp;
}

static void sfc64_seed(sfc64 *rng, uint64_t seed)
{
    int i;
    rng->a = rng->b = rng->c = seed;
    rng->counter = 1;
    for (i = 0; i < 12; i++) sfc64_raw(rng);
}
