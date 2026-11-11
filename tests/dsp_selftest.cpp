// Standalone DSP self-test (no JUCE). Build: c++ -std=c++17 -O2 -I../Source/DSP dsp_selftest.cpp
#include "GlitchEngine.h"
#include "LoFi.h"
#include <cassert>
#include <cstdio>
#include <set>

static void fill (std::vector<float>& L, std::vector<float>& R, int offset)
{
    for (size_t i = 0; i < L.size(); ++i)
    {
        const float t = (float) (i + (size_t) offset) / 48000.0f;
        L[i] = 0.8f * std::sin (2 * ac::kPi * 110 * t);
        R[i] = 0.8f * std::sin (2 * ac::kPi * 220 * t);
    }
}

static void checkSane (const std::vector<float>& v, const char* what)
{
    for (float x : v)
        if (! std::isfinite (x) || std::fabs (x) > 8.0f) { std::printf ("FAIL: %s produced %f\n", what, x); std::exit (1); }
}

int main()
{
    const double sr = 48000; const int block = 512, blocks = 400;
    std::vector<float> L (block), R (block);

    // 1) Glitch with every step type, chaos and odd tempo: must stay finite and walk all 16 steps.
    {
        ac::GlitchEngine g; g.prepare (sr);
        ac::GlitchSettings s;
        for (int k = 0; k < 16; ++k) s.steps[k] = k % ac::kNumStepTypes;
        s.chaos = 0.3f; s.sliceDiv = 6; s.stepBeats = 0.125;
        std::set<int> seen;
        double ppq = 0, bpm = 93.0;
        for (int b = 0; b < blocks; ++b)
        {
            fill (L, R, b * block);
            g.process (L.data(), R.data(), block, ppq, bpm, s);
            checkSane (L, "glitch"); checkSane (R, "glitch");
            seen.insert (g.currentStep());
            ppq += block * bpm / (60.0 * sr);
            if (b == 200) ppq = 0; // simulate a loop jump
        }
        assert (seen.size() == 16);
    }

    // 2) Glitch with an all-off pattern must be bit-transparent.
    {
        ac::GlitchEngine g; g.prepare (sr);
        ac::GlitchSettings s; // all kOff
        std::vector<float> L2 (block), R2 (block);
        fill (L, R, 0); L2 = L; R2 = R;
        g.process (L.data(), R.data(), block, 0, 120, s);
        assert (L == L2 && R == R2);
    }

    // 3) Lo-fi modules at extreme settings.
    ac::Crusher cr; ac::Drive dr; dr.prepare (sr); ac::Wobble wb; wb.prepare (sr);
    ac::Magnetic mg; mg.prepare (sr); ac::Noise nz; nz.prepare (sr); ac::Isolator iso; iso.prepare (sr);
    ac::DualFilter fl; fl.prepare (sr);
    for (int b = 0; b < blocks; ++b)
    {
        fill (L, R, b * block);
        iso.process (L.data(), R.data(), block, 6, -40, 6);
        fl.process (L.data(), R.data(), block, 2000, 2500, 1.0f);
        dr.process (L.data(), R.data(), block, 1.0f, 1000);
        cr.process (L.data(), R.data(), block, 1.0f, 40.0f);
        wb.process (L.data(), R.data(), block, 1.0f, 1.0f);
        mg.process (L.data(), R.data(), block, 1.0f, 1.0f);
        nz.process (L.data(), R.data(), block, 1.0f, 1.0f);
        checkSane (L, "lofi chain"); checkSane (R, "lofi chain");
    }

    // 4) Isolator at 0 dB is (numerically) transparent.
    {
        ac::Isolator i2; i2.prepare (sr);
        fill (L, R, 0); auto ref = L;
        i2.process (L.data(), R.data(), block, 0, 0, 0);
        for (int k = 0; k < block; ++k) assert (std::fabs (L[(size_t) k] - ref[(size_t) k]) < 1e-5f);
    }

    std::puts ("ALL DSP SELF-TESTS PASSED");
    return 0;
}
