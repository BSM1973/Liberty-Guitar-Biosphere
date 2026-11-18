#include "Ui/EngineNavigation.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

int main()
{
    using biosphere::ui::EngineNavigationState;
    using biosphere::ui::enginePageNames;
    constexpr std::array<const char*, 6> expected {
        "PULSE", "HARMONIC LIFE", "MUTATION", "EVOLUTION", "MEMORY", "REACTION"
    };
    static_assert(EngineNavigationState::pageCount == 6);
    EngineNavigationState navigation;
    if (navigation.selected() != 1)
        throw std::runtime_error("Incorrect initial panel");

    // The exact on-screen node order is deliberate, including the bottom row.
    for (int index = 0; index < 6; ++index)
    {
        if (std::string(enginePageNames[index]) != expected[index])
            throw std::runtime_error("Node name/order mismatch");
        if (!navigation.select(index) || navigation.selected() != index)
            throw std::runtime_error("Click failed to select requested panel");

        int visible = 0;
        for (int panel = 0; panel < 6; ++panel)
            visible += navigation.visible(panel) ? 1 : 0;
        if (visible != 1)
            throw std::runtime_error("Multiple or no panels visible");
        if (!navigation.visible(index))
            throw std::runtime_error("Visible panel differs from selected node");
    }

    if (navigation.select(-1) || navigation.select(6) || navigation.select(123))
        throw std::runtime_error("Invalid index accepted");
    if (navigation.selected() != 5)
        throw std::runtime_error("Invalid index changed current selection");

    // Clicking repeatedly must be idempotent, independent of engine parameters.
    if (!navigation.select(5) || !navigation.visible(5))
        throw std::runtime_error("Repeated selection lost the active panel");
    std::cout << "Navigation: PASS (6 nodes, 6 panels, exclusive selection, invalid-index safety)\n";
    return 0;
}
