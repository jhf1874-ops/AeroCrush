#pragma once
// Lo-fi / colour modules inspired by SP-404 effects and RC-20 style "tone" sections.
// Pure C++ (no JUCE). All process() calls work in place on stereo buffers.
#include <vector>
#include <cmath>
#include <algorithm>
#include "Biquad.h"

namespace ac
{
inline float dbToGain (float db) { return db <= -39.9f ? 0.0f : std::pow (10.0f, db / 20.0f); }

// DIGITAL: bit depth reduction + sample-rate reduction (sample & hold).
struct Crusher
{
    float hold[2] {}, phase = 1.0f;
    void process (float* L, float* R, int n, float bits, float factor)
    {
        factor = std::max (1.0f, factor);
        const float levels = std::pow (2.0f, std::clamp (bits, 1.0f, 16.0f) - 1.0f);
        for (int i = 0; i < n; ++i)
        {
            phase += 1.0f;
            if (phase >= factor) { phase -= factor; hold[0] = L[i]; hold[1] = R[i]; }
            L[i] = std::round (hold[0] * levels) / levels;
            R[i] = std::round (hold[1] * levels) / levels;
        }
    }
};

// DRIVE: asymmetric tanh saturation with a tone low-pass and DC blocker.
struct Drive
{
    double sr = 44100;
    Biquad tone;
    float x1[2] {}, y1[2] {};
    void prepare (double s) { sr = s; tone.reset(); }
    void process (float* L, float* R, int n, float amt, float toneHz)
    {
        tone.setLP (sr, toneHz, 0.707f);
        const float pre = 1.0f + amt * 24.0f, bias = 0.15f * amt, tb = std::tanh (bias);
        const float comp = 1.0f / std::sqrt (pre);
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                float y = (std::tanh (pre * ch[c][i] + bias) - tb) * comp;
                const float dc = y - x1[c] + 0.995f * y1[c];
                x1[c] = y; y1[c] = dc;
                ch[c][i] = tone.process (dc, c);
            }
    }
};

// WOBBLE: tape wow (slow, drifting) + flutter (fast) via modulated delay.
struct Wobble
{
    double sr = 44100;
    std::vector<float> buf[2];
    int size = 1, w = 0, rndCount = 0;
    float wowPh = 0, flPh = 0, rnd = 0, rndTarget = 0;
    Rng rng;
    void prepare (double s)
    {
        sr = s; size = (int) (s * 0.1) + 4; w = 0;
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
    }
    void process (float* L, float* R, int n, float wow, float flutter)
    {
        for (int i = 0; i < n; ++i)
        {
            buf[0][(size_t) w] = L[i]; buf[1][(size_t) w] = R[i];
            wowPh += (float) (0.55 / sr); if (wowPh >= 1) wowPh -= 1;
            flPh  += (float) (6.5 / sr);  if (flPh  >= 1) flPh  -= 1;
            if (--rndCount <= 0) { rndTarget = rng.nextBi(); rndCount = (int) (sr * 0.4); }
            rnd += (rndTarget - rnd) * (float) (3.0 / sr);

            const float ms = 12.0f + wow * 6.0f * (0.7f * std::sin (2 * kPi * wowPh) + 0.6f * rnd)
                                   + flutter * 0.8f * std::sin (2 * kPi * flPh);
            const double idx = w - ms * 0.001 * sr;
            const double fl = std::floor (idx);
            const float frac = (float) (idx - fl);
            const int i0 = (((int) fl) % size + size) % size, i1 = (i0 + 1) % size;
            L[i] = buf[0][(size_t) i0] + frac * (buf[0][(size_t) i1] - buf[0][(size_t) i0]);
            R[i] = buf[1][(size_t) i0] + frac * (buf[1][(size_t) i1] - buf[1][(size_t) i0]);
            w = (w + 1) % size;
        }
    }
};

// MAGNETIC: random tape dropouts + level flux (RC-20 "Magnetic"-ish).
struct Magnetic
{
    double sr = 44100;
    Rng rng;
    float gain = 1, target = 1, flux = 0;
    int remain = 0;
    void prepare (double s) { sr = s; }
    void process (float* L, float* R, int n, float drop, float depth)
    {
        const float chance = drop * drop * 3.0f / (float) sr; // up to ~3 dropouts / sec
        const float slew = 1.0f - std::exp (-1.0f / (float) (0.004 * sr));
        const float fluxCoef = 1.0f - std::exp (-2.0f * kPi * 8.0f / (float) sr);
        for (int i = 0; i < n; ++i)
        {
            if (remain > 0) { if (--remain == 0) target = 1.0f; }
            else if (rng.next01() < chance)
            {
                remain = (int) (sr * (0.03 + 0.17 * rng.next01()));
                target = 1.0f - depth * (0.6f + 0.4f * rng.next01());
            }
            gain += (target - gain) * slew;
            flux += (rng.nextBi() - flux) * fluxCoef;
            const float g = gain * (1.0f - depth * 0.25f * std::min (1.0f, std::fabs (flux) * 30.0f));
            L[i] *= g; R[i] *= g;
        }
    }
};

// NOISE: tape hiss + vinyl crackle, added on top of the signal.
struct Noise
{
    double sr = 44100;
    Rng rng;
    Biquad hissLP, crackleHP;
    float pop[2] {}, popDecay = 0.99f;
    void prepare (double s)
    {
        sr = s;
        hissLP.setLP (s, 7000.0f, 0.707f);
        crackleHP.setHP (s, 900.0f, 0.707f);
        popDecay = std::exp (-1.0f / (float) (0.0008 * s));
    }
    void process (float* L, float* R, int n, float hiss, float crackle)
    {
        const float popChance = crackle * crackle * 0.0012f;
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float h = hissLP.process (rng.nextBi(), c) * hiss * 0.06f;
                if (rng.next01() < popChance)
                    pop[c] = rng.nextBi() * (0.2f + 0.8f * rng.next01()) * 0.6f;
                const float k = crackleHP.process (pop[c], c);
                pop[c] *= popDecay;
                ch[c][i] += h + k * crackle;
            }
    }
};

// ISOLATOR: SP-404 style 3-band kill EQ. Bands sum back to the input at 0 dB.
struct Isolator
{
    Biquad lp1, lp2, hp1, hp2;
    void prepare (double sr)
    {
        lp1.setLP (sr, 250.0f, 0.7071f); lp2.setLP (sr, 250.0f, 0.7071f);
        hp1.setHP (sr, 3000.0f, 0.7071f); hp2.setHP (sr, 3000.0f, 0.7071f);
        lp1.reset(); lp2.reset(); hp1.reset(); hp2.reset();
    }
    void process (float* L, float* R, int n, float lowDb, float midDb, float highDb)
    {
        const float gl = dbToGain (lowDb), gm = dbToGain (midDb), gh = dbToGain (highDb);
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
            {
                const float x = ch[c][i];
                const float lo = lp2.process (lp1.process (x, c), c);
                const float hi = hp2.process (hp1.process (x, c), c);
                ch[c][i] = lo * gl + (x - lo - hi) * gm + hi * gh;
            }
    }
};

// FILTER: resonant high-pass + low-pass.
struct DualFilter
{
    double sr = 44100;
    Biquad hp, lp;
    void prepare (double s) { sr = s; hp.reset(); lp.reset(); }
    void process (float* L, float* R, int n, float loCut, float hiCut, float reso)
    {
        const float q = 0.707f + reso * 6.0f;
        hp.setHP (sr, loCut, q);
        lp.setLP (sr, hiCut, q);
        float* ch[2] = { L, R };
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < n; ++i)
                ch[c][i] = lp.process (hp.process (ch[c][i], c), c);
    }
};
} // namespace ac
