#pragma once

#include <memory>
#include <stack>
#include <functional>

#include "Engine.h"

namespace ued {

class ICommand {
public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;
    virtual const char* GetName() const = 0;
};

class CommandSystem {
public:
    void Execute(se::Scope<ICommand> command);
    void Undo();
    void Redo();
    
    bool CanUndo() const { return !undoStack_.empty(); }
    bool CanRedo() const { return !redoStack_.empty(); }
    
    void Clear();

private:
    std::stack<se::Scope<ICommand>> undoStack_;
    std::stack<se::Scope<ICommand>> redoStack_;
};

}  // namespace ued
