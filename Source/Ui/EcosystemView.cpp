#include "EcosystemView.h"
#include <array>
#include <cmath>

namespace biosphere::ui
{
juce::Point<float> EcosystemView::getNodeCentre(int index) const
{
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());
    const std::array<float, 6> horizontal { 0.305f, 0.525f, 0.745f,
                                            0.305f, 0.525f, 0.745f };
    const float vertical = index < 3 ? 0.31f : 0.75f;
    return { width * horizontal[static_cast<std::size_t>(index)], height * vertical };
}

juce::Rectangle<float> EcosystemView::getNodeHitBox(int index) const
{
    const auto centre = getNodeCentre(index);
    return { centre.x - 47.0f, centre.y - 34.0f, 94.0f, 78.0f };
}

void EcosystemView::paint(juce::Graphics& g)
{
    roundedPanel(g, getLocalBounds().toFloat().reduced(1.0f));
    heading(g, juce::String::fromUTF8("ÉCOSYSTÈME"), { 20, 13, 220, 25 }, 13.0f, white);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Choisissez un moteur", 20, 36, 240, 17, juce::Justification::left);

    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const juce::Point<float> centre { w * 0.525f, h * 0.53f };

    juce::ColourGradient halo(blue.withAlpha(0.21f), centre.x, centre.y,
                              navy.withAlpha(0.0f), centre.x + w * 0.31f, centre.y, true);
    g.setGradientFill(halo);
    g.fillEllipse(centre.x - w * 0.33f, centre.y - h * 0.48f, w * 0.66f, h * 0.96f);

    g.setColour(blue.withAlpha(0.22f));
    g.drawEllipse(w * 0.20f, h * 0.09f, w * 0.65f, h * 0.82f, 1.0f);
    g.setColour(border.withAlpha(0.45f));
    g.drawEllipse(w * 0.27f, h * 0.21f, w * 0.51f, h * 0.57f, 1.0f);

    g.setColour(gold.withAlpha(0.80f));
    g.drawRoundedRectangle(juce::Rectangle<float>(27.0f, h * 0.46f, 86.0f, 29.0f), 5.0f, 1.0f);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText(juce::String::fromUTF8("• ENTRÉE"), 30, static_cast<int>(h * 0.46f),
               81, 29, juce::Justification::centred);

    g.setColour(blue.withAlpha(0.40f));
    g.drawLine(114.0f, h * 0.46f + 15.0f, centre.x - 66.0f, h * 0.46f + 15.0f, 1.1f);

    juce::ColourGradient hub(raised, centre.x, centre.y - 45.0f, navy, centre.x, centre.y + 45.0f, false);
    g.setGradientFill(hub);
    g.fillEllipse(centre.x - 46.0f, centre.y - 46.0f, 92.0f, 92.0f);
    g.setColour(gold.withAlpha(0.65f));
    g.drawEllipse(centre.x - 46.0f, centre.y - 46.0f, 92.0f, 92.0f, 1.3f);
    g.setColour(blue.withAlpha(0.5f));
    g.drawEllipse(centre.x - 40.0f, centre.y - 40.0f, 80.0f, 80.0f, 1.0f);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText(juce::String::fromUTF8("ÉTAT VIVANT"),
               static_cast<int>(centre.x - 42), static_cast<int>(centre.y - 21), 84, 16,
               juce::Justification::centred);
    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(13.5f, juce::Font::bold)));
    g.drawText("CALME", static_cast<int>(centre.x - 38), static_cast<int>(centre.y - 1),
               76, 25, juce::Justification::centred);
    g.setColour(gold);
    g.drawLine(centre.x - 21.0f, centre.y + 24.0f,
               centre.x + 21.0f, centre.y + 24.0f, 2.0f);

    const std::array<juce::String, 6> names {
        "PULSE", "HARMONIC LIFE", "MUTATION", "EVOLUTION", "MEMORY", "REACTION"
    };
    for (int i = 0; i < 6; ++i)
    {
        const auto p = getNodeCentre(i);
        const auto accent = i == selected_ ? gold : ((i == 1 || i == 5) ? gold : blue);
        const float radius = 23.0f;
        g.setColour(raised);
        g.fillEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f);
        g.setColour(accent.withAlpha(i == selected_ ? 1.0f : 0.68f));
        g.drawEllipse(p.x - radius, p.y - radius, radius * 2.0f, radius * 2.0f, 1.5f);
        g.drawEllipse(p.x - radius + 4.0f, p.y - radius + 4.0f,
                      radius * 2.0f - 8.0f, radius * 2.0f - 8.0f, 0.9f);

        // Each node has its own vector glyph, never a static image hotspot.
        juce::Path mark;
        switch (i)
        {
            case 0: // PULSE
                mark.startNewSubPath(p.x - 15, p.y);
                mark.lineTo(p.x - 9, p.y);
                mark.lineTo(p.x - 5, p.y - 10);
                mark.lineTo(p.x + 1, p.y + 10);
                mark.lineTo(p.x + 7, p.y - 5);
                mark.lineTo(p.x + 12, p.y);
                mark.lineTo(p.x + 15, p.y);
                break;
            case 1: // HARMONIC LIFE
                mark.startNewSubPath(p.x - 12, p.y + 12);
                mark.lineTo(p.x + 12, p.y + 12);
                mark.startNewSubPath(p.x - 8, p.y + 10);
                mark.lineTo(p.x - 8, p.y - 12);
                mark.startNewSubPath(p.x, p.y + 10);
                mark.lineTo(p.x, p.y - 15);
                mark.startNewSubPath(p.x + 8, p.y + 10);
                mark.lineTo(p.x + 8, p.y - 9);
                break;
            case 2: // MUTATION
                mark.addRectangle(p.x - 10, p.y - 10, 8, 8);
                mark.addRectangle(p.x + 3, p.y + 1, 10, 10);
                break;
            case 3: // EVOLUTION
                mark.startNewSubPath(p.x - 12, p.y + 12);
                mark.quadraticTo(p.x - 10, p.y - 12, p.x + 12, p.y - 12);
                mark.quadraticTo(p.x + 10, p.y + 11, p.x - 12, p.y + 12);
                mark.startNewSubPath(p.x - 8, p.y + 8);
                mark.lineTo(p.x + 9, p.y - 8);
                break;
            case 4: // MEMORY
                mark.addArc(p.x - 12, p.y - 12, 24, 24, 0.1f, 5.0f, true);
                mark.startNewSubPath(p.x, p.y - 9);
                mark.lineTo(p.x, p.y);
                mark.lineTo(p.x + 8, p.y + 5);
                break;
            case 5: // REACTION
                mark.startNewSubPath(p.x - 11, p.y - 9);
                mark.lineTo(p.x + 11, p.y + 9);
                mark.startNewSubPath(p.x - 11, p.y + 9);
                mark.lineTo(p.x + 11, p.y - 9);
                break;
            default: break;
        }
        g.setColour(accent);
        g.strokePath(mark, juce::PathStrokeType(1.7f));

        g.setColour(i == selected_ ? gold : white.withAlpha(0.86f));
        g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
        g.drawText(names[static_cast<std::size_t>(i)], static_cast<int>(p.x - 80.0f),
                   static_cast<int>(p.y + 26.0f), 160, 18, juce::Justification::centred);
    }
    g.setColour(muted.withAlpha(0.75f));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText("8 routes à connecter", 20, getHeight() - 24, 170, 17, juce::Justification::left);
}

void EcosystemView::mouseDown(const juce::MouseEvent& event)
{
    for (int i = 0; i < 6; ++i)
    {
        if (getNodeHitBox(i).contains(event.position))
        {
            setSelectedEngine(i);
            if (onEngineSelected) onEngineSelected(i);
            return;
        }
    }
}

void EcosystemView::mouseMove(const juce::MouseEvent& event)
{
    bool overNode = false;
    for (int i = 0; i < 6; ++i)
        overNode = overNode || getNodeHitBox(i).contains(event.position);
    setMouseCursor(overNode ? juce::MouseCursor::PointingHandCursor : juce::MouseCursor::NormalCursor);
}

void AnalysisPanel::paint(juce::Graphics& g)
{
    roundedPanel(g, getLocalBounds().toFloat().reduced(1.0f));
    heading(g, juce::String::fromUTF8("ANALYSE PARTAGÉE"), { 18, 12, getWidth() - 36, 24 }, 12.0f);
    constexpr std::array<const char*, 4> labels {
        "Enveloppe", "Attaques", "Activité rythmique", "Activité mémoire"
    };
    for (int i = 0; i < 4; ++i)
    {
        const int y = 43 + i * 24;
        g.setColour(white.withAlpha(0.82f));
        g.setFont(juce::Font(juce::FontOptions(10.5f)));
        g.drawText(juce::String::fromUTF8(labels[static_cast<std::size_t>(i)]),
                   17, y, 143, 18, juce::Justification::left);
        g.setColour(border.withAlpha(0.55f));
        g.fillRoundedRectangle(juce::Rectangle<float>(165.0f, static_cast<float>(y + 7),
                               static_cast<float>(juce::jmax(10, getWidth() - 182)), 4.0f), 2.0f);
    }
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText(juce::String::fromUTF8("ÉNERGIE TONALE"), 17, 149, 180, 17,
               juce::Justification::left);
    g.setColour(border.withAlpha(0.55f));
    g.fillRoundedRectangle(17.0f, 170.0f, static_cast<float>(getWidth() - 34), 5.0f, 2.0f);
    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText("Racine harmonique", 17, 186, 165, 16, juce::Justification::left);
    g.setColour(gold);
    g.drawText("—", getWidth() - 70, 184, 53, 20, juce::Justification::centredRight);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(8.5f)));
    g.drawText(juce::String::fromUTF8("Mesures à raccorder au DSP"),
               17, getHeight() - 20, getWidth() - 34, 15, juce::Justification::left);
}
} // namespace biosphere::ui
