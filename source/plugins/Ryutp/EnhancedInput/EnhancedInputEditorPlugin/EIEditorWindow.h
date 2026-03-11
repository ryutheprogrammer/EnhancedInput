#pragma once
#include <EnhancedInput/EnhancedInput.h>
#include "ImGuiBackend.h"
#include <UnigineWidgets.h>
#include <editor/UnigineEngineGuiWindow.h>
#include <memory>

class ImGuiRenderable;

class EIEditorWindow: public UnigineEditor::EngineGuiWindow
{
public:
	using Super = UnigineEditor::EngineGuiWindow;

	EIEditorWindow(QWidget *parent = nullptr);
	~EIEditorWindow() override;

protected:
	void onUpdate() override;
	void onRender() override;

private:
	void renderWindow();
	void renderFileList(const char *id, const char *name,
		const char *ext, std::function<bool(const char *)> creator,
		EIFileSystemRegistryBase *registry);

private:
	Unigine::TexturePtr _imguiTexture;
	ImGuiBackend _imguiBackend;
	Unigine::WidgetSpritePtr _imguiSprite;

	Unigine::String _currentFile;

	Unigine::HashMap<Unigine::String, std::function<ImGuiRenderable *()>> _windows;
	std::unique_ptr<ImGuiRenderable> _currentWindow;
};
