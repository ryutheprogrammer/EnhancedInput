#pragma once

#include <editor/UniginePlugin.h>
#include <QObject>

class QAction;
class EIEditorWindow;

class EIEditorPlugin: public QObject, public ::UnigineEditor::Plugin
{
	Q_OBJECT
	Q_DISABLE_COPY(EIEditorPlugin)
	Q_PLUGIN_METADATA(IID UNIGINE_EDITOR_PLUGIN_IID FILE "EnhancedInput.json")
	Q_INTERFACES(UnigineEditor::Plugin)

public:
	EIEditorPlugin();
	~EIEditorPlugin() override;

	bool init() override;
	void shutdown() override;

private:
	void toggleWindow();

private:
	QAction *_eiEditorWindowAction = nullptr;
	EIEditorWindow *_eiEditorWindow = nullptr;
};
