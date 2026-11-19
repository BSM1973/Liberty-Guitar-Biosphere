#pragma once
#include <array>
#include <cstddef>

namespace biosphere::ui::ecosystem_layout
{
// Shared, JUCE-free rectangles used by both the real editor and CI geometry
// checks. Decorative background orbits may cross, but readable labels,
// button hotspots and the central state must never overlap.
struct Point { float x = 0.0f, y = 0.0f; };
struct Rect
{
    float x = 0.0f, y = 0.0f, w = 0.0f, h = 0.0f;
    constexpr bool overlaps(Rect other, float gap = 0.0f) const noexcept
    {
        return x < other.x + other.w + gap && x + w + gap > other.x
            && y < other.y + other.h + gap && y + h + gap > other.y;
    }
    constexpr bool contains(Rect other) const noexcept
    {
        return other.x >= x && other.y >= y
            && other.x + other.w <= x + w && other.y + other.h <= y + h;
    }
};

inline constexpr std::array<float, 6> xFractions {
    0.305f, 0.525f, 0.745f, 0.305f, 0.525f, 0.745f
};
inline constexpr int count = 6;
inline constexpr float nodeRadius = 19.0f;
inline constexpr float centralRadius = 32.0f;

// Restrict layout to the approved fixed-size widget. All units below are
// relative to EcosystemView's local origin (not the plugin window).
inline Point node(int index, float width, float height) noexcept
{
    if (index < 0 || index >= count) return {};
    return {width * xFractions[static_cast<std::size_t>(index)],
            height * (index < 3 ? 0.27f : 0.78f)};
}
inline Point hub(float width, float height) noexcept
{
    return { width * 0.525f, height * 0.52f };
}
inline Rect disk(int index, float width, float height) noexcept
{
    const auto p = node(index, width, height);
    return {p.x - nodeRadius, p.y - nodeRadius,
            nodeRadius * 2.0f, nodeRadius * 2.0f};
}
inline Rect hubDisk(float width, float height) noexcept
{
    const auto p = hub(width, height);
    return {p.x - centralRadius, p.y - centralRadius,
            centralRadius * 2.0f, centralRadius * 2.0f};
}
inline Rect label(int index, float width, float height) noexcept
{
    const auto p = node(index, width, height);
    // HARMONIC LIFE is intentionally above its icon; the former lower
    // label obscured ÉTAT VIVANT / CALME. 9 pt text fits in this wide box.
    if (index == 1)
        return {p.x - 74.0f, p.y - 42.0f, 148.0f, 18.0f};
    return {p.x - 80.0f, p.y + 26.0f, 160.0f, 18.0f};
}
inline Rect hotspot(int index, float width, float height) noexcept
{
    const auto p = node(index, width, height);
    if (index == 1)
        return {p.x - 82.0f, p.y - 45.0f, 164.0f, 65.0f};
    return {p.x - 82.0f, p.y - 19.0f, 164.0f, 63.0f};
}
inline Rect title() noexcept { return {20.0f, 13.0f, 220.0f, 21.0f}; }
inline Rect subtitle() noexcept { return {20.0f, 36.0f, 150.0f, 17.0f}; }
inline Rect footer(float height) noexcept
{
    return {20.0f, height - 24.0f, 145.0f, 17.0f};
}
inline Rect input(float height) noexcept
{
    return {27.0f, height * 0.46f, 86.0f, 29.0f};
}
} // namespace biosphere::ui::ecosystem_layout
