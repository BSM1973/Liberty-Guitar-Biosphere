#include "PresetLibrary.h"
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

void PresetLibrary::addFactory(const char* name, const char* category,
    std::initializer_list<std::pair<const char*, float>> changes)
{
    Preset item;
    item.name = juce::String::fromUTF8(name);
    item.category = category;
    for (const auto& value : changes)
        item.values.emplace_back(value.first, value.second);
    presets_.push_back(std::move(item));
}

void PresetLibrary::refresh()
{
    presets_.clear();

    // Real, immediately playable starting points. This is not yet the
    // 100-preset, listening-tested factory library planned for release.
    addFactory("01 - Pure Guitar", "ORGANIC CLEAN",
               {{"mix", 0.08f}, {"pulse", 0.0f}, {"harmonic", 0.08f},
                {"mutation", 0.0f}, {"memory", 0.0f}});
    addFactory("02 - Living Fingers", "ORGANIC CLEAN",
               {{"mix", 0.30f}, {"harmonic", 0.20f}, {"harm.decay", 1.0f},
                {"pulse", 0.12f}, {"macro.energy", 0.18f}});
    addFactory("03 - Quiet Halo", "LIVING AMBIENCE",
               {{"mix", 0.62f}, {"harmonic", 0.44f}, {"harm.decay", 4.7f},
                {"mutation", 0.18f}, {"macro.space", 0.85f}});
    addFactory("04 - Midnight Air", "LIVING AMBIENCE",
               {{"mix", 0.69f}, {"harmonic", 0.35f}, {"mutation", 0.28f},
                {"mut.character", 3.0f}, {"mut.density", 18.0f}});
    addFactory("05 - Soft Tremor", "RHYTHMIC PULSE",
               {{"mix", 0.65f}, {"pulse", 0.70f}, {"rate", 3.0f},
                {"pulse.swing", 0.10f}, {"pulse.mix", 0.85f}});
    addFactory("06 - Swung Sparks", "RHYTHMIC PULSE",
               {{"mix", 0.73f}, {"pulse", 0.88f}, {"rate", 4.2f},
                {"pulse.swing", 0.72f}, {"pulse.feedback", 0.45f}});
    addFactory("07 - Fifth Bloom", "HARMONIC BLOOM",
               {{"mix", 0.75f}, {"pulse", 0.0f}, {"harmonic", 0.82f},
                {"harm.mode", 0.0f}, {"harm.decay", 3.2f}});
    addFactory("08 - Golden Octaves", "HARMONIC BLOOM",
               {{"mix", 0.66f}, {"harmonic", 0.85f}, {"harm.mode", 1.0f},
                {"root", 147.0f}, {"macro.growth", 0.83f}});
    addFactory("09 - Crystal Seeds", "GRANULAR MUTATIONS",
               {{"mix", 0.82f}, {"mutation", 0.86f}, {"grain", 65.0f},
                {"mut.character", 1.0f}, {"mut.density", 25.0f}});
    addFactory("10 - Moving Metal", "GRANULAR MUTATIONS",
               {{"mix", 0.86f}, {"mutation", 0.82f}, {"mut.character", 2.0f},
                {"mut.pitch", -5.0f}, {"mut.motion", 0.75f}});
    addFactory("11 - Afterimage", "MEMORY GHOSTS",
               {{"mix", 0.82f}, {"memory", 0.82f}, {"mem.length", 0.6f},
                {"mem.recall", 0.91f}, {"feedback", 0.58f}});
    addFactory("12 - Reversed Shadows", "MEMORY GHOSTS",
               {{"mix", 0.88f}, {"memory", 0.85f}, {"mem.reverse", 0.95f},
                {"mem.fade", 120.0f}, {"mem.length", 1.2f}});
    addFactory("13 - Rising Lead", "REACTIVE LEADS",
               {{"mix", 0.56f}, {"harmonic", 0.64f}, {"harm.mode", 2.0f},
                {"macro.energy", 0.80f}, {"pulse.sensitivity", 0.82f}});
    addFactory("14 - Touch & Motion", "REACTIVE LEADS",
               {{"mix", 0.60f}, {"pulse", 0.47f}, {"mutation", 0.31f},
                {"macro.motion", 0.82f}, {"pulse.mix", 0.60f}});
    addFactory("15 - Wide Orbit", "CINEMATIC WORLDS",
               {{"mix", 0.87f}, {"harmonic", 0.60f}, {"mutation", 0.45f},
                {"global.width", 1.0f}, {"macro.space", 1.0f}});
    addFactory("16 - Distant Lights", "CINEMATIC WORLDS",
               {{"mix", 0.90f}, {"harmonic", 0.52f}, {"memory", 0.39f},
                {"harm.decay", 5.2f}, {"mem.fade", 160.0f}});
    addFactory("17 - Fractured Strings", "EXPERIMENTAL LIFE",
               {{"mix", 0.94f}, {"mutation", 0.94f}, {"mut.random", 0.88f},
                {"mut.pitch", 7.0f}, {"mut.density", 33.0f}});
    addFactory("18 - Broken Time", "EXPERIMENTAL LIFE",
               {{"mix", 0.93f}, {"pulse", 0.76f}, {"memory", 0.72f},
                {"rate", 7.0f}, {"mem.reverse", 0.69f}});
    addFactory("19 - Growing Ecosystem", "PERFORMANCE ECOSYSTEMS",
               {{"mix", 0.82f}, {"harmonic", 0.60f}, {"mutation", 0.48f},
                {"macro.growth", 0.94f}, {"macro.motion", 0.74f}});
    addFactory("20 - The Living Guitar", "PERFORMANCE ECOSYSTEMS",
               {{"mix", 0.84f}, {"pulse", 0.54f}, {"harmonic", 0.57f},
                {"mutation", 0.39f}, {"memory", 0.51f},
                {"macro.energy", 0.70f}, {"macro.space", 0.78f}});

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
