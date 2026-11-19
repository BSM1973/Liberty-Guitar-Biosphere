#include "EcosystemView.h"
#include <array>
#include <cmath>

namespace biosphere::ui
{
void EcosystemView::Hotspot::paintButton(juce::Graphics& g, bool highlighted, bool down)
{
    if (!highlighted && !down && !hasKeyboardFocus(true))
        return;

    // Preserve the reference UI: hover/focus highlights only the circular icon,
    // never a large rectangle over the ecosystem orbit or its labels.
    const float cx = getWidth() * 0.5f;
    const float cy = 34.0f;
    g.setColour(gold.withAlpha(down ? 0.30f : 0.10f));
    g.fillEllipse(cx - 27.0f, cy - 27.0f, 54.0f, 54.0f);
    g.setColour(gold.withAlpha(hasKeyboardFocus(true) ? 0.9f : 0.40f));
    g.drawEllipse(cx - 27.0f, cy - 27.0f, 54.0f, 54.0f,
                  hasKeyboardFocus(true) ? 1.5f : 1.0f);
}

EcosystemView::EcosystemView()
{
    for (int index = 0; index < EngineNavigationState::pageCount; ++index)
    {
        auto button = std::make_unique<Hotspot>(
            juce::String::fromUTF8(enginePageNames[static_cast<std::size_t>(index)]));
        button->setComponentID("engine-node-" + juce::String(index));
        button->setTooltip("Afficher le panneau " +
            juce::String::fromUTF8(enginePageNames[static_cast<std::size_t>(index)]));
        button->setClickingTogglesState(true);
        button->setWantsKeyboardFocus(true);
        button->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        button->onClick = [this, index]
        {
            setSelectedEngine(index);
            if (onEngineSelected)
                onEngineSelected(index);
        };
        addAndMakeVisible(*button);
        hotspots_[static_cast<std::size_t>(index)] = std::move(button);
    }
    setSelectedEngine(EngineNavigationState::defaultPage);
}

void EcosystemView::setSelectedEngine(int index)
{
    if (index < 0 || index >= EngineNavigationState::pageCount)
        return;
    selected_ = index;
    for (int i = 0; i < EngineNavigationState::pageCount; ++i)
        hotspots_[static_cast<std::size_t>(i)]->setToggleState(i == index, juce::dontSendNotification);
    repaint();
}

void EcosystemView::resized()
{
    for (int i = 0; i < EngineNavigationState::pageCount; ++i)
        hotspots_[static_cast<std::size_t>(i)]->setBounds(getNodeHitBox(i).toNearestInt());
}

juce::Point<float> EcosystemView::getNodeCentre(int index) const
{
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());
    const std::array<float, 6> horizontal { 0.305f, 0.525f, 0.745f,
                                            0.305f, 0.525f, 0.745f };
    const float vertical = index < 3 ? 0.26f : 0.76f;
    return { width * horizontal[static_cast<std::size_t>(index)], height * vertical };
}

juce::Rectangle<float> EcosystemView::getNodeHitBox(int index) const
{
    const auto centre = getNodeCentre(index);
    return { centre.x - 82.0f, centre.y - 34.0f, 164.0f, 78.0f };
}

void EcosystemView::paint(juce::Graphics& g)
{
    roundedPanel(g, getLocalBounds().toFloat().reduced(1.0f));
    heading(g, tr(juce::String::fromUTF8("ÉCOSYSTÈME"), english_), { 20, 13, 220, 25 }, 13.0f, white);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(tr("Choisissez un moteur", english_), 20, 36, 175, 17, juce::Justification::left);

    const float w = static_cast<float>(getWidth());
    const float h = static_cast<float>(getHeight());
    const juce::Point<float> centre { w * 0.525f, h * 0.51f };

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
    g.drawText(tr(juce::String::fromUTF8("ENTRÉE"), english_), 30, static_cast<int>(h * 0.46f),
               81, 29, juce::Justification::centred);

    g.setColour(blue.withAlpha(0.40f));
    g.drawLine(114.0f, h * 0.46f + 15.0f, centre.x - 66.0f, h * 0.46f + 15.0f, 1.1f);

    juce::ColourGradient hub(raised, centre.x, centre.y - 45.0f, navy, centre.x, centre.y + 45.0f, false);
    g.setGradientFill(hub);
    g.fillEllipse(centre.x - 35.0f, centre.y - 35.0f, 70.0f, 70.0f);
    g.setColour(gold.withAlpha(0.65f));
    g.drawEllipse(centre.x - 46.0f, centre.y - 46.0f, 92.0f, 92.0f, 1.3f);
    g.setColour(blue.withAlpha(0.5f));
    g.drawEllipse(centre.x - 29.0f, centre.y - 29.0f, 58.0f, 58.0f, 1.0f);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.0f, juce::Font::bold)));
    g.drawText(tr(juce::String::fromUTF8("ÉTAT VIVANT"), english_),
               static_cast<int>(centre.x - 34), static_cast<int>(centre.y - 19), 68, 16,
               juce::Justification::centred);
    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(13.5f, juce::Font::bold)));
    g.drawText(tr("CALME", english_), static_cast<int>(centre.x - 31), static_cast<int>(centre.y - 2),
               62, 24, juce::Justification::centred);
    g.setColour(gold);
    g.drawLine(centre.x - 17.0f, centre.y + 21.0f,
               centre.x + 17.0f, centre.y + 21.0f, 2.0f);

    for (int i = 0; i < 6; ++i)
    {
        const auto p = getNodeCentre(i);
        const auto accent = i == selected_ ? gold : ((i == 1 || i == 5) ? gold : blue);
        const float radius = 19.0f;
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
        g.drawText(juce::String::fromUTF8(enginePageNames[static_cast<std::size_t>(i)]), static_cast<int>(p.x - 80.0f),
                   static_cast<int>(p.y + 26.0f), 160, 18, juce::Justification::centred);
    }
    g.setColour(muted.withAlpha(0.75f));
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(tr("8 routes à connecter", english_), 20, getHeight() - 24, 170, 17, juce::Justification::left);
}

void AnalysisPanel::paint(juce::Graphics& g)
{
    roundedPanel(g, getLocalBounds().toFloat().reduced(1.0f));
    heading(g, tr(juce::String::fromUTF8("ANALYSE PARTAGÉE"), english_),
            { 18, 12, getWidth() - 36, 24 }, 12.0f);

    const std::array<juce::String, 4> labels {
        tr("Enveloppe", english_), tr("Attaques", english_),
        tr(juce::String::fromUTF8("Activité rythmique"), english_),
        tr(juce::String::fromUTF8("Activité mémoire"), english_)
    };
    const std::array<float, 4> levels {
        metrics_.envelope, metrics_.attacks, metrics_.rhythm, metrics_.memory
    };
    for (int i = 0; i < 4; ++i)
    {
        const int y = 43 + i * 24;
        g.setColour(white.withAlpha(0.86f));
        g.setFont(juce::Font(juce::FontOptions(10.5f)));
        g.drawFittedText(labels[static_cast<std::size_t>(i)],
                         17, y, 143, 18, juce::Justification::centredLeft, 1);

        const float x = 165.0f, width = static_cast<float>(juce::jmax(10, getWidth() - 182));
        g.setColour(border.withAlpha(0.58f));
        g.fillRoundedRectangle(x, static_cast<float>(y + 7), width, 5.0f, 2.0f);
        g.setColour((i == 1 || i == 3 ? gold : blue).withAlpha(0.94f));
        g.fillRoundedRectangle(x, static_cast<float>(y + 7),
                               width * juce::jlimit(0.0f, 1.0f, levels[static_cast<std::size_t>(i)]),
                               5.0f, 2.0f);
    }
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f, juce::Font::bold)));
    g.drawText(tr(juce::String::fromUTF8("ÉNERGIE TONALE"), english_),
               17, 149, 180, 17, juce::Justification::left);
    g.setColour(border.withAlpha(0.58f));
    g.fillRoundedRectangle(17.0f, 170.0f, static_cast<float>(getWidth() - 34), 5.0f, 2.0f);
    g.setColour(gold);
    g.fillRoundedRectangle(17.0f, 170.0f,
                           static_cast<float>(getWidth() - 34)
                               * juce::jlimit(0.0f, 1.0f, metrics_.tonal), 5.0f, 2.0f);
    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText(tr("Racine harmonique", english_), 17, 185, 165, 16,
               juce::Justification::left);
    g.setColour(gold);
    const auto root = metrics_.rootHz > 0.0f
        ? juce::String(metrics_.rootHz, 1) + " Hz" : juce::String("-");
    g.drawText(root, getWidth() - 112, 184, 95, 20, juce::Justification::centredRight);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(8.5f)));
    g.drawText(english_ ? "Live DSP measurements" : "Mesures DSP en direct",
               17, getHeight() - 20, getWidth() - 34, 15, juce::Justification::left);
}
} // namespace biosphere::ui
