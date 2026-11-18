#pragma once

#include <JuceHeader.h>
#include <cmath>

namespace biosphere::ui
{
inline const juce::Colour navy   = juce::Colour::fromRGB(5, 13, 29);
inline const juce::Colour panel  = juce::Colour::fromRGB(10, 24, 45);
inline const juce::Colour raised = juce::Colour::fromRGB(17, 35, 62);
inline const juce::Colour border = juce::Colour::fromRGB(67, 88, 116);
inline const juce::Colour gold   = juce::Colour::fromRGB(232, 191, 111);
inline const juce::Colour blue   = juce::Colour::fromRGB(110, 174, 232);
inline const juce::Colour white  = juce::Colour::fromRGB(239, 240, 233);
inline const juce::Colour muted  = juce::Colour::fromRGB(151, 167, 181);

inline void roundedPanel(juce::Graphics& g, juce::Rectangle<float> bounds, float corner = 11.0f,
                         juce::Colour edge = border)
{
    juce::ColourGradient fill(panel.brighter(0.08f), bounds.getX(), bounds.getY(),
                              navy.brighter(0.07f), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill(fill);
    g.fillRoundedRectangle(bounds, corner);
    g.setColour(edge.withAlpha(0.9f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), corner, 1.05f);
}

inline void heading(juce::Graphics& g, const juce::String& text, juce::Rectangle<int> area,
                    float size = 13.0f, juce::Colour colour = white)
{
    g.setColour(colour);
    g.setFont(juce::Font(juce::FontOptions(size, juce::Font::bold)));
    g.drawFittedText(text, area, juce::Justification::centredLeft, 1);
}

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour(juce::PopupMenu::backgroundColourId, panel);
        setColour(juce::PopupMenu::textColourId, white);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, raised);
        setColour(juce::PopupMenu::highlightedTextColourId, gold);
        setColour(juce::ComboBox::backgroundColourId, raised);
        setColour(juce::ComboBox::outlineColourId, border);
        setColour(juce::ComboBox::textColourId, white);
        setColour(juce::ComboBox::arrowColourId, gold);
        setColour(juce::Label::textColourId, white);
        setColour(juce::TextButton::buttonColourId, raised);
        setColour(juce::TextButton::buttonOnColourId, gold.darker(0.7f));
        setColour(juce::TextButton::textColourOffId, white);
        setColour(juce::TextButton::textColourOnId, gold);
    }

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPos, float startAngle, float endAngle,
                          juce::Slider& slider) override
    {
        const float diameter = juce::jmin(static_cast<float>(width), static_cast<float>(height)) - 9.0f;
        const float radius = diameter * 0.45f;
        const float cx = static_cast<float>(x) + width * 0.5f;
        const float cy = static_cast<float>(y) + height * 0.5f;
        const auto accent = slider.isEnabled() ? gold : border;
        g.setColour(raised);
        g.fillEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(border.withAlpha(0.9f));
        g.drawEllipse(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f, 1.2f);
        g.setColour(blue.withAlpha(0.22f));
        g.drawEllipse(cx - radius + 5.0f, cy - radius + 5.0f,
                      2.0f * (radius - 5.0f), 2.0f * (radius - 5.0f), 1.3f);

        juce::Path base, active;
        base.addCentredArc(cx, cy, radius + 4.0f, radius + 4.0f, 0.0f, startAngle, endAngle, true);
        active.addCentredArc(cx, cy, radius + 4.0f, radius + 4.0f, 0.0f, startAngle,
                             startAngle + sliderPos * (endAngle - startAngle), true);
        g.setColour(border.withAlpha(0.65f));
        g.strokePath(base, juce::PathStrokeType(2.2f));
        g.setColour(accent);
        g.strokePath(active, juce::PathStrokeType(2.6f));

        const auto angle = startAngle + sliderPos * (endAngle - startAngle);
        juce::Point<float> end(cx + std::sin(angle) * radius * 0.57f,
                               cy - std::cos(angle) * radius * 0.57f);
        g.setColour(accent);
        g.drawLine(cx, cy, end.x, end.y, 2.25f);
        g.fillEllipse(cx - 2.8f, cy - 2.8f, 5.6f, 5.6f);
    }
};
} // namespace biosphere::ui
