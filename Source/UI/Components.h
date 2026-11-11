#pragma once
#include "AeroLookAndFeel.h"
#include "../PluginProcessor.h"

using APVTS = juce::AudioProcessorValueTreeState;

// Rotary knob + caption, bound to one parameter.
struct AeroKnob : juce::Component
{
    AeroKnob (APVTS& s, const juce::String& id, const juce::String& name)
        : attachment (s, id, slider)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 72, 18);
        slider.setPopupDisplayEnabled (false, false, nullptr);
        slider.setTooltip (name);
        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (slider);
        addAndMakeVisible (label);
    }

    void resized() override
    {
        auto r = getLocalBounds();
        label.setBounds (r.removeFromTop (16));
        slider.setBounds (r);
    }

    juce::Slider slider;
    juce::Label label;
    APVTS::SliderAttachment attachment;
};

// Frosted-glass effect module: power pill + a row of knobs.
struct Module : juce::Component
{
    Module (APVTS& s, const juce::String& title, const juce::String& onId,
            std::vector<std::pair<juce::String, juce::String>> knobDefs)
        : name (title)
    {
        if (onId.isNotEmpty())
        {
            power.setButtonText (title);
            addAndMakeVisible (power);
            powerAttachment = std::make_unique<APVTS::ButtonAttachment> (s, onId, power);
        }
        for (auto& [id, label] : knobDefs)
            addAndMakeVisible (knobs.add (new AeroKnob (s, id, label)));
    }

    void paint (juce::Graphics& g) override
    {
        auto r = getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.10f));
        g.fillRoundedRectangle (r.translated (0, 3), 16);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.55f), 0, r.getY(),
                                                 juce::Colours::white.withAlpha (0.16f), 0, r.getBottom(), false));
        g.fillRoundedRectangle (r, 16);
        auto shine = r.reduced (6, 2).withHeight (r.getHeight() * 0.3f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.45f), 0, shine.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), 0, shine.getBottom(), false));
        g.fillRoundedRectangle (shine, 12);
        g.setColour (juce::Colours::white.withAlpha (0.85f));
        g.drawRoundedRectangle (r, 16, 1.3f);

        if (! power.isVisible())
        {
            g.setColour (aero::navy);
            g.setFont (aero::font (13.0f));
            g.drawText (name, getLocalBounds().reduced (16, 8).removeFromTop (24), juce::Justification::centredLeft);
        }
    }

    void resized() override
    {
        auto r = getLocalBounds().reduced (10, 8);
        power.setBounds (r.removeFromTop (26).withWidth (juce::jmin (r.getWidth(), 210)));
        r.removeFromTop (6);
        r.removeFromLeft ((int) (r.getWidth() * knobsFromX));
        if (knobs.isEmpty()) return;
        const int w = r.getWidth() / knobs.size();
        for (auto* k : knobs) k->setBounds (r.removeFromLeft (w).reduced (2, 0));
    }

    juce::String name;
    float knobsFromX = 0.0f; // fraction of width left free for other controls
    juce::ToggleButton power;
    std::unique_ptr<APVTS::ButtonAttachment> powerAttachment;
    juce::OwnedArray<AeroKnob> knobs;
};

// 16 glossy step pads. Left-click = next effect, right-click = previous.
class StepSequencer : public juce::Component
{
public:
    explicit StepSequencer (AeroCrushProcessor& p) : proc (p) {}

    static juce::Colour typeColour (int t)
    {
        static const juce::Colour c[] = { juce::Colours::white, juce::Colour (0xff29d3ff), juce::Colour (0xffa07bff),
                                          juce::Colour (0xffffa64d), juce::Colour (0xff8be04e), juce::Colour (0xffff6fb5),
                                          juce::Colour (0xffffd84d), juce::Colour (0xff6b7c93) };
        return c[juce::jlimit (0, 7, t)];
    }

    static juce::String abbrev (int t)
    {
        static const char* a[] = { "", "STUT", "REV", "STOP", "HALF", "GATE", "SCAT", "MUTE" };
        return a[juce::jlimit (0, 7, t)];
    }

    int stepType (int i) const
    {
        return (int) proc.apvts.getRawParameterValue ("step" + juce::String (i))->load();
    }

    void setStep (int i, int type)
    {
        if (auto* p = proc.apvts.getParameter ("step" + juce::String (i)))
        {
            p->beginChangeGesture();
            p->setValueNotifyingHost (p->convertTo0to1 ((float) type));
            p->endChangeGesture();
        }
    }

    void paint (juce::Graphics& g) override
    {
        const int play = proc.playStep.load();
        for (int i = 0; i < 16; ++i)
        {
            auto r = cell (i);
            const int t = stepType (i);
            const auto col = typeColour (t);
            if (i == play)
            {
                g.setColour (juce::Colour (0xffbff6ff).withAlpha (0.55f));
                g.fillRoundedRectangle (r.expanded (4), 10);
            }
            if (t == 0)
                aero::glossyRect (g, r, 8, juce::Colours::white.withAlpha (0.5f), juce::Colour (0xff9fd4ee).withAlpha (0.45f));
            else
                aero::glossyRect (g, r, 8, col.brighter (0.6f), col.darker (0.3f));

            g.setColour (i == play ? juce::Colours::white : juce::Colours::white.withAlpha (0.75f));
            g.drawRoundedRectangle (r, 8, i == play ? 2.4f : 1.1f);

            g.setFont (aero::font (t == 0 ? 12.0f : 10.5f));
            g.setColour (t == 0 ? aero::navy.withAlpha (0.45f) : juce::Colours::white);
            g.drawText (t == 0 ? juce::String (i + 1) : abbrev (t), r, juce::Justification::centred);
        }
    }

    void mouseDown (const juce::MouseEvent& e) override
    {
        for (int i = 0; i < 16; ++i)
            if (cell (i).contains (e.position))
            {
                const int step = e.mods.isPopupMenu() ? ac::kNumStepTypes - 1 : 1;
                setStep (i, (stepType (i) + step) % ac::kNumStepTypes);
                repaint();
                return;
            }
    }

private:
    juce::Rectangle<float> cell (int i) const
    {
        const float gap = 5.0f, groupGap = 9.0f;
        const float w = ((float) getWidth() - 12 * gap - 3 * groupGap) / 16.0f;
        const float x = (float) i * (w + gap) + (float) (i / 4) * (groupGap - gap);
        return { x, 4.0f, w, (float) getHeight() - 8.0f };
    }

    AeroCrushProcessor& proc;
};
