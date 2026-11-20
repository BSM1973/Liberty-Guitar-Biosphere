#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include <array>
#include "PresetHistory.h"

namespace biosphere::ui
{
// Message-thread-only controller. Uses complete APVTS ValueTree snapshots for
// A/B comparisons and Undo/Redo; no audio-thread locks or GUI allocations
// are performed inside processBlock. A/B survives host session saving via a
// PRESET_LAB child, which is excluded from the snapshots to avoid recursion.
class PresetLabState final
{
public:
    explicit PresetLabState(juce::AudioProcessorValueTreeState& state) : state_(state)
    {
        const auto current = snapshot();
        slots_[0] = current.createCopy();
        slots_[1] = current.createCopy();

        const auto saved = state_.state.getChildWithName("PRESET_LAB");
        if (saved.isValid())
        {
            const int choice = static_cast<int>(saved.getProperty("active", 0));
            active_ = choice == 1 ? 1 : 0;
            for (int i = 0; i < 2; ++i)
            {
                auto holder = saved.getChildWithName(i == 0 ? "A" : "B");
                if (holder.isValid() && holder.getNumChildren() == 1)
                {
                    auto candidate = holder.getChild(0);
                    if (candidate.hasType(state_.state.getType()))
                        slots_[static_cast<std::size_t>(i)] = candidate.createCopy();
                }
            }
            // The host's top-level audio state is authoritative on recall.
            slots_[static_cast<std::size_t>(active_)] = current.createCopy();
        }
        history_.reset(current.createCopy(), active_);
        publish();
    }

    int activeSlot() const noexcept { return active_; }

    void tick()
    {
        const auto current = snapshot();
        const auto* saved = history_.current();
        if (saved && saved->snapshot.isEquivalentTo(current))
        {
            pendingFrames_ = 0;
            return;
        }
        if (++pendingFrames_ >= 6) // group knob gestures after ~200 ms
            commitNow();
    }

    bool hasPendingEdit() const
    {
        const auto* saved = history_.current();
        return saved && !saved->snapshot.isEquivalentTo(snapshot());
    }

    bool canUndo() const { return history_.canUndo() || hasPendingEdit(); }
    bool canRedo() const { return history_.canRedo() && !hasPendingEdit(); }

    void commitNow()
    {
        pendingFrames_ = 0;
        const auto current = snapshot();
        const auto* previous = history_.current();
        if (!previous || previous->slot != active_
            || !previous->snapshot.isEquivalentTo(current))
            history_.push(current.createCopy(), active_);
        slots_[static_cast<std::size_t>(active_)] = current.createCopy();
        publish();
    }

    void choose(int next)
    {
        if (next < 0 || next > 1 || next == active_) return;
        commitNow();
        // An edit on A doesn't mutate B; each slot has its own full snapshot.
        const auto recalled = slots_[static_cast<std::size_t>(next)].createCopy();
        state_.replaceState(recalled);
        active_ = next;
        history_.push(snapshot(), active_);
        publish();
    }

    bool undo()
    {
        commitNow(); // immediate Undo after a drag must still work
        const auto* earlier = history_.undo();
        if (!earlier) return false;
        restore(*earlier);
        return true;
    }

    bool redo()
    {
        // Do not force a new timeline entry when redoing an old snapshot.
        const auto* later = history_.redo();
        if (!later) return false;
        restore(*later);
        return true;
    }

private:
    juce::ValueTree snapshot() const
    {
        auto tree = state_.copyState();
        if (auto child = tree.getChildWithName("PRESET_LAB"); child.isValid())
            tree.removeChild(child, nullptr);
        return tree;
    }

    void restore(const PresetHistory<juce::ValueTree>::Entry& entry)
    {
        active_ = entry.slot;
        slots_[static_cast<std::size_t>(active_)] = entry.snapshot.createCopy();
        state_.replaceState(entry.snapshot.createCopy());
        pendingFrames_ = 0;
        publish();
    }

    void publish()
    {
        // ValueTree child persists A/B, but is NEVER copied inside A or B.
        auto tree = juce::ValueTree("PRESET_LAB");
        tree.setProperty("active", active_, nullptr);
        for (int i = 0; i < 2; ++i)
        {
            auto holder = juce::ValueTree(i == 0 ? "A" : "B");
            holder.addChild(slots_[static_cast<std::size_t>(i)].createCopy(),
                            -1, nullptr);
            tree.addChild(holder, -1, nullptr);
        }
        if (auto existing = state_.state.getChildWithName("PRESET_LAB");
            existing.isValid())
            state_.state.removeChild(existing, nullptr);
        state_.state.addChild(tree, -1, nullptr);
    }

    juce::AudioProcessorValueTreeState& state_;
    std::array<juce::ValueTree, 2> slots_ {};
    PresetHistory<juce::ValueTree> history_;
    int active_ = 0, pendingFrames_ = 0;
};
} // namespace biosphere::ui
