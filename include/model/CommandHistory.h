#pragma once

#include <QObject>
#include <QString>
#include <vector>
#include <memory>

namespace OwnCAD {
namespace Model {

class Command;

/**
 * @brief Manages undo/redo stacks for command history.
 *
 * INVARIANTS:
 * - Executing a new command clears the redo stack
 * - Undo moves command from undo stack to redo stack
 * - Redo moves command from redo stack to undo stack
 * - Stack size is bounded by maxHistorySize (default 100)
 *
 * THREAD SAFETY: Not thread-safe. All calls must be from UI thread.
 *
 * USAGE:
 * @code
 *   CommandHistory history;
 *   history.executeCommand(std::make_unique<CreateLineCommand>(...));
 *   // Later:
 *   history.undo();  // Reverses the line creation
 *   history.redo();  // Re-creates the line
 * @endcode
 */
class CommandHistory : public QObject {
    Q_OBJECT

public:
    explicit CommandHistory(QObject* parent = nullptr);
    ~CommandHistory() override;

    /**
     * @brief Execute a command and add to history.
     * @param command The command to execute (ownership transferred)
     * @return true if execution succeeded
     *
     * On success:
     * - Command is pushed to undo stack
     * - Redo stack is cleared
     * - historyChanged() signal emitted
     * - commandExecuted() signal emitted with description
     *
     * On failure:
     * - Command is discarded
     * - Stacks unchanged
     * - No signals emitted
     */
    bool executeCommand(std::unique_ptr<Command> command);

    /**
     * @brief Undo the most recent command.
     * @return true if undo succeeded, false if nothing to undo or undo failed
     *
     * On success:
     * - Command moved from undo stack to redo stack
     * - commandUndone() signal emitted
     * - historyChanged() signal emitted
     */
    bool undo();

    /**
     * @brief Redo the most recently undone command.
     * @return true if redo succeeded, false if nothing to redo or redo failed
     *
     * On success:
     * - Command moved from redo stack to undo stack
     * - commandRedone() signal emitted
     * - historyChanged() signal emitted
     */
    bool redo();

    // State queries
    bool canUndo() const;
    bool canRedo() const;

    /**
     * @brief Get description of next command to undo.
     * @return Description string, or empty if nothing to undo
     */
    QString undoDescription() const;

    /**
     * @brief Get description of next command to redo.
     * @return Description string, or empty if nothing to redo
     */
    QString redoDescription() const;

    /**
     * @brief Number of commands in undo stack.
     */
    size_t undoCount() const;

    /**
     * @brief Number of commands in redo stack.
     */
    size_t redoCount() const;

    /**
     * @brief Clear all history (e.g., on document close or new document).
     *
     * Emits historyChanged() signal.
     */
    void clear();

    /**
     * @brief Set maximum history size.
     * @param size Max commands to keep (oldest discarded first)
     *
     * Default is 100. Set to 0 for unlimited (not recommended for CAD
     * applications due to memory concerns with large geometry).
     *
     * If current stack exceeds new size, oldest commands are trimmed.
     */
    void setMaxHistorySize(size_t size);

    /**
     * @brief Get current max history size.
     */
    size_t maxHistorySize() const { return m_maxHistorySize; }

    /**
     * @brief Check if document has been modified since last save.
     *
     * Returns true if there are commands in the undo stack that
     * haven't been saved. Use markAsSaved() after saving.
     */
    bool isModified() const { return m_modified; }

    /**
     * @brief Mark document as saved (clears modified flag).
     *
     * Call after successfully saving the document.
     */
    void markAsSaved() { m_modified = false; }

signals:
    /**
     * @brief Emitted when undo/redo availability changes.
     *
     * Connect to this to update UI state (enable/disable buttons).
     */
    void historyChanged();

    /**
     * @brief Emitted after a command is executed.
     * @param description The command's description
     */
    void commandExecuted(const QString& description);

    /**
     * @brief Emitted after a command is undone.
     * @param description The undone command's description
     */
    void commandUndone(const QString& description);

    /**
     * @brief Emitted after a command is redone.
     * @param description The redone command's description
     */
    void commandRedone(const QString& description);

private:
    /**
     * @brief Trim undo stack to max size (removes oldest commands).
     */
    void trimUndoStack();

    std::vector<std::unique_ptr<Command>> m_undoStack;
    std::vector<std::unique_ptr<Command>> m_redoStack;
    size_t m_maxHistorySize = 100;
    bool m_modified = false;
};

} // namespace Model
} // namespace OwnCAD
