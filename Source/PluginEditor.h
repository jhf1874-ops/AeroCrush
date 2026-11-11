#pragma once
#include "UI/Components.h"

class AeroCrushEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit AeroCrushEditor (AeroCrushProcessor&);
    ~AeroCrushEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void renderBackground();
    void stepPreset (int delta);

    AeroLookAndFeel lnf; // declared first so it outlives every child
    AeroCrushProcessor& proc;
    juce::Image background;

    std::unique_ptr<Module> glitchModule;
    StepSequencer seq;
    juce::ComboBox rateBox;
    std::unique_ptr<APVTS::ComboBoxAttachment> rateAttachment;
    juce::TextButton clearButton { "Clear" }, randomButton { "Randomize" }, fillButton { "Fill" };

    std::vector<std::unique_ptr<Module>> row2, row3;

    juce::ComboBox presetBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    juce::TooltipWindow tooltips { this, 600 };
    juce::Random rng;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AeroCrushEditor)
};
