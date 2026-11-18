#include "PluginEditor.h"

namespace
{
const juce::Colour background = juce::Colour::fromRGB(12, 20, 22);
const juce::Colour surface = juce::Colour::fromRGB(22, 34, 36);
const juce::Colour emerald = juce::Colour::fromRGB(73, 212, 163);
const juce::Colour copper = juce::Colour::fromRGB(220, 163, 97);
const juce::Colour white = juce::Colour::fromRGB(230, 238, 232);
}

GuitarBiosphereAudioProcessorEditor::GuitarBiosphereAudioProcessorEditor(
    GuitarBiosphereAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor), processor_(processor)
{
    constexpr std::array<const char*, 11> ids {
        "pulse", "harmonic", "mutation", "memory", "rate", "root",
        "grain", "feedback", "input", "output", "mix"
    };
    constexpr std::array<const char*, 11> labels {
        "PULSE", "HARMONIC LIFE", "MUTATION", "MEMORY", "RATE (Hz)", "ROOT (Hz)",
        "GRAIN (ms)", "FEEDBACK", "ENTRÉE (dB)", "SORTIE (dB)", "MIX GLOBAL"
    };

    for (std::size_t i = 0; i < controls_.size(); ++i)
    {
        auto& control = controls_[i];
        control.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        control.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 86, 20);
        control.slider.setColour(juce::Slider::rotarySliderFillColourId, emerald);
        control.slider.setColour(juce::Slider::rotarySliderOutlineColourId, juce::Colour::fromRGB(52, 72, 70));
        control.slider.setColour(juce::Slider::thumbColourId, copper);
        control.slider.setColour(juce::Slider::textBoxTextColourId, white);
        control.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
        control.slider.setColour(juce::Slider::textBoxBackgroundColourId, surface);
        addAndMakeVisible(control.slider);

        control.label.setText(juce::String::fromUTF8(labels[i]), juce::dontSendNotification);
        control.label.setJustificationType(juce::Justification::centred);
        control.label.setColour(juce::Label::textColourId, white);
        control.label.setFont(juce::Font(juce::FontOptions(12.0f, juce::Font::bold)));
        addAndMakeVisible(control.label);

        control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
            processor_.parameters, ids[i], control.slider);
    }

    setSize(960, 570);
}

void GuitarBiosphereAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(background);
    g.setColour(surface);
    g.fillRoundedRectangle(26.0f, 165.0f, 908.0f, 165.0f, 15.0f);
    g.fillRoundedRectangle(26.0f, 350.0f, 908.0f, 185.0f, 15.0f);

    g.setColour(emerald);
    g.setFont(juce::Font(juce::FontOptions(33.0f, juce::Font::bold)));
    g.drawText("GUITAR BIOSPHERE", 30, 20, getWidth() - 60, 44, juce::Justification::centred);

    g.setColour(copper);
    g.setFont(juce::Font(juce::FontOptions(15.0f)));
    g.drawText("LIBERTY MUSIC SOFTWARE  •  THE LIVING GUITAR EFFECTS ENGINE",
               30, 67, getWidth() - 60, 25, juce::Justification::centred);

    g.setColour(white.withAlpha(0.78f));
    g.setFont(juce::Font(juce::FontOptions(13.0f)));
    g.drawFittedText("FONDATION DSP : 4 MOTEURS AUDIO RÉELS — REACTION ET EVOLUTION EN DÉVELOPPEMENT",
                     35, 105, getWidth() - 70, 30, juce::Justification::centred, 1);

    g.setColour(copper);
    g.setFont(juce::Font(juce::FontOptions(13.0f, juce::Font::bold)));
    g.drawText("MOTEURS & ACCORDAGE", 43, 168, 300, 22, juce::Justification::left);
    g.drawText("PARAMÈTRES & MIXAGE", 43, 355, 300, 22, juce::Justification::left);

    g.setColour(white.withAlpha(0.5f));
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText("Version 0.1 · traitement natif C++ / JUCE", 30, 540, getWidth() - 60, 20,
               juce::Justification::centred);
}

void GuitarBiosphereAudioProcessorEditor::resized()
{
    // Six controls in the first panel, five in the second. No scroll bars.
    for (std::size_t i = 0; i < controls_.size(); ++i)
    {
        const bool firstRow = i < 6;
        const int index = firstRow ? static_cast<int>(i) : static_cast<int>(i - 6);
        const int count = firstRow ? 6 : 5;
        const int cellWidth = 880 / count;
        const int cellX = 40 + index * cellWidth;
        const int y = firstRow ? 195 : 389;
        auto& control = controls_[i];
        control.label.setBounds(cellX, y, cellWidth, 20);
        control.slider.setBounds(cellX + (cellWidth - 116) / 2, y + 19, 116, 112);
    }
}
