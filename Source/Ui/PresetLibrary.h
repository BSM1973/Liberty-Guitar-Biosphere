#pragma once
#include <JuceHeader.h>
#include <vector>
#include <utility>

namespace biosphere::ui
{
struct Preset
{
    juce::String name, category;
    bool user = false;
    juce::File file;
    std::vector<std::pair<juce::String, float>> values;

    juce::String key() const { return (user ? "U:" : "F:") + name; }
};

class PresetLibrary final
{
public:
    explicit PresetLibrary(juce::AudioProcessorValueTreeState& parameters);

    const std::vector<Preset>& all() const { return presets_; }
    void refresh();
    bool load(int index);
    bool saveUser(juce::String name);
    bool deleteUser(int index);
    bool toggleFavorite(int index);
    bool isFavorite(int index) const;
    juce::String currentName() const;
    juce::String error() const { return lastError_; }

private:
    juce::File directory() const;
    juce::File favoriteFile() const;
    bool saveFavorites() const;

    juce::AudioProcessorValueTreeState& state_;
    std::vector<Preset> presets_;
    juce::StringArray favorites_;
    juce::String lastError_;
};
} // namespace biosphere::ui
