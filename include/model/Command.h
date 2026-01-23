#pragma once

#include <QString>
#include <QDateTime>
#include <memory>

namespace OwnCAD {
namespace Model {

class DocumentModel;

/**
 * @brief Abstract base class for all undoable commands.
 *
 * Commands encapsulate operations that modify the DocumentModel.
 * Each command must be able to execute, undo, and describe itself.
 *
 * INVARIANTS:
 * - execute() must be called before undo()
 * - undo() must be called before redo()
 * - Commands are single-use: execute() should only be called once
 *
 * OWNERSHIP:
 * - Commands hold a raw pointer to DocumentModel (non-owning)
 * - Commands must not outlive the DocumentModel they reference
 * - CommandHistory owns Command objects via unique_ptr
 */
class Command {
public:
    explicit Command(DocumentModel* model);
    virtual ~Command() = default;

    // Non-copyable, non-movable (commands own state for undo)
    Command(const Command&) = delete;
    Command& operator=(const Command&) = delete;
    Command(Command&&) = delete;
    Command& operator=(Command&&) = delete;

    /**
     * @brief Execute the command (first time).
     * @return true if successful, false if failed
     *
     * Implementations must:
     * - Validate preconditions
     * - Perform the operation
     * - Store state needed for undo
     * - Set m_executed = true on success
     */
    virtual bool execute() = 0;

    /**
     * @brief Reverse the command's effect.
     * @return true if successful, false if failed
     *
     * Implementations must:
     * - Restore previous state exactly
     * - Set m_executed = false on success
     */
    virtual bool undo() = 0;

    /**
     * @brief Re-apply the command after undo.
     * @return true if successful, false if failed
     *
     * Default implementation calls execute().
     * Override only if redo differs from initial execute.
     */
    virtual bool redo();

    /**
     * @brief Human-readable description for UI.
     * @return Description like "Draw Line" or "Move 5 entities"
     *
     * Used in Edit menu: "Undo Draw Line", "Redo Move 5 entities"
     */
    virtual QString description() const = 0;

    /**
     * @brief Check if command is in valid state for execution.
     * @return true if command can be executed
     *
     * Default checks that DocumentModel pointer is non-null.
     * Override to add additional validation.
     */
    virtual bool isValid() const;

    /**
     * @brief Check if this command can merge with another.
     * @param other The command to potentially merge with
     * @return true if merge is possible
     *
     * Used for combining rapid successive edits (e.g., continuous dragging).
     * Default returns false (no merging).
     */
    virtual bool canMerge(const Command* other) const;

    /**
     * @brief Merge another command into this one.
     * @param other The command to absorb
     * @return true if merge succeeded
     *
     * Precondition: canMerge(other) returned true.
     * After merge, 'other' should be discarded.
     */
    virtual bool merge(const Command* other);

    // Accessors
    bool isExecuted() const { return m_executed; }
    QDateTime timestamp() const { return m_timestamp; }

protected:
    DocumentModel* m_documentModel;  // Non-owning pointer
    bool m_executed = false;
    QDateTime m_timestamp;
};

// =============================================================================
// INLINE IMPLEMENTATIONS
// =============================================================================

inline Command::Command(DocumentModel* model)
    : m_documentModel(model)
    , m_executed(false)
    , m_timestamp(QDateTime::currentDateTime())
{
}

inline bool Command::redo()
{
    // Default: redo is same as execute
    // Override in subclasses if redo requires different behavior
    return execute();
}

inline bool Command::isValid() const
{
    return m_documentModel != nullptr;
}

inline bool Command::canMerge(const Command* /*other*/) const
{
    return false;
}

inline bool Command::merge(const Command* /*other*/)
{
    return false;
}

} // namespace Model
} // namespace OwnCAD
