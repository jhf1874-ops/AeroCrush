#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "Presets.h"

namespace
{
using Float = juce::AudioParameterFloat;

std::unique_ptr<Float> fparam (const char* id, const char* name, juce::NormalisableRange<float> r,
                               float def, juce::String suffix = {}, int decimals = 1)
{
    return std::make_unique<Float> (juce::ParameterID { id, 1 }, name, r, def,
        juce::AudioParameterFloatAttributes().withLabel (suffix).withStringFromValueFunction (
            [decimals, suffix] (float x, int) { return juce::String (x, decimals) + (suffix.isEmpty() ? "" : " " + suffix); }));
}

std::unique_ptr<Float> pct (const char* id, const char* name, float def)
{
    return std::make_unique<Float> (juce::ParameterID { id, 1 }, name, juce::NormalisableRange<float> (0.f, 1.f), def,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction (
            [] (float x, int) { return juce::String (juce::roundToInt (x * 100.f)) + "%"; }));
}

std::unique_ptr<juce::AudioParameterBool> on (const char* id, const char* name, bool def = false)
{
    return std::make_unique<juce::AudioParameterBool> (juce::ParameterID { id, 1 }, name, def);
}

constexpr double kRateBeats[] = { 1.0, 0.5, 0.25, 0.125 };
} // namespace

const juce::StringArray& AeroCrushProcessor::stepTypeNames()
{
    static const juce::StringArray n { "Off", "Stutter", "Reverse", "Tape Stop", "Half Speed", "Gate", "Scatter", "Mute" };
    return n;
}

const juce::StringArray& AeroCrushProcessor::rateNames()
{
    static const juce::StringArray n { "1/4", "1/8", "1/16", "1/32" };
    return n;
}

juce::AudioProcessorValueTreeState::ParameterLayout AeroCrushProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout l;
    using R = juce::NormalisableRange<float>;

    // Glitch sequencer
    l.add (on ("glitch_on", "Glitch On", true),
           std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "glitch_rate", 1 }, "Glitch Rate", rateNames(), 2),
           pct ("glitch_prob", "Glitch Probability", 1.0f),
           pct ("glitch_chaos", "Glitch Chaos", 0.0f),
           std::make_unique<juce::AudioParameterInt> (juce::ParameterID { "glitch_slice", 1 }, "Glitch Slices", 1, 16, 4),
           pct ("glitch_mix", "Glitch Mix", 1.0f));
    for (int i = 0; i < 16; ++i)
        l.add (std::make_unique<juce::AudioParameterChoice> (juce::ParameterID { "step" + juce::String (i), 1 },
                                                             "Step " + juce::String (i + 1), stepTypeNames(), 0));

    l.add (on ("crush_on", "Digital On"),
           fparam ("crush_bits", "Bits", R (1.f, 16.f, 0.1f), 12.f, "bit"),
           fparam ("crush_rate", "Downsample", R (1.f, 40.f, 0.1f, 0.5f), 4.f, "x"));

    l.add (on ("drive_on", "Drive On"),
           pct ("drive_amt", "Drive", 0.3f),
           fparam ("drive_tone", "Drive Tone", R (1000.f, 20000.f, 1.f, 0.3f), 12000.f, "Hz", 0));

    l.add (on ("wobble_on", "Wobble On"),
           pct ("wobble_wow", "Wow", 0.3f),
           pct ("wobble_flutter", "Flutter", 0.2f));

    l.add (on ("mag_on", "Magnetic On"),
           pct ("mag_drop", "Dropouts", 0.3f),
           pct ("mag_depth", "Drop Depth", 0.5f));

    l.add (on ("noise_on", "Noise On"),
           pct ("noise_hiss", "Hiss", 0.2f),
           pct ("noise_crackle", "Crackle", 0.3f));

    l.add (on ("filter_on", "Filter On"),
           fparam ("filter_lo", "Low Cut", R (20.f, 2000.f, 1.f, 0.35f), 20.f, "Hz", 0),
           fparam ("filter_hi", "High Cut", R (500.f, 20000.f, 1.f, 0.3f), 20000.f, "Hz", 0),
           pct ("filter_reso", "Resonance", 0.0f));

    l.add (on ("iso_on", "Isolator On"),
           fparam ("iso_low", "Iso Low", R (-40.f, 6.f, 0.1f), 0.f, "dB"),
           fparam ("iso_mid", "Iso Mid", R (-40.f, 6.f, 0.1f), 0.f, "dB"),
           fparam ("iso_high", "Iso High", R (-40.f, 6.f, 0.1f), 0.f, "dB"));

    l.add (on ("space_on", "Space On"),
           pct ("space_size", "Space Size", 0.5f),
           pct ("space_mix", "Space Mix", 0.25f));

    l.add (fparam ("out_width", "Width", R (0.f, 2.f, 0.01f), 1.f, "", 2),
           pct ("out_mix", "Dry/Wet", 1.0f),
           fparam ("out_gain", "Output", R (-24.f, 12.f, 0.1f), 0.f, "dB"));
    return l;
}

AeroCrushProcessor::AeroCrushProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (int i = 0; i < 16; ++i)
        stepParams[i] = apvts.getRawParameterValue ("step" + juce::String (i));
}

bool AeroCrushProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    // shortcut: stereo only (FL Studio always runs effects in stereo); add mono if another host needs it.
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void AeroCrushProcessor::prepareToPlay (double sr, int samplesPerBlock)
{
    glitch.prepare (sr);
    isolator.prepare (sr);
    filter.prepare (sr);
    drive.prepare (sr);
    wobble.prepare (sr);
    magnetic.prepare (sr);
    noise.prepare (sr);
    reverb.setSampleRate (sr);
    reverb.reset();
    dry.setSize (2, samplesPerBlock);
}

void AeroCrushProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (n == 0 || buffer.getNumChannels() < 2) return;
    const double sr = getSampleRate();

    dry.makeCopyOf (buffer, true);
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    // Transport: follow the host when playing, otherwise free-run at host tempo.
    double bpm = 120.0, ppq = freePpq;
    bool playing = false;
    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
        {
            if (auto b = pos->getBpm()) bpm = *b;
            playing = pos->getIsPlaying();
            if (playing)
                if (auto p = pos->getPpqPosition()) ppq = *p;
        }
    freePpq = ppq + n * bpm / (60.0 * sr);

    // 1. Glitch sequencer (always records history so turning it on is instant)
    const bool glitchOn = v ("glitch_on") > 0.5f;
    ac::GlitchSettings gs;
    for (int i = 0; i < 16; ++i) gs.steps[i] = (int) stepParams[i]->load();
    gs.stepBeats = kRateBeats[juce::jlimit (0, 3, (int) v ("glitch_rate"))];
    gs.prob = v ("glitch_prob");
    gs.chaos = v ("glitch_chaos");
    gs.sliceDiv = (int) v ("glitch_slice");
    gs.mix = glitchOn ? v ("glitch_mix") : 0.0f;
    glitch.process (L, R, n, ppq, bpm, gs);
    playStep = glitchOn ? glitch.currentStep() : -1;

    // 2. Tone / lo-fi chain
    if (v ("iso_on") > 0.5f)    isolator.process (L, R, n, v ("iso_low"), v ("iso_mid"), v ("iso_high"));
    if (v ("filter_on") > 0.5f) filter.process (L, R, n, v ("filter_lo"), v ("filter_hi"), v ("filter_reso"));
    if (v ("drive_on") > 0.5f)  drive.process (L, R, n, v ("drive_amt"), v ("drive_tone"));
    if (v ("crush_on") > 0.5f)  crusher.process (L, R, n, v ("crush_bits"), v ("crush_rate"));
    if (v ("wobble_on") > 0.5f) wobble.process (L, R, n, v ("wobble_wow"), v ("wobble_flutter"));
    if (v ("mag_on") > 0.5f)    magnetic.process (L, R, n, v ("mag_drop"), v ("mag_depth"));
    if (v ("noise_on") > 0.5f)  noise.process (L, R, n, v ("noise_hiss"), v ("noise_crackle"));

    if (v ("space_on") > 0.5f)
    {
        juce::Reverb::Parameters p;
        p.roomSize = v ("space_size");
        p.damping = 0.45f;
        p.wetLevel = v ("space_mix");
        p.dryLevel = 1.0f - v ("space_mix");
        p.width = 1.0f;
        reverb.setParameters (p);
        reverb.processStereo (L, R, n);
    }

    // 3. Output: width, dry/wet against the untouched input, gain
    const float width = v ("out_width"), mix = v ("out_mix");
    const float gain = juce::Decibels::decibelsToGain (v ("out_gain"));
    const float* dL = dry.getReadPointer (0);
    const float* dR = dry.getReadPointer (1);
    for (int i = 0; i < n; ++i)
    {
        const float m = 0.5f * (L[i] + R[i]), s = 0.5f * (L[i] - R[i]) * width;
        L[i] = (dL[i] + mix * ((m + s) - dL[i])) * gain;
        R[i] = (dR[i] + mix * ((m - s) - dR[i])) * gain;
    }
}

int AeroCrushProcessor::getNumPrograms() { return (int) factoryPresets().size(); }

const juce::String AeroCrushProcessor::getProgramName (int index)
{
    const auto& p = factoryPresets();
    return juce::isPositiveAndBelow (index, (int) p.size()) ? p[(size_t) index].name : juce::String();
}

void AeroCrushProcessor::setCurrentProgram (int index)
{
    // Guard: some hosts re-send the current program while restoring a project.
    if (index != currentProgram) loadPreset (index);
}

void AeroCrushProcessor::setParam (const juce::String& id, float plainValue)
{
    if (auto* p = apvts.getParameter (id))
        p->setValueNotifyingHost (p->convertTo0to1 (plainValue));
    else
        jassertfalse; // typo in a preset id
}

void AeroCrushProcessor::loadPreset (int index)
{
    const auto& list = factoryPresets();
    if (! juce::isPositiveAndBelow (index, (int) list.size())) return;
    currentProgram = index;
    const auto& pr = list[(size_t) index];

    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            rp->setValueNotifyingHost (rp->getDefaultValue());

    const juce::String codes (".SRTHGXM");
    for (int i = 0; i < 16; ++i)
    {
        const auto c = i < pr.pattern.length() ? pr.pattern[i] : '.';
        setParam ("step" + juce::String (i), (float) juce::jmax (0, codes.indexOfChar (c)));
    }
    for (const auto& [id, val] : pr.values)
        setParam (id, val);
}

void AeroCrushProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void AeroCrushProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            currentProgram = tree.getProperty ("program", 0);
            apvts.replaceState (tree);
        }
}

juce::AudioProcessorEditor* AeroCrushProcessor::createEditor() { return new AeroCrushEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new AeroCrushProcessor(); }
