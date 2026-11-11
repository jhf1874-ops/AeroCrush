#pragma once
// Pure C++ (no JUCE) so the DSP can be unit-tested on its own.
#include <cmath>
#include <cstdint>

namespace ac
{
constexpr float kPi = 3.14159265358979f;

// Fast xorshift RNG: allocation-free and safe on the audio thread.
struct Rng
{
    uint32_t s = 0x9E3779B9u;
    float next01() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float) (s >> 8) * (1.0f / 16777216.0f); }
    float nextBi() { return next01() * 2.0f - 1.0f; }
};

// RBJ biquad, transposed direct form II, stereo state.
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1[2] {}, z2[2] {};

    void setLP (double sr, float f, float q) { calc (sr, f, q, false); }
    void setHP (double sr, float f, float q) { calc (sr, f, q, true); }
    void reset() { z1[0] = z1[1] = z2[0] = z2[1] = 0; }

    float process (float x, int ch)
    {
        const float y = b0 * x + z1[ch];
        z1[ch] = b1 * x - a1 * y + z2[ch];
        z2[ch] = b2 * x - a2 * y;
        return y;
    }

private:
    void calc (double sr, float f, float q, bool high)
    {
        f = std::fmin (std::fmax (f, 10.0f), (float) (sr * 0.45));
        const float w = 2.0f * kPi * f / (float) sr, c = std::cos (w), s = std::sin (w);
        const float al = s / (2.0f * std::fmax (q, 0.1f)), a0 = 1.0f + al;
        if (high) { b0 = (1 + c) * 0.5f; b1 = -(1 + c); b2 = (1 + c) * 0.5f; }
        else      { b0 = (1 - c) * 0.5f; b1 =  (1 - c); b2 = (1 - c) * 0.5f; }
        a1 = -2.0f * c; a2 = 1.0f - al;
        b0 /= a0; b1 /= a0; b2 /= a0; a1 /= a0; a2 /= a0;
    }
};
} // namespace ac
