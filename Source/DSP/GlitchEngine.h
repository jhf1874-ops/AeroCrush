#pragma once
// SP-404-style sequenced glitch engine. Pure C++ (no JUCE).
// Keeps a rolling history buffer; each sequencer step can replace the live
// signal with a manipulated read from that history.
#include <vector>
#include <cmath>
#include <climits>
#include <algorithm>
#include "Biquad.h"

namespace ac
{
enum StepType { kOff = 0, kStutter, kReverse, kTapeStop, kHalf, kGate, kScatter, kMute, kNumStepTypes };

struct GlitchSettings
{
    int    steps[16] {};
    double stepBeats = 0.25; // 1/16 note
    float  prob = 1.0f;      // chance a programmed step fires
    float  chaos = 0.0f;     // chance any step becomes a random effect
    float  mix = 1.0f;
    int    sliceDiv = 4;     // repeats per step (stutter) / gate chops
};

class GlitchEngine
{
public:
    void prepare (double sr)
    {
        sampleRate = sr;
        size = (int) (sr * 6.0); // enough history for a 1/4 step at 20 BPM
        for (auto& b : buf) b.assign ((size_t) size, 0.0f);
        writePos = 0;
        lastStepAbs = LLONG_MIN;
        active = kOff;
    }

    int currentStep() const { return curStep; }
    int activeType() const  { return active; }

    // ppqStart: host position (quarter notes) of the block's first sample.
    void process (float* L, float* R, int n, double ppqStart, double bpm, const GlitchSettings& s)
    {
        bpm = std::clamp (bpm, 20.0, 400.0);
        const double spb     = sampleRate * 60.0 / bpm;          // samples per beat
        const double stepLen = std::max (64.0, s.stepBeats * spb);
        const double fade    = std::min (stepLen * 0.2, sampleRate * 0.003);
        const double slice   = std::max (32.0, stepLen / std::max (1, s.sliceDiv));
        const double sFade   = std::min (slice * 0.15, sampleRate * 0.0015);

        for (int i = 0; i < n; ++i)
        {
            const double ppq = ppqStart + i / spb;
            const auto absStep = (long long) std::floor (ppq / s.stepBeats);
            const double t = (ppq - (double) absStep * s.stepBeats) * spb; // samples into step

            buf[0][(size_t) writePos] = L[i];
            buf[1][(size_t) writePos] = R[i];

            if (absStep != lastStepAbs)
                startStep (absStep, s, t, stepLen);

            const float dl = L[i], dr = R[i];
            float wl = dl, wr = dr;

            switch (active)
            {
                case kStutter: {
                    const double ph = std::fmod (t, slice);
                    const float w = edge (ph, slice, sFade);
                    wl = read (0, capture - slice + ph) * w;
                    wr = read (1, capture - slice + ph) * w;
                } break;
                case kReverse:
                    wl = read (0, capture - 1.0 - t);
                    wr = read (1, capture - 1.0 - t);
                    break;
                case kTapeStop: {
                    const double k = 1.0 - std::min (1.0, t / stepLen);
                    tapePhase += k * k;
                    wl = read (0, tapeBase + tapePhase);
                    wr = read (1, tapeBase + tapePhase);
                } break;
                case kHalf:
                    wl = read (0, capture + 0.5 * t);
                    wr = read (1, capture + 0.5 * t);
                    break;
                case kGate: {
                    const double seg = stepLen / (2.0 * std::max (1, s.sliceDiv));
                    const bool open = ((long long) (t / seg)) % 2 == 0;
                    const float w = open ? edge (std::fmod (t, seg), seg, sFade) : 0.0f;
                    wl = dl * w; wr = dr * w;
                } break;
                case kScatter: {
                    const double sl = stepLen * 0.5, ph = std::fmod (t, sl);
                    const float w = edge (ph, sl, sFade);
                    wl = read (0, scatterBase + ph) * w;
                    wr = read (1, scatterBase + ph) * w;
                } break;
                case kMute: wl = wr = 0.0f; break;
                default: break;
            }

            if (active != kOff)
            {
                // fade in/out at step edges so step changes never click
                const float g = s.mix * edge (t, stepLen, fade);
                L[i] = dl + g * (wl - dl);
                R[i] = dr + g * (wr - dr);
            }

            writePos = (writePos + 1) % size;
        }
    }

private:
    void startStep (long long absStep, const GlitchSettings& s, double tEntry, double stepLen)
    {
        lastStepAbs = absStep;
        curStep = (int) (((absStep % 16) + 16) % 16);
        int type = std::clamp (s.steps[curStep], 0, kNumStepTypes - 1);
        if (rng.next01() < s.chaos)
            type = 1 + std::min (kNumStepTypes - 2, (int) (rng.next01() * (kNumStepTypes - 1)));
        if (type != kOff && rng.next01() >= s.prob)
            type = kOff;
        active = type;
        capture = (double) writePos - tEntry; // where this step "began" in the history
        tapeBase = writePos;
        tapePhase = 0.0;
        scatterBase = capture - (1 + (int) (rng.next01() * 6)) * stepLen * 0.5;
    }

    static float edge (double pos, double len, double f)
    {
        if (f <= 0) return 1.0f;
        return (float) std::clamp (std::min (pos, len - pos) / f, 0.0, 1.0);
    }

    float read (int ch, double idx) const
    {
        const double fl = std::floor (idx);
        const float frac = (float) (idx - fl);
        long long i0 = ((long long) fl % size + size) % size;
        const long long i1 = (i0 + 1) % size;
        const auto& b = buf[ch];
        return b[(size_t) i0] + frac * (b[(size_t) i1] - b[(size_t) i0]);
    }

    double sampleRate = 44100.0;
    std::vector<float> buf[2];
    int size = 1, writePos = 0, curStep = 0, active = kOff;
    long long lastStepAbs = LLONG_MIN;
    double capture = 0, tapeBase = 0, tapePhase = 0, scatterBase = 0;
    Rng rng;
};
} // namespace ac
