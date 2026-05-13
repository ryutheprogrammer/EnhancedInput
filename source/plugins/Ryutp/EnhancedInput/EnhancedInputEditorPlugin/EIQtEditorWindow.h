#pragma once
#include <QWidget>

class QLayout;
class QLineEdit;
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

	void populateMiddleForContext();
	void populateInspectorForContext();
	void populateInspectorForAction();
	void populateInspectorForMapping(EIMapping *mapping);

	void refreshContextView();
	void refreshInspector();
	void updateSelectionItemText();

	EIMapping *lookupCurrentMapping();

	void releaseCurrent();
	void clearLayout(QLayout *layout);

	QTreeWidget *_contextList = nullptr;
	QTreeWidget *_actionList = nullptr;
	QLineEdit *_contextFilter = nullptr;
	QLineEdit *_actionFilter = nullptr;
	QWidget *_middleWidget = nullptr;
	QVBoxLayout *_middleLayout = nullptr;
	QVBoxLayout *_rightLayout = nullptr;
	QTreeWidget *_tree = nullptr;

	EIContext *_currentContext = nullptr;
	EIAction *_currentAction = nullptr;
	EIMapping *_currentMapping = nullptr;
	int _currentActionIdx = -1;
	int _currentMappingIdx = -1;
	// -1 = mapping row (parent), 0 = primary binding, >=1 = andKeys[idx-1]
	int _currentBindingIdx = -1;
};
