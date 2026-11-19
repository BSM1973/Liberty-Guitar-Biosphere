#include "Ui/EcosystemLayout.h"
#include <array>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace biosphere::ui::ecosystem_layout;

void verify(float width, float height)
{
    const Rect viewport {0.0f, 0.0f, width, height};
    const auto state = hubDisk(width, height);
    const std::array<Rect, 4> chrome { title(), subtitle(), footer(height), input(height) };
    const std::array<const char*, 4> chromeNames { "title", "instruction", "footer", "input" };
    if (!viewport.contains(state))
        throw std::runtime_error("Central state out of bounds");
    for (std::size_t a = 0; a < chrome.size(); ++a)
        for (std::size_t b = a + 1; b < chrome.size(); ++b)
            if (chrome[a].overlaps(chrome[b]))
                throw std::runtime_error("Header/footer/input texts overlap");

    for (int i = 0; i < count; ++i)
    {
        const auto labelRect = label(i, width, height);
        const auto iconRect = disk(i, width, height);
        const auto click = hotspot(i, width, height);
        if (!viewport.contains(labelRect) || !viewport.contains(iconRect)
            || !viewport.contains(click))
            throw std::runtime_error("Engine " + std::to_string(i) + " outside viewport");
        if (!click.contains(iconRect) || !click.contains(labelRect))
            throw std::runtime_error("Clickable area does not include icon and label "
                                     + std::to_string(i));
        if (labelRect.overlaps(state) || iconRect.overlaps(state) || click.overlaps(state))
            throw std::runtime_error("Engine " + std::to_string(i)
                                     + " overlaps STATE / CALM");
        if (labelRect.overlaps(iconRect))
            throw std::runtime_error("Engine label overlaps its icon");
        for (std::size_t k = 0; k < chrome.size(); ++k)
        {
            if (labelRect.overlaps(chrome[k]) || click.overlaps(chrome[k]))
                throw std::runtime_error("Engine " + std::to_string(i)
                                         + " overlaps " + chromeNames[k]);
        }
        for (int j = i + 1; j < count; ++j)
        {
            if (labelRect.overlaps(label(j, width, height)))
                throw std::runtime_error("Two engine labels overlap");
            if (labelRect.overlaps(disk(j, width, height))
                || label(j, width, height).overlaps(iconRect))
                throw std::runtime_error("Engine label overlaps neighbouring icon");
            if (click.overlaps(hotspot(j, width, height)))
                throw std::runtime_error("Two clickable node regions overlap");
        }
    }

    // HARMONIC LIFE specifically: label must sit above its node, with space
    // between it and the central state caption for both FR and EN.
    if (!(label(1, width, height).y + label(1, width, height).h
          < disk(1, width, height).y))
        throw std::runtime_error("HARMONIC LIFE label is not above its icon");
    std::cout << "PASS " << width << " x " << height
              << ": 6 nodes, 6 labels, central state and controls do not overlap\n";
}

int main()
{
    // Approved native fixed-size 13-inch-friendly layout, plus proportional
    // host/UI DPI checks. Geometry lives in the same header used for drawing.
    for (const auto& size : std::array<std::array<float, 2>, 3> {{
            {{834.0f, 218.0f}}, {{834.0f, 230.0f}}, {{1000.0f, 250.0f}}
        }})
        verify(size[0], size[1]);
}
