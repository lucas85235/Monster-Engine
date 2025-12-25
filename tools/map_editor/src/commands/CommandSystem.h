#pragma once
/**
 * CommandSystem.h - Manages command execution with undo/redo stack.
 *
 * Provides centralized command execution ensuring all operations
 * go through the same undo/redo mechanism.
 */

#include <vector>
#include <memory>

#include "ICommand.h"
#include "../core/EventBus.h"

namespace mst {

struct CommandExecutedEvent {
    std::string commandName;
    bool isUndo;
};

struct UndoStackChangedEvent {
    size_t undoCount;
    size_t redoCount;
};

class CommandSystem {
public:
    explicit CommandSystem(EventBus& eventBus);
    
    void Execute(CommandPtr command);
    
    void Undo();
    void Redo();
    
    bool CanUndo() const { return !undoStack_.empty(); }
    bool CanRedo() const { return !redoStack_.empty(); }
    
    std::string GetUndoCommandName() const;
    std::string GetRedoCommandName() const;
    
    size_t GetUndoCount() const { return undoStack_.size(); }
    size_t GetRedoCount() const { return redoStack_.size(); }
    
    void Clear();
    
    void SetMaxUndoLevels(size_t levels) { maxUndoLevels_ = levels; }

private:
    void NotifyStackChanged();
    
    EventBus& eventBus_;
    std::vector<CommandPtr> undoStack_;
    std::vector<CommandPtr> redoStack_;
    size_t maxUndoLevels_ = 100;
};

}  // namespace mst
