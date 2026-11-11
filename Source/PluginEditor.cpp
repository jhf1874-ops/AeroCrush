#include "PluginEditor.h"
#include "Presets.h"

namespace { constexpr int kW = 1060, kH = 730; }

AeroCrushEditor::AeroCrushEditor (AeroCrushProcessor& p)
    : AudioProcessorEditor (&p), proc (p), seq (p)
{
    setLookAndFeel (&lnf);
    auto& s = proc.apvts;

    // --- Glitch sequencer panel ---
    glitchModule = std::make_unique<Module> (s, "GLITCH SEQUENCER", "glitch_on",
        std::vector<std::pair<juce::String, juce::String>> { { "glitch_prob", "Prob" }, { "glitch_chaos", "Chaos" },
                                                             { "glitch_slice", "Slices" }, { "glitch_mix", "Mix" } });
    glitchModule->knobsFromX = 0.63f;
    addAndMakeVisible (*glitchModule);
    addAndMakeVisible (seq);

    rateBox.addItemList (AeroCrushProcessor::rateNames(), 1);
    rateBox.setTooltip ("Step length");
    addAndMakeVisible (rateBox);
    rateAttachment = std::make_unique<APVTS::ComboBoxAttachment> (s, "glitch_rate", rateBox);

    clearButton.onClick = [this] { for (int i = 0; i < 16; ++i) seq.setStep (i, 0); };
    randomButton.onClick = [this]
    {
        for (int i = 0; i < 16; ++i)
            seq.setStep (i, rng.nextFloat() < 0.45f ? 1 + rng.nextInt (ac::kNumStepTypes - 1) : 0);
    };
    fillButton.onClick = [this] { for (int i = 12; i < 16; ++i) seq.setStep (i, ac::kStutter); };
    clearButton.setTooltip ("Clear all steps");
    randomButton.setTooltip ("Random glitch pattern");
    fillButton.setTooltip ("Stutter roll on the last beat");
    for (auto* b : { &clearButton, &randomButton, &fillButton }) addAndMakeVisible (b);

    // --- Effect modules ---
    using K = std::vector<std::pair<juce::String, juce::String>>;
    row2.push_back (std::make_unique<Module> (s, "DIGITAL", "crush_on", K { { "crush_bits", "Bits" }, { "crush_rate", "Rate" } }));
    row2.push_back (std::make_unique<Module> (s, "DRIVE", "drive_on", K { { "drive_amt", "Drive" }, { "drive_tone", "Tone" } }));
    row2.push_back (std::make_unique<Module> (s, "WOBBLE", "wobble_on", K { { "wobble_wow", "Wow" }, { "wobble_flutter", "Flutter" } }));
    row2.push_back (std::make_unique<Module> (s, "MAGNETIC", "mag_on", K { { "mag_drop", "Dropout" }, { "mag_depth", "Depth" } }));
    row2.push_back (std::make_unique<Module> (s, "NOISE", "noise_on", K { { "noise_hiss", "Hiss" }, { "noise_crackle", "Crackle" } }));

    row3.push_back (std::make_unique<Module> (s, "FILTER", "filter_on", K { { "filter_lo", "Low Cut" }, { "filter_hi", "High Cut" }, { "filter_reso", "Reso" } }));
    row3.push_back (std::make_unique<Module> (s, "ISOLATOR", "iso_on", K { { "iso_low", "Low" }, { "iso_mid", "Mid" }, { "iso_high", "High" } }));
    row3.push_back (std::make_unique<Module> (s, "SPACE", "space_on", K { { "space_size", "Size" }, { "space_mix", "Mix" } }));
    row3.push_back (std::make_unique<Module> (s, "OUTPUT", "", K { { "out_width", "Width" }, { "out_mix", "Dry/Wet" }, { "out_gain", "Gain" } }));
    for (auto* row : { &row2, &row3 })
        for (auto& m : *row) addAndMakeVisible (*m);

    // --- Presets ---
    const auto& presets = factoryPresets();
    for (int i = 0; i < (int) presets.size(); ++i)
        presetBox.addItem (presets[(size_t) i].name, i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        if (const int id = presetBox.getSelectedId(); id > 0) proc.loadPreset (id - 1);
    };
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    for (auto* c : std::initializer_list<juce::Component*> { &presetBox, &prevButton, &nextButton }) addAndMakeVisible (c);

    // shortcut: fixed window size; add a scaled content component if resizing is wanted.
    setSize (kW, kH);
    startTimerHz (30);
}

AeroCrushEditor::~AeroCrushEditor()
{
    setLookAndFeel (nullptr);
}

void AeroCrushEditor::stepPreset (int delta)
{
    const int n = presetBox.getNumItems();
    const int idx = (juce::jmax (0, presetBox.getSelectedId() - 1) + delta + n) % n;
    presetBox.setSelectedId (idx + 1, juce::sendNotificationSync);
}

void AeroCrushEditor::timerCallback()
{
    seq.repaint();
    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
}

void AeroCrushEditor::renderBackground()
{
    const int w = getWidth(), h = getHeight();
    background = juce::Image (juce::Image::ARGB, w, h, true);
    juce::Graphics g (background);

    // sky
    juce::ColourGradient sky (aero::skyTop, 0, 0, aero::skyLow, 0, (float) h * 0.72f, false);
    sky.addColour (0.45, aero::skyMid);
    g.setGradientFill (sky);
    g.fillAll();

    // sun glow + light rays
    const juce::Point<float> sun ((float) w * 0.86f, -20.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.85f), sun.x, sun.y,
                                             juce::Colours::white.withAlpha (0.0f), sun.x + 380, sun.y + 260, true));
    g.fillAll();
    g.setColour (juce::Colours::white.withAlpha (0.07f));
    for (int i = 0; i < 9; ++i)
    {
        const float a = juce::MathConstants<float>::pi * (0.52f + (float) i * 0.055f);
        juce::Path ray;
        ray.startNewSubPath (sun);
        ray.lineTo (sun.x + std::cos (a) * 1400, sun.y + std::sin (a) * 1400);
        ray.lineTo (sun.x + std::cos (a + 0.025f) * 1400, sun.y + std::sin (a + 0.025f) * 1400);
        ray.closeSubPath();
        g.fillPath (ray);
    }

    // rolling green hills
    juce::Path hill;
    hill.startNewSubPath (0, (float) h * 0.74f);
    hill.quadraticTo ((float) w * 0.3f, (float) h * 0.62f, (float) w * 0.62f, (float) h * 0.76f);
    hill.quadraticTo ((float) w * 0.86f, (float) h * 0.86f, (float) w, (float) h * 0.70f);
    hill.lineTo ((float) w, (float) h);
    hill.lineTo (0, (float) h);
    hill.closeSubPath();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffa8f06a), 0, (float) h * 0.64f,
                                             juce::Colour (0xff2f9e3a), 0, (float) h, false));
    g.fillPath (hill);
    g.setColour (juce::Colours::white.withAlpha (0.35f));
    g.strokePath (hill, juce::PathStrokeType (2.0f));

    // bubbles
    juce::Random r (404);
    for (int i = 0; i < 34; ++i)
    {
        const float d = 8.0f + r.nextFloat() * 46.0f;
        auto b = juce::Rectangle<float> (r.nextFloat() * (float) w, r.nextFloat() * (float) h, d, d);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.05f), b.getCentreX(), b.getCentreY(),
                                                 juce::Colours::white.withAlpha (0.35f), b.getRight(), b.getCentreY(), true));
        g.fillEllipse (b);
        g.setColour (juce::Colours::white.withAlpha (0.55f));
        g.drawEllipse (b, 1.0f);
        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.fillEllipse (b.getX() + d * 0.22f, b.getY() + d * 0.18f, d * 0.22f, d * 0.14f);
    }
}

void AeroCrushEditor::paint (juce::Graphics& g)
{
    if (background.isNull() || background.getWidth() != getWidth()) renderBackground();
    g.drawImageAt (background, 0, 0);

    // title
    auto title = juce::Rectangle<int> (24, 14, 420, 44);
    g.setFont (aero::font (38.0f));
    g.setColour (aero::navy.withAlpha (0.35f));
    g.drawText ("AeroCrush", title.translated (0, 2), juce::Justification::centredLeft);
    g.setColour (juce::Colours::white);
    g.drawText ("AeroCrush", title, juce::Justification::centredLeft);
    g.setFont (aero::font (13.0f, false));
    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.drawText ("glitch  /  lo-fi  /  abstract", title.translated (2, 34).withHeight (18), juce::Justification::centredLeft);

    // glitch panel labels
    auto gl = glitchModule->getBounds();
    g.setFont (aero::font (11.0f, false));
    g.setColour (aero::navy.withAlpha (0.75f));
    g.drawText ("Click a step for the next effect  |  right-click for previous",
                gl.getX() + 18, seq.getBottom() + 4, 400, 16, juce::Justification::centredLeft);

    // legend
    auto leg = juce::Rectangle<int> (20, getHeight() - 30, getWidth() - 40, 22);
    g.setFont (aero::font (11.5f));
    for (int t = 1; t < ac::kNumStepTypes; ++t)
    {
        auto chip = leg.removeFromLeft (118);
        aero::glossyRect (g, chip.removeFromLeft (18).reduced (2).toFloat(), 4,
                          StepSequencer::typeColour (t).brighter (0.6f), StepSequencer::typeColour (t).darker (0.3f));
        g.setColour (aero::navy);
        g.drawText (AeroCrushProcessor::stepTypeNames()[t], chip.withTrimmedLeft (5), juce::Justification::centredLeft);
    }
}

void AeroCrushEditor::resized()
{
    background = {};
    auto area = getLocalBounds().reduced (16, 0);

    // header
    auto header = area.removeFromTop (74);
    auto pr = header.withTrimmedTop (20).removeFromRight (420).withHeight (32);
    nextButton.setBounds (pr.removeFromRight (36));
    pr.removeFromRight (6);
    prevButton.setBounds (pr.removeFromRight (36));
    pr.removeFromRight (8);
    presetBox.setBounds (pr);

    // glitch panel
    auto gp = area.removeFromTop (214);
    glitchModule->setBounds (gp);
    auto inner = gp.reduced (14, 10).withWidth ((int) (gp.getWidth() * 0.61f));
    auto top = inner.removeFromTop (28);
    top.removeFromLeft (220);
    fillButton.setBounds (top.removeFromRight (64));
    top.removeFromRight (6);
    randomButton.setBounds (top.removeFromRight (92));
    top.removeFromRight (6);
    clearButton.setBounds (top.removeFromRight (64));
    top.removeFromRight (10);
    rateBox.setBounds (top.removeFromRight (88));
    inner.removeFromTop (14);
    seq.setBounds (inner.removeFromTop (108));

    auto layoutRow = [] (juce::Rectangle<int> r, std::vector<std::unique_ptr<Module>>& mods)
    {
        int units = 0;
        for (auto& m : mods) units += m->knobs.size();
        const int gap = 12, total = r.getWidth() - gap * ((int) mods.size() - 1);
        for (size_t i = 0; i < mods.size(); ++i)
        {
            const int w = i + 1 == mods.size() ? r.getWidth() : total * mods[i]->knobs.size() / units;
            mods[i]->setBounds (r.removeFromLeft (w));
            r.removeFromLeft (gap);
        }
    };

    area.removeFromTop (12);
    layoutRow (area.removeFromTop (190), row2);
    area.removeFromTop (12);
    layoutRow (area.removeFromTop (190), row3);
}
