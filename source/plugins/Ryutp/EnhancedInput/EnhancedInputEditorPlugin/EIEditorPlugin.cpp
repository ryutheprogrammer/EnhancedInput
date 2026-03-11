#include "EIEditorPlugin.h"
#include "EIEditorWindow.h"
#include <editor/UnigineWindowManager.h>
#include <editor/UnigineConstants.h>
#include <UnigineLog.h>
#include <QAction>
#include <QMenu>
#include <UnigineComponentSystem.h>
#include <UnigineEngine.h>

EIEditorPlugin::EIEditorPlugin() = default;
EIEditorPlugin::~EIEditorPlugin() = default;

bool EIEditorPlugin::init()
{
	Unigine::Log::message("EnhancedInput::init");

	if (!Unigine::Engine::get()->addPlugin("RyutpEnhancedInput"))
		return false;

	auto menu_windows = UnigineEditor::WindowManager::findMenu(UnigineEditor::Constants::MM_WINDOWS);

	_eiEditorWindowAction = menu_windows->addAction("EnhancedInput", this, &EIEditorPlugin::toggleWindow);
	return true;
}

void EIEditorPlugin::shutdown()
{
	Unigine::Log::message("EnhancedInput::shutdown");

	auto menu_windows = UnigineEditor::WindowManager::findMenu(UnigineEditor::Constants::MM_WINDOWS);
	menu_windows->removeAction(_eiEditorWindowAction);
	_eiEditorWindowAction = nullptr;

	if (_eiEditorWindow)
	{
		UnigineEditor::WindowManager::remove(_eiEditorWindow);

		delete _eiEditorWindow;
		_eiEditorWindow = nullptr;
	}
}

void EIEditorPlugin::toggleWindow()
{
	if (_eiEditorWindow)
	{
		if (UnigineEditor::WindowManager::isHidden(_eiEditorWindow))
		{
			UnigineEditor::WindowManager::show(_eiEditorWindow);
		}
		return;
	}

	_eiEditorWindow = new EIEditorWindow;
	_eiEditorWindow->setWindowTitle("EnhancedInput");

	UnigineEditor::WindowManager::add(_eiEditorWindow, UnigineEditor::WindowManager::NEW_FLOATING_AREA);

	connect(_eiEditorWindow, &QObject::destroyed, this, [this]() { _eiEditorWindow = nullptr; });

	UnigineEditor::WindowManager::show(_eiEditorWindow);
}
