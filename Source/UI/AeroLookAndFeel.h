#pragma once
#include <JuceHeader.h>

// Frutiger Aero palette: sky blues, aqua glass, fresh greens, glossy white highlights.
namespace aero
{
const juce::Colour navy   { 0xff0b3a5c };
const juce::Colour aqua   { 0xff29d3ff };
const juce::Colour lime   { 0xff8be04e };
const juce::Colour skyTop { 0xff1f9be0 };
const juce::Colour skyMid { 0xff8fdcff };
const juce::Colour skyLow { 0xffe6f9ff };

inline juce::Font font (float size, bool bold = true)
{
    return juce::Font (juce::FontOptions (size).withStyle (bold ? "Bold" : "Regular"));
}

// Glossy "gel" fill used by panels, pills and steps.
inline void glossyRect (juce::Graphics& g, juce::Rectangle<float> r, float radius, juce::Colour top, juce::Colour bottom)
{
    g.setGradientFill (juce::ColourGradient (top, 0, r.getY(), bottom, 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, radius);
    auto shine = r.reduced (radius * 0.35f, 1.5f).withHeight (r.getHeight() * 0.48f);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.75f), 0, shine.getY(),
                                             juce::Colours::white.withAlpha (0.05f), 0, shine.getBottom(), false));
    g.fillRoundedRectangle (shine, radius * 0.8f);
}
} // namespace aero

class AeroLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AeroLookAndFeel()
    {
        setColour (juce::Slider::textBoxTextColourId, aero::navy);
        setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::white.withAlpha (0.35f));
        setColour (juce::Label::textColourId, aero::navy);
        setColour (juce::ComboBox::textColourId, aero::navy);
        setColour (juce::ComboBox::arrowColourId, aero::navy);
        setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xfff2fbff));
        setColour (juce::PopupMenu::textColourId, aero::navy);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, aero::aqua);
        setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
        setColour (juce::TextButton::textColourOffId, aero::navy);
        setColour (juce::TextButton::textColourOnId, aero::navy);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                           float start, float end, juce::Slider&) override
    {
        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3);
        const float r = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
        const auto c = bounds.getCentre();
        const float ang = start + pos * (end - start);
        const juce::PathStrokeType stroke (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);

        juce::Path track;
        track.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, end, true);
        g.setColour (juce::Colours::white.withAlpha (0.45f));
        g.strokePath (track, stroke);

        juce::Path value;
        value.addCentredArc (c.x, c.y, r - 2, r - 2, 0, start, ang, true);
        g.setGradientFill (juce::ColourGradient (aero::aqua, c.x - r, c.y, aero::lime, c.x + r, c.y, false));
        g.strokePath (value, stroke);

        // glass orb
        const float orbR = r - 8;
        auto orb = juce::Rectangle<float> (orbR * 2, orbR * 2).withCentre (c);
        g.setColour (juce::Colours::black.withAlpha (0.18f));
        g.fillEllipse (orb.translated (0, 3));

        juce::ColourGradient body (juce::Colour (0xffb5efff), c.x, orb.getY(), juce::Colour (0xff0a63a8), c.x, orb.getBottom(), false);
        body.addColour (0.55, juce::Colour (0xff29a6e6));
        g.setGradientFill (body);
        g.fillEllipse (orb);

        // inner bottom glow (light passing through the glass)
        auto glow = orb.reduced (orbR * 0.25f).withTrimmedTop (orbR * 0.7f);
        g.setGradientFill (juce::ColourGradient (aero::aqua.withAlpha (0.0f), c.x, glow.getY(),
                                                 juce::Colour (0xff9ff6ff).withAlpha (0.7f), c.x, glow.getBottom(), false));
        g.fillEllipse (glow);

        g.setColour (juce::Colours::white.withAlpha (0.8f));
        g.drawEllipse (orb, 1.2f);

        auto gloss = orb.reduced (orbR * 0.2f, 0).withHeight (orbR * 0.9f).translated (0, orbR * 0.07f);
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.9f), c.x, gloss.getY(),
                                                 juce::Colours::white.withAlpha (0.0f), c.x, gloss.getBottom(), false));
        g.fillEllipse (gloss);

        const auto p = c.getPointOnCircumference (orbR * 0.7f, ang);
        g.setColour (juce::Colours::white);
        g.fillEllipse (juce::Rectangle<float> (7, 7).withCentre (p));
        g.setColour (aero::navy.withAlpha (0.5f));
        g.drawEllipse (juce::Rectangle<float> (7, 7).withCentre (p), 1.0f);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& b, bool hover, bool) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.5f);
        const bool isOn = b.getToggleState();
        const float rad = r.getHeight() * 0.5f;
        aero::glossyRect (g, r, rad,
                          isOn ? juce::Colour (0xff9df59f) : juce::Colour (0xffeaf6fc),
                          isOn ? juce::Colour (0xff1c9b45) : juce::Colour (0xff86b3cc));
        g.setColour (juce::Colours::white.withAlpha (hover ? 1.0f : 0.8f));
        g.drawRoundedRectangle (r, rad, 1.2f);

        // LED
        auto led = juce::Rectangle<float> (r.getHeight() * 0.42f, r.getHeight() * 0.42f)
                       .withCentre ({ r.getX() + rad, r.getCentreY() });
        g.setColour (isOn ? juce::Colour (0xffe9ffb0) : juce::Colour (0xff5d7d92));
        g.fillEllipse (led);
        if (isOn)
        {
            g.setColour (juce::Colour (0xffe9ffb0).withAlpha (0.35f));
            g.fillEllipse (led.expanded (3));
        }

        g.setColour (isOn ? juce::Colours::white : aero::navy);
        g.setFont (aero::font (13.0f));
        g.drawText (b.getButtonText(), r.withTrimmedLeft (rad * 2), juce::Justification::centredLeft);
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hover, bool down) override
    {
        auto r = b.getLocalBounds().toFloat().reduced (1.5f);
        const float rad = r.getHeight() * 0.5f;
        aero::glossyRect (g, r, rad, juce::Colour (0xffd9f6ff).brighter (hover ? 0.2f : 0.0f),
                          down ? juce::Colour (0xff1f8fd0) : juce::Colour (0xff57b9e8));
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawRoundedRectangle (r, rad, 1.2f);
    }

    juce::Font getTextButtonFont (juce::TextButton&, int) override { return aero::font (12.5f); }

    void drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box) override
    {
        auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (1.5f);
        const float rad = r.getHeight() * 0.5f;
        aero::glossyRect (g, r, rad, juce::Colours::white, juce::Colour (0xffa9dcf5));
        g.setColour (juce::Colours::white);
        g.drawRoundedRectangle (r, rad, 1.2f);

        juce::Path arrow;
        const float ax = r.getRight() - rad - 4, ay = r.getCentreY();
        arrow.addTriangle (ax - 5, ay - 2.5f, ax + 5, ay - 2.5f, ax, ay + 3.5f);
        g.setColour (box.findColour (juce::ComboBox::arrowColourId));
        g.fillPath (arrow);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (12, 0, box.getWidth() - 34, box.getHeight());
        label.setFont (getComboBoxFont (box));
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return aero::font (14.0f); }
    juce::Font getLabelFont (juce::Label&) override { return aero::font (12.5f); }
    juce::Font getPopupMenuFont() override { return aero::font (14.0f, false); }
};
