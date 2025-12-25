#pragma once
/**
 * ICommand.h - Command pattern interface for undoable operations.
 *
 * All editor operations that modify state should implement this interface
 * to enable undo/redo functionality.
 */

#include <string>
#include <memory>

namespace mst {

class ICommand {
public:
    virtual ~ICommand() = default;

    virtual void Execute() = 0;
    virtual void Undo() = 0;
    
    virtual std::string GetName() const = 0;
    
    virtual bool CanMerge(const ICommand& other) const { return false; }
    virtual void Merge(const ICommand& other) {}
};

using CommandPtr = std::unique_ptr<ICommand>;

}  // namespace mst
