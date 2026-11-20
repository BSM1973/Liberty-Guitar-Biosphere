#include "PluginEditor.h"
#include <algorithm>

using namespace biosphere::ui;

// Keep the proven 1200 × 748 design grid. Scale the complete drawing AND
// all direct child components together, so labels, knobs, panels, mouse
// targets and preset browser remain aligned in the slightly larger editor.
namespace
{
constexpr int designWidth = 1200;
constexpr int designHeight = 748;
constexpr int editorWidth = 1284;
constexpr int editorHeight = 800;
}


GuitarBiosphereAudioProcessorEditor::GuitarBiosphereAudioProcessorEditor(
    GuitarBiosphereAudioProcessor& processor)
    : juce::AudioProcessorEditor(&processor),
      processor_(processor),
      presetLibrary_(processor.parameters), browser_(presetLibrary_),
      presetLab_(processor.parameters),
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
    preset_.setTooltip(juce::String::fromUTF8("Choisir un préréglage d'usine ou personnel."));
    preset_.onChange = [this] { loadCurrentPreset(); };
    refreshPresetSelector();

    for (auto* button : { &previous_, &next_, &library_, &french_, &english_ })
    {
        button->setColour(juce::TextButton::buttonColourId, raised);
        button->setColour(juce::TextButton::textColourOffId, white);
        addAndMakeVisible(button);
    }
    french_.setColour(juce::TextButton::buttonColourId, gold.darker(0.63f));
    french_.setColour(juce::TextButton::textColourOffId, gold);
    previous_.setTooltip(juce::String::fromUTF8("Préréglage précédent"));
    next_.setTooltip(juce::String::fromUTF8("Préréglage suivant"));
    library_.setTooltip(juce::String::fromUTF8("Ouvrir la bibliothèque, chercher et enregistrer des préréglages."));
    previous_.onClick = [this] { chooseRelativePreset(-1); };
    next_.onClick = [this] { chooseRelativePreset(1); };
    library_.onClick = [this] { openBrowser(); };

    // Compact PRESET LAB controls in the unused upper-right header row.
    // These do not overlap the preset selector or language buttons.
    for (auto* button : { &compareA_, &compareB_, &undo_, &redo_ })
    {
        button->setColour(juce::TextButton::buttonColourId, raised);
        button->setColour(juce::TextButton::textColourOffId, white);
        button->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible(button);
    }
    compareA_.onClick = [this]
    {
        presetLab_.choose(0);
        refreshPresetSelector();
        refreshPresetLabButtons();
    };
    compareB_.onClick = [this]
    {
        presetLab_.choose(1);
        refreshPresetSelector();
        refreshPresetLabButtons();
    };
    undo_.onClick = [this]
    {
        if (presetLab_.undo()) refreshPresetSelector();
        refreshPresetLabButtons();
    };
    redo_.onClick = [this]
    {
        if (presetLab_.redo()) refreshPresetSelector();
        refreshPresetLabButtons();
    };
    french_.onClick = [this] { setLanguage(false); };
    english_.onClick = [this] { setLanguage(true); };

    initialiseGlobalControl(0, "Croissance", "macro.growth", 0, 1, 0.35f, " %");
    initialiseGlobalControl(1, "Mouvement", "macro.motion", 0, 1, 0.35f, " %");
    initialiseGlobalControl(2, "Espace", "macro.space", 0, 1, 0.50f, " %");
    initialiseGlobalControl(3, juce::String::fromUTF8("Énergie"), "macro.energy", 0, 1, 0.35f, " %");
    initialiseGlobalControl(4, juce::String::fromUTF8("Entrée"), "input", -24, 18, 0, " dB");
    initialiseGlobalControl(5, "Sortie", "output", -24, 12, 0, " dB");
    initialiseGlobalControl(6, juce::String::fromUTF8("Mix maître"), "mix", 0, 1, 0.65f, " %");
    initialiseGlobalControl(7, juce::String::fromUTF8("Intensité globale"), "global.intensity", 0, 1, 0.50f, " %");
    initialiseGlobalControl(8, "Vitesse globale", "global.speed", 0.25f, 4.0f, 1.0f, " x");
    initialiseGlobalControl(9, juce::String::fromUTF8("Largeur stéréo"), "global.width", 0, 1, 0.80f, " %");

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

    sync_.setTooltip(juce::String::fromUTF8("Synchronisation PULSE au tempo du DAW, lorsque le tempo hôte est disponible."));
    freeze_.setTooltip(juce::String::fromUTF8("Fige le tampon MEMORY et conserve les fragments capturés."));
    bypass_.setTooltip("Contourne le traitement avec un fondu court vers la guitare brute.");
    sync_.setComponentID("global.sync");
    freeze_.setComponentID("global.freeze");
    bypass_.setComponentID("global.bypass");
    syncAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, "global.sync", sync_);
    freezeAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, "global.freeze", freeze_);
    bypassAttachment_ = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processor_.parameters, "global.bypass", bypass_);

    browser_.onClose = [this] { browser_.setVisible(false); };
    browser_.onBeforePresetLoad = [this] { presetLab_.commitNow(); };
    browser_.onPresetLoaded = [this]
    {
        presetLab_.commitNow();
        refreshPresetSelector();
        refreshPresetLabButtons();
    };
    browser_.onLibraryChanged = [this]
    {
        presetLab_.commitNow();
        refreshPresetSelector();
        refreshPresetLabButtons();
    };
    addAndMakeVisible(browser_);
    browser_.setVisible(false);

    selectEngine(processor_.getSelectedEditorPage());
    setResizable(false, false);
    // The footer is intentionally empty. MIDI Learn remains accessible
    // from the contextual menu, but no status label is rendered below.
    // Listener sees the original mouse event on all descendants, including
    // hidden engine panels when displayed. Parameter IDs are JUCE component
    // IDs, not inferred from captions or translated labels.
    addMouseListener(this, true);
    // Create the window once, after all child components exist, so resized()
    // can apply the same scaling transform to every interactive child.
    setSize(editorWidth, editorHeight);
    setLanguage(processor_.isEnglish());
    refreshPresetLabButtons();
    updateMidiStatus();
    startTimerHz(30);
}

GuitarBiosphereAudioProcessorEditor::~GuitarBiosphereAudioProcessorEditor()
{
    stopTimer();
    removeMouseListener(this);
    setLookAndFeel(nullptr);
}

void GuitarBiosphereAudioProcessorEditor::timerCallback()
{
    // Message thread only. Coalesce parameter knob edits into a 200 ms
    // Undo step, rather than recording a step for every automation sample.
    presetLab_.tick();
    refreshPresetLabButtons();
    const int learned = processor_.consumeMidiLearnNotification();
    if (learned >= 0)
    {
        lastLearnBinding_ = biosphere::midi::LearnRouter::learnedBinding(learned);
        updateMidiStatus();
    }
    else if (processor_.midiLearnTarget() >= 0)
        updateMidiStatus();
    const auto& m = processor_.analysis;
    analysis_.setValues({
        m.envelope.load(std::memory_order_relaxed),
        m.attacks.load(std::memory_order_relaxed),
        m.rhythmicActivity.load(std::memory_order_relaxed),
        m.memoryActivity.load(std::memory_order_relaxed),
        m.tonalEnergy.load(std::memory_order_relaxed),
        m.rootHz.load(std::memory_order_relaxed),
        m.pitchConfidence.load(std::memory_order_relaxed),
        m.midiNote.load(std::memory_order_relaxed)
    });
    ecosystem_.setLivingState(
        m.evolutionState.load(std::memory_order_relaxed),
        m.evolutionEnergy.load(std::memory_order_relaxed));
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
        juce::String::fromUTF8("Entrée"), "Sortie", juce::String::fromUTF8("Mix maître"),
        juce::String::fromUTF8("Intensité globale"), "Vitesse globale",
        juce::String::fromUTF8("Largeur stéréo")
    };
    for (std::size_t i = 0; i < names.size(); ++i)
        globals_[i].label.setText(tr(names[i], english), juce::dontSendNotification);
    sync_.setButtonText(tr("Synchro hôte", english));
    freeze_.setButtonText(tr("Gel global", english));
    bypass_.setButtonText(tr("Bypass global", english));
    preset_.setTooltip(english ? "Choose a factory or user preset."
                               : juce::String::fromUTF8("Choisir un préréglage d'usine ou personnel."));
    previous_.setTooltip(english ? "Previous preset" : juce::String::fromUTF8("Préréglage précédent"));
    next_.setTooltip(english ? "Next preset" : juce::String::fromUTF8("Préréglage suivant"));
    library_.setTooltip(english ? "Open, search and save presets."
                                : juce::String::fromUTF8("Ouvrir, rechercher et enregistrer des préréglages."));
    french_.setColour(juce::TextButton::buttonColourId,
                      english ? raised : gold.darker(0.60f));
    english_.setColour(juce::TextButton::buttonColourId,
                       english ? gold.darker(0.60f) : raised);
    french_.setColour(juce::TextButton::textColourOffId, english ? white : gold);
    english_.setColour(juce::TextButton::textColourOffId, english ? gold : white);
    undo_.setButtonText(english ? "UNDO" : "ANNULER");
    redo_.setButtonText(english ? "REDO" : "REFAIRE");
    compareA_.setTooltip(english ? "Recall comparison A" : "Rappeler la comparaison A");
    compareB_.setTooltip(english ? "Recall comparison B" : "Rappeler la comparaison B");
    undo_.setTooltip(english ? "Undo last preset or parameter edit"
                             : juce::String::fromUTF8("Annuler la modification précédente"));
    redo_.setTooltip(english ? "Redo last undone edit"
                             : juce::String::fromUTF8("Rétablir la modification annulée"));
    refreshPresetLabButtons();
    updateMidiStatus();
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

void GuitarBiosphereAudioProcessorEditor::loadCurrentPreset()
{
    if (preset_.getSelectedId() <= 1) return;
    presetLab_.commitNow();
    if (presetLibrary_.load(preset_.getSelectedId() - 2))
        presetLab_.commitNow();
    refreshPresetSelector();
    refreshPresetLabButtons();
}

void GuitarBiosphereAudioProcessorEditor::refreshPresetLabButtons()
{
    const bool isA = presetLab_.activeSlot() == 0;
    compareA_.setColour(juce::TextButton::buttonColourId,
                        isA ? gold.darker(0.55f) : raised);
    compareB_.setColour(juce::TextButton::buttonColourId,
                        isA ? raised : gold.darker(0.55f));
    compareA_.setColour(juce::TextButton::textColourOffId, isA ? gold : white);
    compareB_.setColour(juce::TextButton::textColourOffId, isA ? white : gold);
    undo_.setEnabled(presetLab_.canUndo());
    redo_.setEnabled(presetLab_.canRedo());
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
    control.slider.setComponentID(parameterId);
    addAndMakeVisible(control.slider);

    jassert(processor_.parameters.getParameter(parameterId) != nullptr);
    control.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processor_.parameters, parameterId, control.slider);
    control.slider.setTooltip(juce::String::fromUTF8("Automatisation et mémorisation APVTS : ") + parameterId);
}

void GuitarBiosphereAudioProcessorEditor::mouseDown(const juce::MouseEvent& event)
{
    if (!event.mods.isPopupMenu()) return;
    auto* component = event.originalComponent;
    while (component != nullptr && component != this)
    {
        const auto id = component->getComponentID();
        if (id.isNotEmpty() && processor_.parameters.getParameter(id) != nullptr)
        {
            showMidiMenu(*component, id);
            return;
        }
        component = component->getParentComponent();
    }
}

void GuitarBiosphereAudioProcessorEditor::showMidiMenu(
    juce::Component& component, const juce::String& id)
{
    const bool english = processor_.isEnglish();
    const int mapped = processor_.midiAssignment(id);
    juce::PopupMenu menu;
    menu.addItem(1, english ? "MIDI Learn: next CC"
                            : juce::String::fromUTF8("Apprendre le prochain CC MIDI"));
    if (mapped >= 0)
    {
        const juce::String name = "CC " + juce::String(
            biosphere::midi::LearnRouter::controllerOf(mapped)) + " / "
            + (english ? "Channel " : "Canal ")
            + juce::String(biosphere::midi::LearnRouter::channelOf(mapped));
        menu.addSectionHeader(name);
    }
    menu.addItem(2, english ? "Remove this MIDI assignment"
                            : juce::String::fromUTF8("Effacer cette affectation MIDI"),
                 mapped >= 0);
    menu.addItem(4, english ? "Cancel MIDI Learn"
                             : juce::String::fromUTF8("Annuler l'apprentissage MIDI"),
                 processor_.midiLearnTarget() >= 0);
    menu.addSeparator();
    menu.addItem(3, english ? "Clear all MIDI assignments"
                            : juce::String::fromUTF8("Effacer toutes les affectations MIDI"));
    const juce::Component::SafePointer<GuitarBiosphereAudioProcessorEditor> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options().withTargetComponent(&component),
        [safe, id](int chosen)
        {
            if (safe == nullptr) return;
            if (chosen == 1) safe->processor_.armMidiLearn(id);
            else if (chosen == 2) safe->processor_.clearMidiLearn(id);
            else if (chosen == 3) safe->processor_.clearAllMidiLearn();
            else if (chosen == 4) safe->processor_.cancelMidiLearn();
            if (chosen > 0) safe->lastLearnBinding_ = -1;
            safe->updateMidiStatus();
        });
}

void GuitarBiosphereAudioProcessorEditor::updateMidiStatus()
{
    const bool english = processor_.isEnglish();
    const int target = processor_.midiLearnTarget();
    if (target >= 0)
    {
        midiLearnStatus_.setText(
            (english ? "MIDI: waiting for CC" : "MIDI : attendre un CC"),
            juce::dontSendNotification);
    }
    else if (lastLearnBinding_ >= 0)
    {
        const int cc = biosphere::midi::LearnRouter::controllerOf(lastLearnBinding_);
        const int channel = biosphere::midi::LearnRouter::channelOf(lastLearnBinding_);
        midiLearnStatus_.setText("MIDI : CC " + juce::String(cc)
            + (english ? " / Ch " : " / Canal ") + juce::String(channel),
            juce::dontSendNotification);
    }
    else
    {
        midiLearnStatus_.setText(english ? "Right-click: MIDI Learn"
                                        : "Clic droit : MIDI Learn",
                                 juce::dontSendNotification);
    }
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
    // Draw the original vetted composition in design coordinates and scale
    // it to the new size. Text and decorations keep their relative spacing.
    g.addTransform(juce::AffineTransform::scale(
        static_cast<float>(getWidth()) / static_cast<float>(designWidth),
        static_cast<float>(getHeight()) / static_cast<float>(designHeight)));
    juce::ColourGradient backdrop(panel.brighter(0.05f), 0.0f, 0.0f,
                                  navy, static_cast<float>(designWidth),
                                  static_cast<float>(designHeight), false);
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
                   : "- BPM", 962, 21, 90, 20, juce::Justification::left);

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
    g.drawLine(29.0f, 102.0f, static_cast<float>(designWidth - 29), 102.0f, 1.0f);

    roundedPanel(g, { 28.0f, 577.0f, 1144.0f, 142.0f });
    heading(g, tr("MACROS DE PERFORMANCE", processor_.isEnglish()), { 47, 584, 390, 19 }, 11.0f);
    heading(g, tr(juce::String::fromUTF8("BUS MAÎTRE"), processor_.isEnglish()), { 452, 584, 500, 19 }, 11.0f);
    g.setColour(border);
    g.drawLine(443.0f, 596.0f, 443.0f, 705.0f, 1.0f);

    // Empty footer by design: no version, MIDI hint or decorative text.
}

void GuitarBiosphereAudioProcessorEditor::resized()
{
    preset_.setBounds(567, 42, 244, 33);
    previous_.setBounds(821, 42, 35, 33);
    next_.setBounds(861, 42, 35, 33);
    library_.setBounds(901, 42, 40, 33);
    // The PRESET heading occupies x567..752 at y19..36.
    // A/B and Undo/Redo sit x756..940 without intersection.
    compareA_.setBounds(756, 17, 26, 20);
    compareB_.setBounds(788, 17, 26, 20);
    undo_.setBounds(821, 17, 55, 20);
    redo_.setBounds(883, 17, 57, 20);
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

    // Component transforms are relative to their parent's origin in JUCE.
    // Applying the same factor used in paint() scales every child position,
    // font, hit area and nested panel without changing its internal layout.
    const auto transform = juce::AffineTransform::scale(
        static_cast<float>(getWidth()) / static_cast<float>(designWidth),
        static_cast<float>(getHeight()) / static_cast<float>(designHeight));
    for (int i = 0; i < getNumChildComponents(); ++i)
        if (auto* child = getChildComponent(i))
            child->setTransform(transform);
}
