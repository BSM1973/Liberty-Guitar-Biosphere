#pragma once

#include <array>

namespace biosphere::ui
{
// The order matches the on-screen diagram (top row, then bottom row).
// It is also the only supported navigation mapping to the six JUCE panels.
enum class EnginePage : int
{
    pulse = 0,
    harmonicLife = 1,
    mutation = 2,
    evolution = 3,
    memory = 4,
    reaction = 5
};

inline constexpr std::array<const char*, 6> enginePageNames {
    "PULSE", "HARMONIC LIFE", "MUTATION", "EVOLUTION", "MEMORY", "REACTION"
};

class EngineNavigationState final
{
public:
    static constexpr int pageCount = static_cast<int>(enginePageNames.size());
    static constexpr int defaultPage = static_cast<int>(EnginePage::harmonicLife);

    // Reject invalid indices (rather than silently selecting the wrong engine).
    bool select(int index) noexcept
    {
        if (index < 0 || index >= pageCount)
            return false;
        current_ = index;
        return true;
    }

    int selected() const noexcept { return current_; }
    bool visible(int index) const noexcept { return index == current_; }

private:
    int current_ = defaultPage;
};
} // namespace biosphere::ui
