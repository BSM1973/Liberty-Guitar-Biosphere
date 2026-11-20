#include "PresetBrowser.h"
#include <array>

namespace biosphere::ui
{
namespace
{
constexpr std::array<const char*, 10> categories {
    "ORGANIC CLEAN", "LIVING AMBIENCE", "RHYTHMIC PULSE",
    "HARMONIC BLOOM", "GRANULAR MUTATIONS", "MEMORY GHOSTS",
    "REACTIVE LEADS", "CINEMATIC WORLDS", "EXPERIMENTAL LIFE",
    "PERFORMANCE ECOSYSTEMS"
};
}

PresetBrowser::PresetBrowser(PresetLibrary& library) : library_(library)
{
    setName("Guitar Biosphere Preset Library");

    for (auto* button : { &close_, &load_, &save_, &favorite_, &remove_ })
    {
        button->setColour(juce::TextButton::buttonColourId, raised);
        button->setColour(juce::TextButton::textColourOffId, white);
        button->setMouseCursor(juce::MouseCursor::PointingHandCursor);
        addAndMakeVisible(button);
    }
    load_.setColour(juce::TextButton::buttonColourId, gold.darker(0.60f));
    favorite_.setColour(juce::TextButton::textColourOffId, gold);

    title_.setJustificationType(juce::Justification::centredLeft);
    title_.setColour(juce::Label::textColourId, gold);
    title_.setFont(juce::Font(juce::FontOptions(20.0f, juce::Font::bold)));
    status_.setColour(juce::Label::textColourId, muted);
    status_.setFont(juce::Font(juce::FontOptions(11.0f)));
    addAndMakeVisible(title_);
    addAndMakeVisible(status_);

    for (auto* editor : { &search_, &name_ })
    {
        editor->setColour(juce::TextEditor::backgroundColourId, raised);
        editor->setColour(juce::TextEditor::outlineColourId, border);
        editor->setColour(juce::TextEditor::textColourId, white);
        editor->setColour(juce::TextEditor::highlightColourId, gold.withAlpha(0.4f));
        addAndMakeVisible(editor);
    }
    search_.onTextChange = [this] { filter(); };
    name_.setInputRestrictions(80);

    category_.setColour(juce::ComboBox::backgroundColourId, raised);
    category_.setColour(juce::ComboBox::outlineColourId, border);
    category_.setColour(juce::ComboBox::textColourId, white);
    category_.onChange = [this] { filter(); };
    addAndMakeVisible(category_);

    list_.setModel(this);
    list_.setRowHeight(33);
    list_.setOutlineThickness(0);
    list_.setColour(juce::ListBox::backgroundColourId, panel);
    addAndMakeVisible(list_);

    close_.onClick = [this] { if (onClose) onClose(); };
    load_.onClick = [this] { loadSelected(); };
    favorite_.onClick = [this] { toggleFavorite(); };
    save_.onClick = [this] { saveCurrent(); };
    remove_.onClick = [this] { deleteSelected(); };

    setLanguage(false);
    filter();
}

PresetBrowser::~PresetBrowser()
{
    list_.setModel(nullptr);
}

void PresetBrowser::setLanguage(bool english)
{
    english_ = english;
    title_.setText(tr(juce::String::fromUTF8("BIBLIOTHÈQUE DE PRÉRÉGLAGES"), english_),
                   juce::dontSendNotification);
    close_.setButtonText(tr("Fermer", english_));
    load_.setButtonText(tr("Charger", english_));
    save_.setButtonText(tr("Enregistrer", english_));
    remove_.setButtonText(tr("Supprimer", english_));
    search_.setTextToShowWhenEmpty(tr("Rechercher un son...", english_), muted);
    name_.setTextToShowWhenEmpty(tr("Nom du préréglage", english_), muted);

    const int selected = juce::jmax(1, category_.getSelectedId());
    category_.clear(juce::dontSendNotification);
    category_.addItem(tr("Toutes les catégories", english_), 1);
    category_.addItem(tr("Favoris", english_), 2);
    category_.addItem(tr("Mes préréglages", english_), 3);
    for (int index = 0; index < static_cast<int>(categories.size()); ++index)
        category_.addItem(categories[static_cast<std::size_t>(index)], index + 4);
    category_.setSelectedId(selected, juce::dontSendNotification);
    filter();
}

void PresetBrowser::refresh()
{
    library_.refresh();
    filter();
}

int PresetBrowser::selectedIndex() const
{
    const int row = list_.getSelectedRow();
    if (row < 0 || row >= static_cast<int>(visible_.size()))
        return -1;
    return visible_[static_cast<std::size_t>(row)];
}

void PresetBrowser::filter()
{
    confirmDelete_ = false;
    remove_.setButtonText(tr("Supprimer", english_));
    const auto searchText = search_.getText().trim();
    visible_.clear();
    const auto& all = library_.all();
    const int group = category_.getSelectedId();
    for (int index = 0; index < static_cast<int>(all.size()); ++index)
    {
        const auto& preset = all[static_cast<std::size_t>(index)];
        if (searchText.isNotEmpty()
            && !preset.name.containsIgnoreCase(searchText)
            && !preset.category.containsIgnoreCase(searchText))
            continue;
        if (group == 2 && !library_.isFavorite(index)) continue;
        if (group == 3 && !preset.user) continue;
        if (group >= 4 && preset.category != categories[static_cast<std::size_t>(group - 4)])
            continue;
        visible_.push_back(index);
    }
    list_.updateContent();
    if (!visible_.empty())
        list_.selectRow(0);
    else
        list_.deselectAllRows();
    selectedRowsChanged(list_.getSelectedRow());
}

void PresetBrowser::paintListBoxItem(int row, juce::Graphics& g, int width, int height,
                                      bool selected)
{
    if (row < 0 || row >= static_cast<int>(visible_.size())) return;
    const int index = visible_[static_cast<std::size_t>(row)];
    const auto& preset = library_.all()[static_cast<std::size_t>(index)];
    if (selected)
    {
        g.setColour(raised.brighter(0.25f));
        g.fillRoundedRectangle(2.0f, 2.0f, static_cast<float>(width - 4),
                               static_cast<float>(height - 4), 5.0f);
    }
    g.setColour(library_.isFavorite(index) ? gold : muted);
    g.setFont(juce::Font(juce::FontOptions(14.0f)));
    g.drawText(library_.isFavorite(index) ? juce::String("F")
                                        : juce::String("-"),
               13, 0, 28, height, juce::Justification::centred);
    g.setColour(selected ? gold : white);
    g.setFont(juce::Font(juce::FontOptions(12.3f)));
    g.drawFittedText(preset.name, 47, 0, juce::jmax(80, width - 260), height,
                     juce::Justification::centredLeft, 1);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.0f)));
    g.drawFittedText(preset.user ? tr("Mes préréglages", english_) : preset.category,
                     width - 208, 0, 190, height, juce::Justification::centredRight, 1);
}

void PresetBrowser::listBoxItemDoubleClicked(int, const juce::MouseEvent&)
{
    loadSelected();
}

void PresetBrowser::selectedRowsChanged(int)
{
    const int index = selectedIndex();
    const bool hasSelection = index >= 0;
    load_.setEnabled(hasSelection);
    favorite_.setEnabled(hasSelection);
    remove_.setEnabled(hasSelection && library_.all()[static_cast<std::size_t>(index)].user);
    favorite_.setButtonText(hasSelection && library_.isFavorite(index)
                                ? juce::String("F")
                                : juce::String("-"));
    confirmDelete_ = false;
    remove_.setButtonText(tr("Supprimer", english_));
    status_.setText(juce::String(visible_.size()) + (english_ ? juce::String(" results") : juce::String::fromUTF8(" résultats")),
                    juce::dontSendNotification);
}

void PresetBrowser::loadSelected()
{
    const int index = selectedIndex();
    if (index < 0) return;
    if (onBeforePresetLoad) onBeforePresetLoad();
    if (!library_.load(index))
    {
        status_.setText(library_.error(), juce::dontSendNotification);
        return;
    }
    if (onPresetLoaded) onPresetLoaded();
    if (onClose) onClose();
}

void PresetBrowser::saveCurrent()
{
    if (!library_.saveUser(name_.getText()))
    {
        status_.setText(library_.error(), juce::dontSendNotification);
        return;
    }
    if (onLibraryChanged) onLibraryChanged();
    filter();
    status_.setText(tr("Préréglage enregistré", english_), juce::dontSendNotification);
}

void PresetBrowser::toggleFavorite()
{
    const int index = selectedIndex();
    if (index < 0) return;
    if (library_.toggleFavorite(index))
    {
        if (category_.getSelectedId() == 2) filter();
        else { list_.repaint(); selectedRowsChanged(list_.getSelectedRow()); }
    }
}

void PresetBrowser::deleteSelected()
{
    const int index = selectedIndex();
    if (index < 0 || !library_.all()[static_cast<std::size_t>(index)].user) return;
    if (!confirmDelete_)
    {
        confirmDelete_ = true;
        remove_.setButtonText(english_ ? "Confirm delete" : "Confirmer ?");
        return;
    }
    if (!library_.deleteUser(index))
    {
        status_.setText(library_.error(), juce::dontSendNotification);
        return;
    }
    if (onLibraryChanged) onLibraryChanged();
    filter();
    status_.setText(tr("Préréglage supprimé", english_), juce::dontSendNotification);
}

void PresetBrowser::paint(juce::Graphics& g)
{
    g.fillAll(navy.withAlpha(0.99f));
    roundedPanel(g, getLocalBounds().toFloat().reduced(2.0f), 13.0f, gold.withAlpha(0.82f));
    g.setColour(gold.withAlpha(0.5f));
    g.drawLine(25.0f, 57.0f, static_cast<float>(getWidth() - 25), 57.0f, 0.8f);
    g.setColour(muted);
    g.setFont(juce::Font(juce::FontOptions(10.5f)));
    g.drawText(english_ ? "100 factory sounds + your own presets"
                        : juce::String::fromUTF8("100 sons d'usine + vos préréglages"),
               24, 64, getWidth() - 48, 17, juce::Justification::left);
}

void PresetBrowser::resized()
{
    const auto width = getWidth();
    title_.setBounds(24, 13, width - 135, 37);
    close_.setBounds(width - 101, 14, 77, 31);
    search_.setBounds(24, 94, width / 2 - 31, 32);
    category_.setBounds(width / 2 + 6, 94, width / 2 - 30, 32);
    list_.setBounds(24, 139, width - 48, getHeight() - 237);
    name_.setBounds(24, getHeight() - 82, width / 2 - 31, 32);
    save_.setBounds(width / 2 + 6, getHeight() - 82, 115, 32);
    favorite_.setBounds(width / 2 + 129, getHeight() - 82, 53, 32);
    remove_.setBounds(width / 2 + 190, getHeight() - 82, 115, 32);
    load_.setBounds(width - 139, getHeight() - 82, 115, 32);
    status_.setBounds(24, getHeight() - 40, width - 48, 23);
}
} // namespace biosphere::ui
