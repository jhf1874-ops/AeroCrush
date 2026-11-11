#pragma once
#include <JuceHeader.h>

// Factory presets. Pattern chars per step:
//  . off   S stutter   R reverse   T tape stop   H half-speed   G gate   X scatter   M mute
// Any parameter not listed is reset to its default.
struct FactoryPreset
{
    juce::String name;
    juce::String pattern;
    std::vector<std::pair<juce::String, float>> values;
};

inline const std::vector<FactoryPreset>& factoryPresets()
{
    static const std::vector<FactoryPreset> list {
        { "Init", "................", {} },

        { "SP Lo-Fi Dust", "................",
          { { "crush_on", 1.f }, { "crush_bits", 10.f }, { "crush_rate", 3.f },
            { "filter_on", 1.f }, { "filter_hi", 9000.f },
            { "noise_on", 1.f }, { "noise_hiss", 0.2f }, { "noise_crackle", 0.35f },
            { "drive_on", 1.f }, { "drive_amt", 0.2f } } },

        { "Stutter Bounce", "S...S...S.S.S...",
          { { "glitch_slice", 4.f }, { "glitch_rate", 2.f },
            { "crush_on", 1.f }, { "crush_bits", 12.f }, { "crush_rate", 2.f } } },

        { "Tape Stop Endings", ".......T.......T",
          { { "glitch_rate", 1.f }, { "wobble_on", 1.f }, { "wobble_wow", 0.2f }, { "wobble_flutter", 0.15f } } },

        { "Reverse Dreams", "R...R.R...R...RR",
          { { "glitch_rate", 1.f }, { "space_on", 1.f }, { "space_size", 0.8f }, { "space_mix", 0.35f },
            { "out_width", 1.4f } } },

        { "Abstract Chaos", "S.RX.G.TS..HX.M.",
          { { "glitch_chaos", 0.5f }, { "glitch_prob", 0.7f }, { "glitch_slice", 6.f }, { "glitch_rate", 3.f },
            { "crush_on", 1.f }, { "crush_bits", 8.f }, { "crush_rate", 6.f } } },

        { "Cassette Warble", "................",
          { { "wobble_on", 1.f }, { "wobble_wow", 0.6f }, { "wobble_flutter", 0.4f },
            { "mag_on", 1.f }, { "mag_drop", 0.3f }, { "mag_depth", 0.6f },
            { "noise_on", 1.f }, { "noise_hiss", 0.35f }, { "noise_crackle", 0.0f },
            { "filter_on", 1.f }, { "filter_hi", 7000.f }, { "filter_lo", 60.f } } },

        { "Broken Sampler", "X..X.X..X..XX.X.",
          { { "glitch_rate", 2.f }, { "crush_on", 1.f }, { "crush_bits", 6.f }, { "crush_rate", 8.f },
            { "drive_on", 1.f }, { "drive_amt", 0.45f }, { "drive_tone", 6000.f } } },

        { "Underwater Isolator", "........H.......",
          { { "iso_on", 1.f }, { "iso_high", -40.f }, { "iso_mid", -8.f }, { "iso_low", 3.f },
            { "space_on", 1.f }, { "space_size", 0.7f }, { "space_mix", 0.3f },
            { "wobble_on", 1.f }, { "wobble_wow", 0.35f } } },

        { "Half-Time Haze", "H.......H.......",
          { { "glitch_rate", 0.f }, { "glitch_mix", 0.85f },
            { "space_on", 1.f }, { "space_size", 0.85f }, { "space_mix", 0.3f },
            { "filter_on", 1.f }, { "filter_hi", 8000.f } } },

        { "Gate Chop", "G.G.GGG.G.G.GG.G",
          { { "glitch_slice", 2.f }, { "glitch_rate", 2.f } } },

        { "Aero Bubbles", "...R.......S....",
          { { "space_on", 1.f }, { "space_size", 0.92f }, { "space_mix", 0.42f },
            { "wobble_on", 1.f }, { "wobble_wow", 0.15f }, { "wobble_flutter", 0.1f },
            { "out_width", 1.6f }, { "filter_on", 1.f }, { "filter_lo", 120.f } } },

        { "Radio Ghost", "...........M..M.",
          { { "filter_on", 1.f }, { "filter_lo", 600.f }, { "filter_hi", 3500.f }, { "filter_reso", 0.5f },
            { "drive_on", 1.f }, { "drive_amt", 0.5f },
            { "noise_on", 1.f }, { "noise_hiss", 0.45f }, { "noise_crackle", 0.2f },
            { "mag_on", 1.f }, { "mag_drop", 0.4f }, { "mag_depth", 0.8f }, { "out_width", 0.3f } } },

        { "Drunk Machine", "S.......S..S....",
          { { "glitch_chaos", 0.2f }, { "glitch_slice", 3.f },
            { "wobble_on", 1.f }, { "wobble_wow", 0.8f }, { "wobble_flutter", 0.2f },
            { "crush_on", 1.f }, { "crush_bits", 11.f }, { "crush_rate", 2.f } } },

        { "Floppy Disk Kit", "....T.......X...",
          { { "crush_on", 1.f }, { "crush_bits", 5.f }, { "crush_rate", 12.f },
            { "iso_on", 1.f }, { "iso_low", 4.f },
            { "drive_on", 1.f }, { "drive_amt", 0.3f } } },

        { "Skyline Shimmer", "................",
          { { "space_on", 1.f }, { "space_size", 0.98f }, { "space_mix", 0.55f },
            { "filter_on", 1.f }, { "filter_lo", 250.f },
            { "noise_on", 1.f }, { "noise_hiss", 0.1f }, { "noise_crackle", 0.15f }, { "out_width", 1.8f } } },
    };
    return list;
}
