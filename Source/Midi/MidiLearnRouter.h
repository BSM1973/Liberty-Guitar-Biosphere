#pragma once
#include <array>
#include <atomic>
#include <cstddef>

namespace biosphere::midi
{
// No JUCE, locks or allocations inside receive(). CC events are transferred
// from audio thread to the message thread, where APVTS and hosts are notified.
// Each binding is a (MIDI channel 1-16, controller 0-127) -> parameter index.
class LearnRouter final
{
public:
    static constexpr int maximumParameters = 128;

    LearnRouter()
    {
        for (auto& value : assignments_) value.store(-1, std::memory_order_relaxed);
        for (auto& value : pending_) value.store(-1, std::memory_order_relaxed);
    }
    static constexpr int encode(int channel, int controller) noexcept
    {
        return channel >= 1 && channel <= 16 && controller >= 0 && controller < 128
            ? (channel << 7) | controller : -1;
    }
    static constexpr int channelOf(int encoded) noexcept { return encoded >> 7; }
    static constexpr int controllerOf(int encoded) noexcept { return encoded & 127; }

    void arm(int index) noexcept
    {
        if (index >= 0 && index < maximumParameters)
            learnTarget_.store(index, std::memory_order_release);
    }
    void cancel() noexcept { learnTarget_.store(-1, std::memory_order_release); }
    int learnTarget() const noexcept { return learnTarget_.load(std::memory_order_acquire); }

    // A CC can control one parameter at a time. Reassigning a CC removes
    // its previous owner, preventing accidental multi-parameter jumps.
    void assign(int index, int channel, int controller) noexcept
    {
        if (!valid(index)) return;
        const int code = encode(channel, controller);
        if (code < 0) { clear(index); return; }
        for (std::size_t i = 0; i < assignments_.size(); ++i)
            if (static_cast<int>(i) != index
                && assignments_[i].load(std::memory_order_relaxed) == code)
            {
                assignments_[i].store(-1, std::memory_order_release);
                pending_[i].store(-1, std::memory_order_release);
            }
        pending_[static_cast<std::size_t>(index)].store(-1, std::memory_order_release);
        assignments_[static_cast<std::size_t>(index)].store(code, std::memory_order_release);
    }
    void clear(int index) noexcept
    {
        if (valid(index))
        {
            assignments_[static_cast<std::size_t>(index)].store(-1, std::memory_order_release);
            pending_[static_cast<std::size_t>(index)].store(-1, std::memory_order_release);
        }
        if (learnTarget() == index) cancel();
    }
    void clearAll() noexcept
    {
        cancel();
        for (auto& a : assignments_) a.store(-1, std::memory_order_release);
        for (auto& p : pending_) p.store(-1, std::memory_order_release);
    }
    int assignment(int index) const noexcept
    {
        return valid(index)
            ? assignments_[static_cast<std::size_t>(index)].load(std::memory_order_acquire)
            : -1;
    }

    void receive(int channel, int controller, int controllerValue) noexcept
    {
        const int code = encode(channel, controller);
        if (code < 0 || controllerValue < 0 || controllerValue > 127) return;

        const int learn = learnTarget_.exchange(-1, std::memory_order_acq_rel);
        if (valid(learn))
        {
            assign(learn, channel, controller);
            // Last learned target recorded for status in editor.
            lastLearn_.store((learn << 12) | code, std::memory_order_release);
        }

        for (std::size_t i = 0; i < assignments_.size(); ++i)
            if (assignments_[i].load(std::memory_order_relaxed) == code)
                pending_[i].store(controllerValue, std::memory_order_release);
    }
    int consume(int index) noexcept
    {
        return valid(index)
            ? pending_[static_cast<std::size_t>(index)].exchange(-1, std::memory_order_acq_rel)
            : -1;
    }
    int consumeLearnNotification() noexcept
    {
        return lastLearn_.exchange(-1, std::memory_order_acq_rel);
    }
    static constexpr int learnedIndex(int notification) noexcept { return notification >> 12; }
    static constexpr int learnedBinding(int notification) noexcept { return notification & 4095; }

private:
    static constexpr bool valid(int index) noexcept
    { return index >= 0 && index < maximumParameters; }

    std::array<std::atomic<int>, maximumParameters> assignments_ {};
    std::array<std::atomic<int>, maximumParameters> pending_ {};
    std::atomic<int> learnTarget_ {-1};
    std::atomic<int> lastLearn_ {-1};
};
} // namespace biosphere::midi
