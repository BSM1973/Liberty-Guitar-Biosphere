#pragma once
#include <cstddef>
#include <utility>
#include <vector>

namespace biosphere::ui
{
// Small JUCE-free timeline so actual preset state history can be regression
// tested on Linux, macOS and Windows. No history work runs in the audio thread.
template <class Snapshot>
class PresetHistory final
{
public:
    struct Entry
    {
        Snapshot snapshot;
        int slot = 0;
    };

    explicit PresetHistory(std::size_t limit = 64) : limit_(limit < 2 ? 2 : limit) {}

    void reset(Snapshot initial, int slot)
    {
        entries_.clear();
        entries_.push_back({std::move(initial), slot});
        cursor_ = 0;
    }

    void push(Snapshot snapshot, int slot)
    {
        if (entries_.empty()) { reset(std::move(snapshot), slot); return; }
        entries_.erase(entries_.begin() + static_cast<std::ptrdiff_t>(cursor_ + 1),
                       entries_.end());
        entries_.push_back({std::move(snapshot), slot});
        if (entries_.size() > limit_)
            entries_.erase(entries_.begin());
        cursor_ = entries_.size() - 1;
    }

    bool canUndo() const noexcept { return cursor_ > 0; }
    bool canRedo() const noexcept { return cursor_ + 1 < entries_.size(); }
    const Entry* undo() noexcept
    {
        if (!canUndo()) return nullptr;
        return &entries_[--cursor_];
    }
    const Entry* redo() noexcept
    {
        if (!canRedo()) return nullptr;
        return &entries_[++cursor_];
    }
    const Entry* current() const noexcept
    {
        return entries_.empty() ? nullptr : &entries_[cursor_];
    }
    std::size_t size() const noexcept { return entries_.size(); }

private:
    std::vector<Entry> entries_;
    std::size_t cursor_ = 0, limit_ = 64;
};
} // namespace biosphere::ui
