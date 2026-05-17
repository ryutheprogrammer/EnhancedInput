#pragma once
#include <QPointer>
#include <QString>
#include <QWidget>

class QLabel;
class QLayout;
class QLineEdit;
class QPushButton;
class QTreeWidget;
class QVBoxLayout;
class QWidget;
class EIContext;
struct EIAction;
struct EIMapping;

class EIQtEditorWindow: public QWidget
{
	Q_OBJECT

public:
	EIQtEditorWindow(QWidget *parent = nullptr);
	~EIQtEditorWindow() override;

private:
	void refreshContexts();
	void refreshActions();
	void applyContextFilter();
	void applyActionFilter();

	void onCreateContext();
	void onCreateAction();
	void onDeleteContext(int row);
	void onDeleteAction(int row);
	void onRenameContext(int row);
	void onRenameAction(int row);

	void onContextSelected(int row);
	void onActionSelected(int row);

	void onSaveContext();
	void onSaveAction();
	void onDiscard();

	void populateMiddleForContext();
	void populateInspectorForContext();
	void populateInspectorForAction();
	void populateInspectorForActionEntry();
	void populateInspectorForMapping(EIMapping *mapping);

	void refreshContextView();
	void refreshInspector();
	void updateSelectionItemText();

	EIMapping *lookupCurrentMapping();

	void releaseCurrent();
	void clearLayout(QLayout *layout);

	// Dirty-state tracking. The visible " *" suffix lives on the asset row in
	// the left-pane list — that label is stable across middle-pane navigation
	// so switching mappings/bindings inside the same context keeps the
	// indicator. markDirty()/clearDirty() also drive Save/Discard button
	// enabled state.
	void markDirty();
	void clearDirty();
	void addSaveDiscardRow();
	QLabel *createInspectorTitle(const QString &base);
	QLabel *getActiveAssetLabel() const;
	// Returns false if the user picks Cancel — caller must abort the switch.
	// Save → writes to disk and clears dirty. Discard → returns true so the
	// caller proceeds; in-memory changes are dropped by releaseCurrent.
	bool promptUnsavedChanges();

	QTreeWidget *_contextList = nullptr;
	QTreeWidget *_actionList = nullptr;
	QLineEdit *_contextFilter = nullptr;
	QLineEdit *_actionFilter = nullptr;
	QWidget *_middleWidget = nullptr;
	QVBoxLayout *_middleLayout = nullptr;
	QVBoxLayout *_rightLayout = nullptr;
	QTreeWidget *_tree = nullptr;
	// QPointers go null automatically when clearLayout deleteLater's the
	// previous inspector — markDirty/clearDirty can safely skip the update
	// during the brief window between teardown and rebuild.
	QPointer<QPushButton> _saveBtn;
	QPointer<QPushButton> _discardBtn;

	EIContext *_currentContext = nullptr;
	EIAction *_currentAction = nullptr;
	EIMapping *_currentMapping = nullptr;
	int _currentContextRow = -1;
	int _currentActionRow = -1;
	int _currentActionIdx = -1;
	int _currentMappingIdx = -1;
	// -1 = mapping row (parent), 0 = primary binding, >=1 = andKeys[idx-1]
	int _currentBindingIdx = -1;
	bool _dirty = false;
};
