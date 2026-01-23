#include <QTest>
#include <QSignalSpy>
#include "model/Command.h"
#include "model/CommandHistory.h"
#include "model/DocumentModel.h"

using namespace OwnCAD::Model;

/**
 * @brief Mock command for testing CommandHistory
 *
 * Tracks execute/undo/redo calls and can be configured to fail.
 */
class MockCommand : public Command {
public:
    explicit MockCommand(DocumentModel* model, const QString& desc = "Mock Command")
        : Command(model)
        , m_description(desc)
        , m_executeCount(0)
        , m_undoCount(0)
        , m_redoCount(0)
        , m_failOnExecute(false)
        , m_failOnUndo(false)
        , m_failOnRedo(false)
    {
    }

    bool execute() override {
        m_executeCount++;
        if (m_failOnExecute) {
            return false;
        }
        m_executed = true;
        return true;
    }

    bool undo() override {
        m_undoCount++;
        if (m_failOnUndo) {
            return false;
        }
        m_executed = false;
        return true;
    }

    bool redo() override {
        m_redoCount++;
        if (m_failOnRedo) {
            return false;
        }
        m_executed = true;
        return true;
    }

    QString description() const override {
        return m_description;
    }

    // Configuration
    void setFailOnExecute(bool fail) { m_failOnExecute = fail; }
    void setFailOnUndo(bool fail) { m_failOnUndo = fail; }
    void setFailOnRedo(bool fail) { m_failOnRedo = fail; }

    // Inspection
    int executeCount() const { return m_executeCount; }
    int undoCount() const { return m_undoCount; }
    int redoCount() const { return m_redoCount; }

private:
    QString m_description;
    int m_executeCount;
    int m_undoCount;
    int m_redoCount;
    bool m_failOnExecute;
    bool m_failOnUndo;
    bool m_failOnRedo;
};

/**
 * @brief Mock command that tracks a value for verifying undo/redo state changes
 */
class ValueCommand : public Command {
public:
    ValueCommand(DocumentModel* model, int& value, int newValue)
        : Command(model)
        , m_value(value)
        , m_newValue(newValue)
        , m_oldValue(value)
    {
    }

    bool execute() override {
        m_oldValue = m_value;
        m_value = m_newValue;
        m_executed = true;
        return true;
    }

    bool undo() override {
        m_value = m_oldValue;
        m_executed = false;
        return true;
    }

    QString description() const override {
        return QString("Set value to %1").arg(m_newValue);
    }

private:
    int& m_value;
    int m_newValue;
    int m_oldValue;
};

class TestCommandHistory : public QObject {
    Q_OBJECT

private slots:
    void initTestCase() {
        // Create a document model for testing
        m_document = std::make_unique<DocumentModel>();
    }

    void cleanupTestCase() {
        m_document.reset();
    }

    void init() {
        // Fresh command history for each test
        m_history = std::make_unique<CommandHistory>();
    }

    void cleanup() {
        m_history.reset();
    }

    // =========================================================================
    // BASIC FUNCTIONALITY TESTS
    // =========================================================================

    void testInitialState() {
        QVERIFY(!m_history->canUndo());
        QVERIFY(!m_history->canRedo());
        QCOMPARE(m_history->undoCount(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(0));
        QVERIFY(m_history->undoDescription().isEmpty());
        QVERIFY(m_history->redoDescription().isEmpty());
        QVERIFY(!m_history->isModified());
    }

    void testExecuteSingleCommand() {
        auto cmd = std::make_unique<MockCommand>(m_document.get(), "Test Command");

        bool result = m_history->executeCommand(std::move(cmd));

        QVERIFY(result);
        QVERIFY(m_history->canUndo());
        QVERIFY(!m_history->canRedo());
        QCOMPARE(m_history->undoCount(), size_t(1));
        QCOMPARE(m_history->undoDescription(), QString("Test Command"));
        QVERIFY(m_history->isModified());
    }

    void testUndoSingleCommand() {
        auto cmd = std::make_unique<MockCommand>(m_document.get());
        m_history->executeCommand(std::move(cmd));

        bool result = m_history->undo();

        QVERIFY(result);
        QVERIFY(!m_history->canUndo());
        QVERIFY(m_history->canRedo());
        QCOMPARE(m_history->undoCount(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(1));
    }

    void testRedoSingleCommand() {
        auto cmd = std::make_unique<MockCommand>(m_document.get());
        m_history->executeCommand(std::move(cmd));
        m_history->undo();

        bool result = m_history->redo();

        QVERIFY(result);
        QVERIFY(m_history->canUndo());
        QVERIFY(!m_history->canRedo());
        QCOMPARE(m_history->undoCount(), size_t(1));
        QCOMPARE(m_history->redoCount(), size_t(0));
    }

    // =========================================================================
    // REDO STACK CLEARING TESTS
    // =========================================================================

    void testExecuteClearsRedoStack() {
        // Execute, undo, then execute new command
        auto cmd1 = std::make_unique<MockCommand>(m_document.get(), "Command 1");
        m_history->executeCommand(std::move(cmd1));
        m_history->undo();

        QCOMPARE(m_history->redoCount(), size_t(1));

        auto cmd2 = std::make_unique<MockCommand>(m_document.get(), "Command 2");
        m_history->executeCommand(std::move(cmd2));

        // Redo stack should be cleared
        QCOMPARE(m_history->redoCount(), size_t(0));
        QVERIFY(!m_history->canRedo());
        QCOMPARE(m_history->undoDescription(), QString("Command 2"));
    }

    // =========================================================================
    // EMPTY STACK TESTS
    // =========================================================================

    void testUndoEmptyStack() {
        bool result = m_history->undo();
        QVERIFY(!result);
    }

    void testRedoEmptyStack() {
        bool result = m_history->redo();
        QVERIFY(!result);
    }

    // =========================================================================
    // FAILURE TESTS
    // =========================================================================

    void testNullCommandRejected() {
        bool result = m_history->executeCommand(nullptr);

        QVERIFY(!result);
        QCOMPARE(m_history->undoCount(), size_t(0));
    }

    void testInvalidCommandRejected() {
        // Command with null DocumentModel is invalid
        auto cmd = std::make_unique<MockCommand>(nullptr);

        bool result = m_history->executeCommand(std::move(cmd));

        QVERIFY(!result);
        QCOMPARE(m_history->undoCount(), size_t(0));
    }

    void testFailedExecuteNotAdded() {
        auto cmd = std::make_unique<MockCommand>(m_document.get());
        cmd->setFailOnExecute(true);

        bool result = m_history->executeCommand(std::move(cmd));

        QVERIFY(!result);
        QCOMPARE(m_history->undoCount(), size_t(0));
        QVERIFY(!m_history->isModified());
    }

    void testFailedUndoRemovesCommand() {
        auto cmd = std::make_unique<MockCommand>(m_document.get());
        cmd->setFailOnUndo(true);
        m_history->executeCommand(std::move(cmd));

        bool result = m_history->undo();

        // Command is lost on failed undo
        QVERIFY(!result);
        QCOMPARE(m_history->undoCount(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(0));
    }

    void testFailedRedoRemovesCommand() {
        auto cmd = std::make_unique<MockCommand>(m_document.get());
        cmd->setFailOnRedo(true);
        m_history->executeCommand(std::move(cmd));
        m_history->undo();

        bool result = m_history->redo();

        // Command is lost on failed redo
        QVERIFY(!result);
        QCOMPARE(m_history->undoCount(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(0));
    }

    // =========================================================================
    // MULTI-COMMAND TESTS
    // =========================================================================

    void testMultipleCommands() {
        for (int i = 1; i <= 5; i++) {
            auto cmd = std::make_unique<MockCommand>(
                m_document.get(), QString("Command %1").arg(i));
            m_history->executeCommand(std::move(cmd));
        }

        QCOMPARE(m_history->undoCount(), size_t(5));
        QCOMPARE(m_history->undoDescription(), QString("Command 5"));

        // Undo 3 times
        m_history->undo();
        m_history->undo();
        m_history->undo();

        QCOMPARE(m_history->undoCount(), size_t(2));
        QCOMPARE(m_history->redoCount(), size_t(3));
        QCOMPARE(m_history->undoDescription(), QString("Command 2"));
        QCOMPARE(m_history->redoDescription(), QString("Command 3"));
    }

    void testValueCommandUndoRedo() {
        int value = 0;

        auto cmd1 = std::make_unique<ValueCommand>(m_document.get(), value, 10);
        m_history->executeCommand(std::move(cmd1));
        QCOMPARE(value, 10);

        auto cmd2 = std::make_unique<ValueCommand>(m_document.get(), value, 20);
        m_history->executeCommand(std::move(cmd2));
        QCOMPARE(value, 20);

        m_history->undo();
        QCOMPARE(value, 10);

        m_history->undo();
        QCOMPARE(value, 0);

        m_history->redo();
        QCOMPARE(value, 10);

        m_history->redo();
        QCOMPARE(value, 20);
    }

    // =========================================================================
    // MAX HISTORY SIZE TESTS
    // =========================================================================

    void testMaxHistorySize() {
        m_history->setMaxHistorySize(3);

        for (int i = 1; i <= 5; i++) {
            auto cmd = std::make_unique<MockCommand>(
                m_document.get(), QString("Command %1").arg(i));
            m_history->executeCommand(std::move(cmd));
        }

        // Only 3 most recent commands should remain
        QCOMPARE(m_history->undoCount(), size_t(3));
        QCOMPARE(m_history->undoDescription(), QString("Command 5"));

        // Oldest commands were discarded
        m_history->undo();
        QCOMPARE(m_history->undoDescription(), QString("Command 4"));
        m_history->undo();
        QCOMPARE(m_history->undoDescription(), QString("Command 3"));
        m_history->undo();

        // No more to undo (commands 1 and 2 were discarded)
        QVERIFY(!m_history->canUndo());
    }

    void testUnlimitedHistory() {
        m_history->setMaxHistorySize(0);  // 0 = unlimited

        for (int i = 0; i < 200; i++) {
            auto cmd = std::make_unique<MockCommand>(m_document.get());
            m_history->executeCommand(std::move(cmd));
        }

        QCOMPARE(m_history->undoCount(), size_t(200));
    }

    // =========================================================================
    // CLEAR TESTS
    // =========================================================================

    void testClear() {
        for (int i = 0; i < 5; i++) {
            auto cmd = std::make_unique<MockCommand>(m_document.get());
            m_history->executeCommand(std::move(cmd));
        }
        m_history->undo();
        m_history->undo();

        QCOMPARE(m_history->undoCount(), size_t(3));
        QCOMPARE(m_history->redoCount(), size_t(2));

        m_history->clear();

        QCOMPARE(m_history->undoCount(), size_t(0));
        QCOMPARE(m_history->redoCount(), size_t(0));
        QVERIFY(!m_history->canUndo());
        QVERIFY(!m_history->canRedo());
        QVERIFY(!m_history->isModified());
    }

    // =========================================================================
    // SIGNAL TESTS
    // =========================================================================

    void testHistoryChangedSignal() {
        QSignalSpy spy(m_history.get(), &CommandHistory::historyChanged);

        auto cmd = std::make_unique<MockCommand>(m_document.get());
        m_history->executeCommand(std::move(cmd));
        QCOMPARE(spy.count(), 1);

        m_history->undo();
        QCOMPARE(spy.count(), 2);

        m_history->redo();
        QCOMPARE(spy.count(), 3);

        m_history->clear();
        QCOMPARE(spy.count(), 4);
    }

    void testCommandExecutedSignal() {
        QSignalSpy spy(m_history.get(), &CommandHistory::commandExecuted);

        auto cmd = std::make_unique<MockCommand>(m_document.get(), "Test Command");
        m_history->executeCommand(std::move(cmd));

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("Test Command"));
    }

    void testCommandUndoneSignal() {
        QSignalSpy spy(m_history.get(), &CommandHistory::commandUndone);

        auto cmd = std::make_unique<MockCommand>(m_document.get(), "Test Command");
        m_history->executeCommand(std::move(cmd));
        m_history->undo();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("Test Command"));
    }

    void testCommandRedoneSignal() {
        QSignalSpy spy(m_history.get(), &CommandHistory::commandRedone);

        auto cmd = std::make_unique<MockCommand>(m_document.get(), "Test Command");
        m_history->executeCommand(std::move(cmd));
        m_history->undo();
        m_history->redo();

        QCOMPARE(spy.count(), 1);
        QList<QVariant> arguments = spy.takeFirst();
        QCOMPARE(arguments.at(0).toString(), QString("Test Command"));
    }

    void testNoSignalOnFailedExecute() {
        QSignalSpy execSpy(m_history.get(), &CommandHistory::commandExecuted);
        QSignalSpy histSpy(m_history.get(), &CommandHistory::historyChanged);

        auto cmd = std::make_unique<MockCommand>(m_document.get());
        cmd->setFailOnExecute(true);
        m_history->executeCommand(std::move(cmd));

        QCOMPARE(execSpy.count(), 0);
        QCOMPARE(histSpy.count(), 0);
    }

    // =========================================================================
    // MODIFIED FLAG TESTS
    // =========================================================================

    void testModifiedFlag() {
        QVERIFY(!m_history->isModified());

        auto cmd = std::make_unique<MockCommand>(m_document.get());
        m_history->executeCommand(std::move(cmd));
        QVERIFY(m_history->isModified());

        m_history->markAsSaved();
        QVERIFY(!m_history->isModified());

        // Redo should set modified again
        m_history->undo();
        m_history->redo();
        QVERIFY(m_history->isModified());
    }

private:
    std::unique_ptr<DocumentModel> m_document;
    std::unique_ptr<CommandHistory> m_history;
};

QTEST_MAIN(TestCommandHistory)
#include "test_CommandHistory.moc"
