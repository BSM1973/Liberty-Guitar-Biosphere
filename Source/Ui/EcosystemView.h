#pragma once

#include <JuceHeader.h>
#include <functional>
#include <array>
#include <memory>
#include "EngineNavigation.h"
#include "BiosphereTheme.h"
#include "Localization.h"
#include "EcosystemLayout.h"

namespace biosphere::ui
{
class EcosystemView final : public juce::Component
{
public:
    EcosystemView();
    std::function<void(int)> onEngineSelected;

    void setSelectedEngine(int index);
    void setLanguage(bool english) { english_ = english; repaint(); }
    void setLivingState(int state, float energy);
    int getSelectedEngine() const noexcept { return selected_; }

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    // Real JUCE buttons give the six icon/label regions mouse, keyboard and
    // accessibility semantics without painting over the approved visual design.
    class Hotspot final : public juce::TextButton
    {
    public:
        explicit Hotspot(const juce::String& title, int engineIndex)
            : juce::TextButton(title), engineIndex_(engineIndex) {}
        private:
            int engineIndex_ = 0;
        public:
        void paintButton(juce::Graphics&, bool highlighted, bool down) override;
    };

    juce::Point<float> getNodeCentre(int index) const;
    juce::Rectangle<float> getNodeHitBox(int index) const;
    bool english_ = false;
    int livingState_ = 0;
    float livingEnergy_ = 0.0f;
    int selected_ = EngineNavigationState::defaultPage;
    std::array<std::unique_ptr<Hotspot>, EngineNavigationState::pageCount> hotspots_ {};
};

class AnalysisPanel final : public juce::Component
{
public:
    struct Metrics
    {
        float envelope = 0.0f, attacks = 0.0f, rhythm = 0.0f;
        float memory = 0.0f, tonal = 0.0f, rootHz = 0.0f;
    };
    void setValues(Metrics metrics) { metrics_ = metrics; repaint(); }
    void setLanguage(bool english) { english_ = english; repaint(); }
    void paint(juce::Graphics&) override;

private:
    Metrics metrics_;
    bool english_ = false;
};
} // namespace biosphere::ui
