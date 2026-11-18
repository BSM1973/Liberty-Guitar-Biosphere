#pragma once

#include <JuceHeader.h>
#include <functional>
#include <array>
#include <memory>
#include "EngineNavigation.h"
#include "BiosphereTheme.h"

namespace biosphere::ui
{
class EcosystemView final : public juce::Component
{
public:
    EcosystemView();
    std::function<void(int)> onEngineSelected;

    void setSelectedEngine(int index);
    int getSelectedEngine() const noexcept { return selected_; }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Real JUCE buttons give the six icon/label regions mouse, keyboard and
    // accessibility semantics without painting over the approved visual design.
    class Hotspot final : public juce::TextButton
    {
    public:
        explicit Hotspot(const juce::String& title) : juce::TextButton(title) {}
        void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    };

    juce::Point<float> getNodeCentre(int index) const;
    juce::Rectangle<float> getNodeHitBox(int index) const;
    int selected_ = EngineNavigationState::defaultPage;
    std::array<std::unique_ptr<Hotspot>, EngineNavigationState::pageCount> hotspots_ {};
};

class AnalysisPanel final : public juce::Component
{
public:
    void paint(juce::Graphics&) override;
};
} // namespace biosphere::ui
