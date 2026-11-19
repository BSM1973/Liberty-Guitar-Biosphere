#pragma once
#include <JuceHeader.h>
#include <functional>
#include <vector>
#include "BiosphereTheme.h"
#include "Localization.h"
#include "PresetLibrary.h"

namespace biosphere::ui
{
class PresetBrowser final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit PresetBrowser(PresetLibrary&);
    ~PresetBrowser() override;

    std::function<void()> onClose, onPresetLoaded, onLibraryChanged;

    void setLanguage(bool english);
    void refresh();
    void paint(juce::Graphics&) override;
    void resized() override;

private:
    int getNumRows() override { return static_cast<int>(visible_.size()); }
    void paintListBoxItem(int row, juce::Graphics&, int width, int height,
                          bool selected) override;
    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override;
    void selectedRowsChanged(int) override;

    int selectedIndex() const;
    void filter();
    void loadSelected();
    void saveCurrent();
    void toggleFavorite();
    void deleteSelected();

    PresetLibrary& library_;
    bool english_ = false, confirmDelete_ = false;
    std::vector<int> visible_;
    juce::Label title_, status_;
    juce::TextEditor search_, name_;
    juce::ComboBox category_;
    juce::ListBox list_;
    juce::TextButton close_ { "X" }, load_ { "Charger" }, save_ { "Enregistrer" },
        favorite_ { "☆" }, remove_ { "Supprimer" };
};
} // namespace biosphere::ui
