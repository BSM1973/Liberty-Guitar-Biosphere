#include "Ui/PresetHistory.h"
#include <iostream>
#include <stdexcept>
#include <string>

void require(bool truth, const char* reason)
{
    if (!truth) throw std::runtime_error(reason);
}
int main()
{
    try
    {
        using biosphere::ui::PresetHistory;
        PresetHistory<std::string> history(5);
        history.reset("Factory 01", 0);
        require(!history.canUndo() && !history.canRedo(), "Empty history has actions");
        history.push("Factory 23", 0);
        history.push("A edited", 0);
        history.push("B original", 1);
        history.push("B edited", 1);
        require(history.size() == 5, "History size was not respected");
        const auto* prior = history.undo();
        require(prior && prior->slot == 1 && prior->snapshot == "B original",
                "Undo within B failed");
        prior = history.undo();
        require(prior && prior->slot == 0 && prior->snapshot == "A edited",
                "Undo B-to-A switch failed");
        const auto* again = history.redo();
        require(again && again->slot == 1 && again->snapshot == "B original",
                "Redo A-to-B switch failed");
        history.push("B alternate", 1);
        require(!history.canRedo(), "New edit failed to clear redo branch");
        require(history.undo() && history.redo(),
                "Undo/Redo after branching failed");
        history.push("Next", 1);
        history.push("Another", 1);
        history.push("Last", 1);
        require(history.size() == 5, "History exceeded its fixed bound");
        for (int n = 0; n < 8; ++n)
            history.undo();
        require(!history.canUndo(), "Undo did not stop at oldest retained state");
        history.reset("DAW reload", 0);
        require(history.size() == 1 && !history.canRedo() && !history.canUndo(),
                "History reset did not clear earlier session");
        std::cout << "PRESET LAB HISTORY PASS: A/B, Undo/Redo, branch pruning, bounded steps\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "PRESET LAB HISTORY FAIL: " << e.what() << '\n';
        return 1;
    }
}
