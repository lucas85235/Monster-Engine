#include "CommandSystem.h"

#include "engine/Log.h"

using namespace se;

namespace mst {

CommandSystem::CommandSystem(EventBus& eventBus) : eventBus_(eventBus) {}

void CommandSystem::Execute(CommandPtr command) {
    if (!command) return;
    
    std::string name = command->GetName();
    
    command->Execute();
    
    undoStack_.push_back(std::move(command));
    redoStack_.clear();
    
    while (undoStack_.size() > maxUndoLevels_) {
        undoStack_.erase(undoStack_.begin());
    }
    
    SE_LOG_INFO("CommandSystem: Executed '{}'", name);
    
    eventBus_.Publish(CommandExecutedEvent{name, false});
    NotifyStackChanged();
}

void CommandSystem::Undo() {
    if (!CanUndo()) return;
    
    auto command = std::move(undoStack_.back());
    undoStack_.pop_back();
    
    std::string name = command->GetName();
    command->Undo();
    
    redoStack_.push_back(std::move(command));
    
    SE_LOG_INFO("CommandSystem: Undid '{}'", name);
    
    eventBus_.Publish(CommandExecutedEvent{name, true});
    NotifyStackChanged();
}

void CommandSystem::Redo() {
    if (!CanRedo()) return;
    
    auto command = std::move(redoStack_.back());
    redoStack_.pop_back();
    
    std::string name = command->GetName();
    command->Execute();
    
    undoStack_.push_back(std::move(command));
    
    SE_LOG_INFO("CommandSystem: Redid '{}'", name);
    
    eventBus_.Publish(CommandExecutedEvent{name, false});
    NotifyStackChanged();
}

std::string CommandSystem::GetUndoCommandName() const {
    if (undoStack_.empty()) return "";
    return undoStack_.back()->GetName();
}

std::string CommandSystem::GetRedoCommandName() const {
    if (redoStack_.empty()) return "";
    return redoStack_.back()->GetName();
}

void CommandSystem::Clear() {
    undoStack_.clear();
    redoStack_.clear();
    NotifyStackChanged();
    SE_LOG_INFO("CommandSystem: Cleared undo/redo history");
}

void CommandSystem::NotifyStackChanged() {
    eventBus_.Publish(UndoStackChangedEvent{undoStack_.size(), redoStack_.size()});
}

}  // namespace mst
