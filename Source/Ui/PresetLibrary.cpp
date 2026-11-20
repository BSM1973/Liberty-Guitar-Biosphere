#include "PresetLibrary.h"
#include "FactoryPresetCatalog.h"
#include <algorithm>

namespace biosphere::ui
{
namespace
{
juce::StringArray parameterIds()
{
    juce::StringArray ids {
        "input", "output", "mix", "pulse", "harmonic", "mutation", "memory",
        "rate", "root", "grain", "feedback",
        "global.intensity", "global.speed", "global.width",
        "global.sync", "global.freeze", "global.bypass",
        "macro.growth", "macro.motion", "macro.space", "macro.energy",
        "pulse.active", "pulse.sensitivity", "pulse.division", "pulse.sync",
        "pulse.swing", "pulse.feedback", "pulse.width", "pulse.mix",
        "harm.active", "harm.mode", "harm.resonance", "harm.decay",
        "harm.damping", "harm.motion", "harm.spread", "harm.mono",
        "mut.active", "mut.density", "mut.character", "mut.motion", "mut.pitch",
        "mut.random", "mut.stereo",
        "react.active", "react.reactivity", "react.interaction",
        "react.sensitivity", "react.response", "react.complexity",
        "mem.active", "mem.length", "mem.capture", "mem.recall",
        "mem.repeat", "mem.reverse", "mem.fade", "mem.freeze",
        "evo.active", "evo.rate", "evo.intensity", "evo.sensitivity",
        "evo.stability", "evo.complexity", "evo.recovery"
    };
    for (int route = 1; route <= 8; ++route)
    {
        const auto root = "react.route" + juce::String(route);
        for (const auto* suffix : { ".active", ".source", ".destination", ".amount" })
            ids.add(root + suffix);
    }
    return ids;
}
} // namespace

PresetLibrary::PresetLibrary(juce::AudioProcessorValueTreeState& state)
    : state_(state)
{
    favorites_.addLines(favoriteFile().loadFileAsString());
    refresh();
}

juce::File PresetLibrary::directory() const
{
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("Liberty Music Software").getChildFile("Guitar Biosphere")
        .getChildFile("Presets");
}

juce::File PresetLibrary::favoriteFile() const
{
    return directory().getChildFile("favorites.txt");
}

void PresetLibrary::refresh()
{
    presets_.clear();

    // One shared 100-patch catalog drives the JUCE preset browser AND
    // standalone DSP regression. Preset ordering/IDs 01..20 remain stable.
    const auto catalog = biosphere::factory::build();
    jassert(catalog.size() == 100);
    for (const auto& patch : catalog)
    {
        Preset item;
        item.name = juce::String::fromUTF8(patch.name.c_str());
        item.category = juce::String::fromUTF8(patch.category.c_str());
        for (const auto& [id, value] : patch.values)
            item.values.emplace_back(juce::String(id), value);
        presets_.push_back(std::move(item));
    }

    juce::Array<juce::File> files;
    directory().findChildFiles(files, juce::File::findFiles, false, "*.xml");
    for (const auto& file : files)
    {
        Preset item;
        item.name = file.getFileNameWithoutExtension();
        item.category = "USER";
        item.file = file;
        item.user = true;
        presets_.push_back(std::move(item));
    }
}

bool PresetLibrary::load(int index)
{
    lastError_.clear();
    if (index < 0 || index >= static_cast<int>(presets_.size()))
        return false;
    const auto preset = presets_[static_cast<std::size_t>(index)];
    if (preset.user)
    {
        auto xml = juce::XmlDocument::parse(preset.file);
        if (xml == nullptr || !xml->hasTagName(state_.state.getType()))
        {
            lastError_ = "Invalid preset XML";
            return false;
        }
        const auto restored = juce::ValueTree::fromXml(*xml);
        if (!restored.isValid())
        {
            lastError_ = "Invalid preset data";
            return false;
        }
        state_.replaceState(restored);
    }
    else
    {
        // An independently repeatable starting point on every factory load:
        // reset all 97 parameters to their declared JUCE defaults first.
        for (const auto& id : parameterIds())
            if (auto* parameter = state_.getParameter(id))
                parameter->setValueNotifyingHost(parameter->getDefaultValue());
        for (const auto& [id, value] : preset.values)
            if (auto* parameter = state_.getParameter(id))
                parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
    }
    state_.state.setProperty("currentPreset", preset.name, nullptr);
    return true;
}

bool PresetLibrary::saveUser(juce::String name)
{
    lastError_.clear();
    name = name.trim().substring(0, 80);
    if (name.isEmpty() || juce::File::createLegalFileName(name) != name)
    {
        lastError_ = "Invalid preset name";
        return false;
    }
    auto folder = directory();
    if (!folder.createDirectory().wasOk())
    {
        lastError_ = "Cannot create preset directory";
        return false;
    }
    const auto file = folder.getChildFile(name + ".xml");
    if (file.existsAsFile())
    {
        lastError_ = "This preset name already exists";
        return false;
    }
    auto xml = state_.copyState().createXml();
    if (!xml || !file.replaceWithText(xml->toString()))
    {
        lastError_ = "Cannot write preset file";
        return false;
    }
    state_.state.setProperty("currentPreset", name, nullptr);
    refresh();
    return true;
}

bool PresetLibrary::deleteUser(int index)
{
    lastError_.clear();
    if (index < 0 || index >= static_cast<int>(presets_.size()))
        return false;
    const auto preset = presets_[static_cast<std::size_t>(index)];
    if (!preset.user || !preset.file.deleteFile())
    {
        lastError_ = "Cannot delete factory preset or file";
        return false;
    }
    favorites_.removeString(preset.key());
    saveFavorites();
    refresh();
    return true;
}

bool PresetLibrary::toggleFavorite(int index)
{
    if (index < 0 || index >= static_cast<int>(presets_.size()))
        return false;
    const auto key = presets_[static_cast<std::size_t>(index)].key();
    if (favorites_.contains(key))
        favorites_.removeString(key);
    else
        favorites_.add(key);
    return saveFavorites();
}

bool PresetLibrary::isFavorite(int index) const
{
    return index >= 0 && index < static_cast<int>(presets_.size())
        && favorites_.contains(presets_[static_cast<std::size_t>(index)].key());
}

bool PresetLibrary::saveFavorites() const
{
    auto folder = directory();
    if (!folder.createDirectory().wasOk()) return false;
    return favoriteFile().replaceWithText(favorites_.joinIntoString("\n"));
}

juce::String PresetLibrary::currentName() const
{
    return state_.state.getProperty("currentPreset", "Init").toString();
}
} // namespace biosphere::ui
