#include "CommandSystem.h"

#include "engine/core/Log.h"

namespace ued {

void CommandSystem::Execute(se::Scope<ICommand> command) {
    command->Execute();
    SE_LOG_INFO("Executed command: {}", command->GetName());
    undoStack_.push(std::move(command));
    
    // Clear redo stack when new command is executed
    while (!redoStack_.empty()) {
        redoStack_.pop();
    }
}

void CommandSystem::Undo() {
    if (undoStack_.empty()) {
        SE_LOG_WARN("Nothing to undo");
        return;
    }
    
    auto command = std::move(undoStack_.top());
    undoStack_.pop();
    
    command->Undo();
    SE_LOG_INFO("Undid command: {}", command->GetName());
    
    redoStack_.push(std::move(command));
}

void CommandSystem::Redo() {
    if (redoStack_.empty()) {
        SE_LOG_WARN("Nothing to redo");
        return;
    }
    
    auto command = std::move(redoStack_.top());
    redoStack_.pop();
    
    command->Execute();
    SE_LOG_INFO("Redid command: {}", command->GetName());
    
    undoStack_.push(std::move(command));
}

void CommandSystem::Clear() {
    while (!undoStack_.empty()) undoStack_.pop();
    while (!redoStack_.empty()) redoStack_.pop();
    SE_LOG_INFO("Command history cleared");
}

}  // namespace ued
