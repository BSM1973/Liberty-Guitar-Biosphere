#pragma once

#include <JuceHeader.h>
#include <functional>
#include "BiosphereTheme.h"

namespace biosphere::ui
{
class EcosystemView final : public juce::Component
{
public:
    std::function<void(int)> onEngineSelected;

    void setSelectedEngine(int index) { selected_ = juce::jlimit(0, 5, index); repaint(); }
    int getSelectedEngine() const noexcept { return selected_; }

    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;

private:
    juce::Point<float> getNodeCentre(int index) const;
    juce::Rectangle<float> getNodeHitBox(int index) const;
    int selected_ = 1; // Reference screen opens at HARMONIC LIFE.
};

class AnalysisPanel final : public juce::Component
{
public:
    void paint(juce::Graphics&) override;
};
} // namespace biosphere::ui
