#pragma once
#include <JuceHeader.h>
#include "DSP/GlitchEngine.h"
#include "DSP/LoFi.h"

class AeroCrushProcessor : public juce::AudioProcessor
{
public:
    AeroCrushProcessor();

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "AeroCrush"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 3.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    void loadPreset (int index);
    void setParam (const juce::String& id, float plainValue);

    static const juce::StringArray& stepTypeNames();
    static const juce::StringArray& rateNames();

    juce::AudioProcessorValueTreeState apvts;
    std::atomic<int> playStep { -1 }; // for the UI playhead

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    float v (const char* id) const { return apvts.getRawParameterValue (id)->load(); }

    std::atomic<float>* stepParams[16] {};
    int currentProgram = 0;
    double freePpq = 0.0; // free-running clock when the host is stopped

    ac::GlitchEngine glitch;
    ac::Isolator isolator;
    ac::DualFilter filter;
    ac::Drive drive;
    ac::Crusher crusher;
    ac::Wobble wobble;
    ac::Magnetic magnetic;
    ac::Noise noise;
    juce::Reverb reverb;
    juce::AudioBuffer<float> dry;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeroCrushProcessor)
};
