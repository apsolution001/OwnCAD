#include "model/CommandHistory.h"
#include "model/Command.h"
#include <QDebug>

namespace OwnCAD {
namespace Model {

CommandHistory::CommandHistory(QObject* parent)
    : QObject(parent)
    , m_maxHistorySize(100)
    , m_modified(false)
{
}

CommandHistory::~CommandHistory() = default;

bool CommandHistory::executeCommand(std::unique_ptr<Command> command)
{
    if (!command) {
        qWarning() << "CommandHistory::executeCommand: null command";
        return false;
    }

    if (!command->isValid()) {
        qWarning() << "CommandHistory::executeCommand: invalid command:"
                   << command->description();
        return false;
    }

    // Execute the command
    if (!command->execute()) {
        qWarning() << "CommandHistory::executeCommand: execute failed:"
                   << command->description();
        return false;
    }

    // Clear redo stack on new command (standard undo/redo behavior)
    // Any commands in redo stack are now unreachable
    m_redoStack.clear();

    // Store description before moving command
    QString desc = command->description();

    // Add to undo stack
    m_undoStack.push_back(std::move(command));

    // Enforce max history size
    trimUndoStack();

    // Mark document as modified
    m_modified = true;

    // Emit signals
    emit commandExecuted(desc);
    emit historyChanged();

    return true;
}

bool CommandHistory::undo()
{
    if (!canUndo()) {
        return false;
    }

    // Pop command from undo stack
    auto command = std::move(m_undoStack.back());
    m_undoStack.pop_back();

    // Store description before potential failure
    QString desc = command->description();

    // Attempt undo
    if (!command->undo()) {
        qWarning() << "CommandHistory::undo: undo failed:" << desc;
        // Command is lost - this is a serious error state
        // We don't put it back on the stack as its state is unknown
        emit historyChanged();
        return false;
    }

    // Move to redo stack
    m_redoStack.push_back(std::move(command));

    // Emit signals
    emit commandUndone(desc);
    emit historyChanged();

    return true;
}

bool CommandHistory::redo()
{
    if (!canRedo()) {
        return false;
    }

    // Pop command from redo stack
    auto command = std::move(m_redoStack.back());
    m_redoStack.pop_back();

    // Store description before potential failure
    QString desc = command->description();

    // Attempt redo
    if (!command->redo()) {
        qWarning() << "CommandHistory::redo: redo failed:" << desc;
        // Command is lost - this is a serious error state
        emit historyChanged();
        return false;
    }

    // Move back to undo stack
    m_undoStack.push_back(std::move(command));

    // Mark document as modified
    m_modified = true;

    // Emit signals
    emit commandRedone(desc);
    emit historyChanged();

    return true;
}

bool CommandHistory::canUndo() const
{
    return !m_undoStack.empty();
}

bool CommandHistory::canRedo() const
{
    return !m_redoStack.empty();
}

QString CommandHistory::undoDescription() const
{
    if (m_undoStack.empty()) {
        return QString();
    }
    return m_undoStack.back()->description();
}

QString CommandHistory::redoDescription() const
{
    if (m_redoStack.empty()) {
        return QString();
    }
    return m_redoStack.back()->description();
}

size_t CommandHistory::undoCount() const
{
    return m_undoStack.size();
}

size_t CommandHistory::redoCount() const
{
    return m_redoStack.size();
}

void CommandHistory::clear()
{
    m_undoStack.clear();
    m_redoStack.clear();
    m_modified = false;
    emit historyChanged();
}

void CommandHistory::setMaxHistorySize(size_t size)
{
    m_maxHistorySize = size;
    trimUndoStack();
}

void CommandHistory::trimUndoStack()
{
    // Size of 0 means unlimited
    if (m_maxHistorySize == 0) {
        return;
    }

    // Remove oldest commands (front of vector) until within limit
    while (m_undoStack.size() > m_maxHistorySize) {
        m_undoStack.erase(m_undoStack.begin());
    }
}

} // namespace Model
} // namespace OwnCAD
