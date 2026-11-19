#include "PluginEditor.h"
#include <algorithm>

using namespace biosphere::ui;

GuitarBiosphereAudioProcessorEditor::GuitarBiosphereAudioProcessorEditor(
    GuitarBiosphereAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor),
      processor_(processor),
      presetLibrary_(processor.parameters), browser_(presetLibrary_),
      pulse_(processor.parameters), harmonic_(processor.parameters),
      mutation_(processor.parameters), reaction_(processor.parameters),
      memory_(processor.parameters), evolution_(processor.parameters)
{
    setLookAndFeel(&theme_);

    // Exact on-screen node order: top (Pulse, Harmonic, Mutation),
    // then bottom (Evolution, Memory, Reaction).
    pages_ = { &pulse_, &harmonic_, &mutation_, &evolution_, &memory_, &reaction_ };
    jassert(pages_.size() == static_cast<std::size_t>(biosphere::ui::EngineNavigationState::pageCount));
    for (auto* panel : pages_)
        addAndMakeVisible(*panel);

    ecosystem_.onEngineSelected = [this](int index) { selectEngine(index); };
    addAndMakeVisible(ecosystem_);
    addAndMakeVisible(analysis_);

    addAndMakeVisible(preset_);
    preset_.setTooltip("Choisir un préréglage d'usine ou personnel.");
    preset_.onChange = [this]
    {
        if (preset_.getSelectedId() > 1)
            presetLibrary_.load(preset_.getSelectedId() - 2);
    };
    refreshPresetSelector();

    for (auto* button : { &previous_, &next_, &library_, &french_, &english_ })
    {
        button->setColour(juce::TextButton::buttonColourId, raised);
        button->setColour(juce::TextButton::textColourOffId, white);
        addAndMakeVisible(button);
    }
    french_.setColour(juce::TextButton::buttonColourId, gold.darker(0.63f));
    french_.setColour(juce::TextButton::textColourOffId, gold);
    previous_.setTooltip("Préréglage précédent");
    next_.setTooltip("Préréglage suivant");
    library_.setTooltip("Ouvrir la bibliothèque, chercher et enregistrer des préréglages.");
    previous_.onClick = [this] { chooseRelativePreset(-1); };
    next_.onClick = [this] { chooseRelativePreset(1); };
    library_.onClick = [this] { openBrowser(); };
    french_.onClick = [this] { setLanguage(false); };
    english_.onClick = [this] { setLanguage(true); };

    initialiseGlobalControl(0, "Croissance", "macro.growth", 0, 1, 0.35f, " %");
    initialiseGlobalControl(1, "Mouvement", "macro.motion", 0, 1, 0.35f, " %");
    initialiseGlobalControl(2, "Espace", "macro.space", 0, 1, 0.50f, " %");
    initialiseGlobalControl(3, juce::String::fromUTF8("Énergie"), "macro.energy", 0, 1, 0.35f, " %");
    initialiseGlobalControl(4, juce::String::fromUTF8("Entrée"), "input", -24, 18, 0, " dB");
    initialiseGlobalControl(5, "Sortie", "output", -24, 12, 0, " dB");
    initialiseGlobalControl(6, "Mix maître", "mix", 0, 1, 0.65f, " %");
    initialiseGlobalControl(7, juce::String::fromUTF8("Intensité globale"), "global.intensity", 0, 1, 0.50f, " %");
    initialiseGlobalControl(8, "Vitesse globale", "global.speed", 0.25f, 4.0f, 1.0f, " x");
    initialiseGlobalControl(9, "Largeur stéréo", "global.width", 0, 1, 0.80f, " %");

    for (auto* button : { &sync_, &freeze_, &bypass_ })
    {
        button->setColour(juce::TextButton::buttonColourId, raised);
        button->setColour(juce::TextButton::textColourOffId, white);
        button->setClickingTogglesState(true);
        button->setColour(juce::TextButton::buttonOnColourId, gold.darker(0.65f));
        addAndMakeVisible(button);
    }
    sync_.setToggleState(true, juce::dontSendNotification);
    freeze_.setToggleState(false, juce::dontSendNotification);
    bypass_.setToggleState(false, juce::dontSendNotification);

    sync_.setTooltip("Synchronisation PULSE au tempo du DAW, lorsque le tempo hôte est disponible.");
    freeze_.setTooltip("Fige le tampon MEMORY et conserve les fragments capturés.");
    bypass_.setTooltip("Contourne le traitement avec un fondu court vers la guitare brute.");
    syncAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, "global.sync", sync_);
    freezeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, "global.freeze", freeze_);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, "global.bypass", bypass_);

    browser_.onClose = [this] { browser_.setVisible(false); };
    browser_.onPresetLoaded = [this] { refreshPresetSelector(); };
    browser_.onLibraryChanged = [this] { refreshPresetSelector(); };
    addAndMakeVisible(browser_);
    browser_.setVisible(false);

    selectEngine(processor_.getSelectedEditorPage());
    setResizable(false, false);
    setSize(1200, 748);
    setLanguage(processor_.isEnglish());
    startTimerHz(30);
}

GuitarBiosphereAudioProcessorEditor::~GuitarBiosphereAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void GuitarBiosphereAudioProcessorEditor::timerCallback()
{
    const auto& m = processor_.analysis;
    analysis_.setValues({
        m.envelope.load(std::memory_order_relaxed),
        m.attacks.load(std::memory_order_relaxed),
        m.rhythmicActivity.load(std::memory_order_relaxed),
        m.memoryActivity.load(std::memory_order_relaxed),
        m.tonalEnergy.load(std::memory_order_relaxed),
        m.rootHz.load(std::memory_order_relaxed)
    });
    // Host position and live input/output levels are drawn in the header.
    repaint(960, 17, 240, 76);
}

void GuitarBiosphereAudioProcessorEditor::setLanguage(bool english)
{
    processor_.setEnglish(english);
    ecosystem_.setLanguage(english);
    analysis_.setLanguage(english);
    browser_.setLanguage(english);
    for (auto* panel : pages_)
        panel->setLanguage(english);
    const std::array<juce::String, 10> names {
        "Croissance", "Mouvement", "Espace", juce::String::fromUTF8("Énergie"),
        juce::String::fromUTF8("Entrée"), "Sortie", "Mix maître",
        juce::String::fromUTF8("Intensité globale"), "Vitesse globale",
        juce::String::fromUTF8("Largeur stéréo")
    };
    for (std::size_t i = 0; i < names.size(); ++i)
        globals_[i].label.setText(tr(names[i], english), juce::dontSendNotification);
    sync_.setButtonText(tr("Synchro hôte", english));
    freeze_.setButtonText(tr("Gel global", english));
    bypass_.setButtonText(tr("Bypass global", english));
    preset_.setTooltip(english ? "Choose a factory or user preset."
                               : "Choisir un préréglage d'usine ou personnel.");
    previous_.setTooltip(english ? "Previous preset" : "Préréglage précédent");
    next_.setTooltip(english ? "Next preset" : "Préréglage suivant");
    library_.setTooltip(english ? "Open, search and save presets."
                                : "Ouvrir, rechercher et enregistrer des préréglages.");
    french_.setColour(juce::TextButton::buttonColourId,
                      english ? raised : gold.darker(0.60f));
    english_.setColour(juce::TextButton::buttonColourId,
                       english ? gold.darker(0.60f) : raised);
    french_.setColour(juce::TextButton::textColourOffId, english ? white : gold);
    english_.setColour(juce::TextButton::textColourOffId, english ? gold : white);
    repaint();
}

void GuitarBiosphereAudioProcessorEditor::refreshPresetSelector()
{
    const auto selected = presetLibrary_.currentName();
    preset_.clear(juce::dontSendNotification);
    preset_.addItem("Init", 1);
    int selectedId = 1;
    const auto& presets = presetLibrary_.all();
    for (int index = 0; index < static_cast<int>(presets.size()); ++index)
    {
        const auto& item = presets[static_cast<std::size_t>(index)];
        preset_.addItem(item.name, index + 2);
        if (item.name == selected) selectedId = index + 2;
    }
    preset_.setSelectedId(selectedId, juce::dontSendNotification);
}

void GuitarBiosphereAudioProcessorEditor::chooseRelativePreset(int increment)
{
    const int size = preset_.getNumItems();
    if (size <= 1) return;
    int current = preset_.getSelectedId() - 1;
    current = (current + increment + size) % size;
    // There is no factory "Init" load action yet; skip the placeholder.
    if (current == 0)
        current = increment < 0 ? size - 1 : 1;
    preset_.setSelectedId(current + 1, juce::sendNotificationSync);
}

void GuitarBiosphereAudioProcessorEditor::openBrowser()
{
    browser_.refresh();
    browser_.setLanguage(processor_.isEnglish());
    browser_.setVisible(true);
    browser_.toFront(true);
    browser_.grabKeyboardFocus();
}

void GuitarBiosphereAudioProcessorEditor::initialiseGlobalControl(
    int index, const juce::String& label, const juce::String& parameterId,
    float minimum, float maximum, float initial, const juce::String& suffix)
{
    auto& control = globals_[static_cast<std::size_t>(index)];
    control.label.setText(label, juce::dontSendNotification);
    control.label.setJustificationType(juce::Justification::centred);
    control.label.setColour(juce::Label::textColourId, white);
    control.label.setFont(juce::Font(juce::FontOptions(10.5f)));
    addAndMakeVisible(control.label);

    control.slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    control.slider.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                                       juce::MathConstants<float>::pi * 2.75f, true);
    control.slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 73, 17);
    control.slider.setColour(juce::Slider::textBoxTextColourId, gold);
    control.slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    control.slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    control.slider.setRange(minimum, maximum, 0.001);
    control.slider.setValue(initial, juce::dontSendNotification);
    control.slider.textFromValueFunction = [suffix, maximum](double value)
    {
        const bool percentage = suffix == " %";
        const double displayed = (percentage && maximum <= 1.0f) ? value * 100.0 : value;
        return juce::String(displayed, percentage || suffix == " dB" ? 0 : 1) + suffix;
    };
    addAndMakeVisible(control.slider);

    jassert(processor_.parameters.getParameter(parameterId) != nullptr);
    control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.parameters, parameterId, control.slider);
    control.slider.setTooltip("Automatisation et mémorisation APVTS : " + parameterId);
}

void GuitarBiosphereAudioProcessorEditor::selectEngine(int index)
{
    if (!navigation_.select(index))
        return;

    // Exactly one engine panel is visible. No component is reconstructed,
    // so all APVTS sliders/menus/buttons preserve their values and listeners.
    for (int i = 0; i < biosphere::ui::EngineNavigationState::pageCount; ++i)
        pages_[static_cast<std::size_t>(i)]->setVisible(navigation_.visible(i));

    ecosystem_.setSelectedEngine(navigation_.selected());
    processor_.setSelectedEditorPage(navigation_.selected());
}

void GuitarBiosphereAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(navy);
    juce::ColourGradient backdrop(panel.brighter(0.05f), 0.0f, 0.0f,
                                  navy, static_cast<float>(getWidth()), static_cast<float>(getHeight()), false);
    g.setGradientFill(backdrop);
    g.fillAll();

    // Header: exactly the same regions as the supplied reference, recoloured midnight/gold.
    const auto logo = juce::Rectangle<float>(31.0f, 18.0f, 24.0f, 24.0f);
    g.setColour(gold.withAlpha(0.14f));
    g.fillRoundedRectangle(logo, 4.0f);
    g.setColour(gold);
    g.drawRoundedRectangle(logo, 4.0f, 1.5f);
    g.setFont(juce::Font(juce::FontOptions(18.0f, juce::Font::bold)));
    g.drawText("L", 32, 18, 22, 23, juce::Justification::centred);

    g.setColour(white);
    g.setFont(juce::Font(juce::FontOptions(11.0f, juce::Font::bold)));
    g.drawText("L I B E R T Y   M U S I C   S O F T W A R E", 67, 20, 385, 22,
               juce::Justification::centredLeft);

    g.setFont(juce::Font(juce::FontOptions(30.0f)));
    g.drawText("GUITAR", 30, 44, 155, 37, juce::Justification::left);
    g.setColour(gold);
    g.setFont(juce::Font(juce::FontOptions(30.0f, juce::Font::bold)));
    g.drawText("BIOSPHERE", 174, 44, 305, 37, juce::Justification::left);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawText(tr(juce::String::fromUTF8("LE MOTEUR D'EFFETS GUITARE VIVANT"),
                  processor_.isEnglish()),
               31, 82, 440, 16, juce::Justification::left);

    roundedPanel(g, { 550.0f, 15.0f, 405.0f, 77.0f }, 8.0f);
    heading(g, tr(juce::String::fromUTF8("PRÉRÉGLAGE"), processor_.isEnglish()), { 567, 19, 185, 17 }, 10.0f, muted);
    g.setColour(gold);
    g.setFont(juce::Font(juce::FontOptions(11.0f)));
    g.drawText(processor_.analysis.hostBpm.load(std::memory_order_relaxed) > 0.0f
                   ? juce::String(processor_.analysis.hostBpm.load(std::memory_order_relaxed), 0) + " BPM"
                   : "— BPM", 962, 21, 90, 20, juce::Justification::left);

    heading(g, tr(juce::String::fromUTF8("ENTRÉE"), processor_.isEnglish()), { 963, 52, 85, 14 }, 10.0f, muted);
    heading(g, tr("SORTIE", processor_.isEnglish()), { 1080, 52, 80, 14 }, 10.0f, muted);
    const float input = processor_.analysis.inputPeak.load(std::memory_order_relaxed);
    const float output = processor_.analysis.outputPeak.load(std::memory_order_relaxed);
    for (int i = 0; i < 9; ++i)
    {
        g.setColour(i < static_cast<int>(juce::jlimit(0.0f, 1.0f, input) * 9.0f)
                        ? gold : border.withAlpha(0.48f));
        g.fillRect(963 + i * 10, 71, 7, 5);
        g.setColour(i < static_cast<int>(juce::jlimit(0.0f, 1.0f, output) * 9.0f)
                        ? blue : border.withAlpha(0.48f));
        g.fillRect(1080 + i * 10, 71, 7, 5);
    }

    g.setColour(border);
    g.drawLine(29.0f, 102.0f, static_cast<float>(getWidth() - 29), 102.0f, 1.0f);

    roundedPanel(g, { 28.0f, 577.0f, 1144.0f, 142.0f });
    heading(g, tr("MACROS DE PERFORMANCE", processor_.isEnglish()), { 47, 584, 390, 19 }, 11.0f);
    heading(g, tr(juce::String::fromUTF8("BUS MAÎTRE"), processor_.isEnglish()), { 452, 584, 500, 19 }, 11.0f);
    g.setColour(border);
    g.drawLine(443.0f, 596.0f, 443.0f, 705.0f, 1.0f);

    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(9.6f)));
    g.drawText(tr("Votre guitare. Un univers sonore vivant.", processor_.isEnglish()), 30, 724, 330, 18,
               juce::Justification::left);
    g.drawText(tr(juce::String::fromUTF8("Phase 5 — analyse, presets et langues FR/EN."), processor_.isEnglish()),
               360, 724, 510, 18, juce::Justification::centred);
    g.setColour(gold);
    g.drawText(tr("DSP & AIDE", processor_.isEnglish()), 1060, 724, 110, 18, juce::Justification::centredRight);
}

void GuitarBiosphereAudioProcessorEditor::resized()
{
    preset_.setBounds(567, 42, 244, 33);
    previous_.setBounds(821, 42, 35, 33);
    next_.setBounds(861, 42, 35, 33);
    library_.setBounds(901, 42, 40, 33);
    french_.setBounds(1094, 17, 37, 29);
    english_.setBounds(1138, 17, 37, 29);

    ecosystem_.setBounds(28, 112, 834, 218);
    analysis_.setBounds(875, 112, 297, 218);
    for (auto* panel : pages_)
        panel->setBounds(28, 342, 1144, 221);

    // Four macro dials followed by six master controls, all visible on a fixed 13-inch window.
    for (int i = 0; i < static_cast<int>(globals_.size()); ++i)
    {
        auto& control = globals_[static_cast<std::size_t>(i)];
        const bool macro = i < 4;
        const int withinGroup = macro ? i : i - 4;
        const int cell = macro ? 95 : 97;
        const int left = macro ? 45 : 452;
        const int x = left + withinGroup * cell;
        control.label.setBounds(x, 608, cell - 4, 18);
        control.slider.setBounds(x + 11, 630, cell - 26, 77);
    }
    sync_.setBounds(1044, 603, 112, 31);
    freeze_.setBounds(1044, 639, 112, 31);
    bypass_.setBounds(1044, 675, 112, 31);
    browser_.setBounds(68, 116, 1064, 520);
}
