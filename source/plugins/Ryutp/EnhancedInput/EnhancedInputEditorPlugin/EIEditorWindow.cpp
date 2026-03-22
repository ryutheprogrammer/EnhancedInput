#include "EIEditorWindow.h"
#include "UnigineImGui.h"
#include "EIActionEditorWindow.h"
#include "EIContextEditorWindow.h"
#include "ImGuiThemes.h"
#include "IconButton.h"

#include <UnigineWindowManager.h>
#include <UnigineFileSystem.h>

using namespace Unigine;
using namespace Unigine::Math;

EIEditorWindow::EIEditorWindow(QWidget *parent)
	: UnigineEditor::EngineGuiWindow(parent)
{
	_imguiTexture = Texture::create();

	_imguiSprite = WidgetSprite::create("white.texture");
	_imguiSprite->setOrder(128);
	getGui()->addChild(_imguiSprite, Gui::ALIGN_OVERLAP);
	_imguiSprite->setRender(_imguiTexture);

	{
		auto materialPath = FileSystem::getAbsolutePath("plugins/Ryutp/EnhancedInput/imgui.basemat");
		auto fontPath = FileSystem::getAbsolutePath("plugins/Ryutp/EnhancedInput/Font Awesome 7 Free-Solid-900.otf");
		
		static const ImWchar icons_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
		
		FontInfo fontInfo;
		fontInfo.path = fontPath;
		fontInfo.size = 12.0f;
		fontInfo.glyph_ranges = icons_ranges;
		fontInfo.merge = true;

		_imguiBackend.init(materialPath, { fontInfo });
	}
	
	ImGuiStyle &style = ImGui::GetStyle();
	style.FontScaleDpi = getGui()->getDpiScale();
	style.FontSizeBase = 18;
	style.FrameRounding = 6.0f;

	// TODO add button?
	ImGuiThemes::dark1();

	removeShortcutsExclusiveContext();

	_windows["input_action"] = [] { return new EIActionEditorWindow; };
	_windows["input_context"] = [] { return new EIContextEditorWindow; };

	Vector<String> exts;
	for (const auto &window : _windows)
		exts.append(window.key);
}

EIEditorWindow::~EIEditorWindow()
{
	_imguiBackend.shutdown();

	_windows.clear();
}

void EIEditorWindow::onUpdate()
{
	Super::onUpdate();

	auto gui = getGui();

	_imguiSprite->setPosition(0, 0);
	_imguiSprite->setWidth(gui->getWidth());
	_imguiSprite->setHeight(gui->getHeight());

	if (_imguiSprite->getWidth() != _imguiTexture->getWidth() ||
		_imguiSprite->getHeight() != _imguiTexture->getHeight())
	{
		_imguiTexture->create2D(_imguiSprite->getWidth(), _imguiSprite->getHeight(),
			Texture::FORMAT_RGBA8, Texture::FORMAT_USAGE_RENDER);
	}

	ivec2 ctxPos = gui->getPosition();
	ivec2 ctxSize = gui->getSize();

	if (auto mainWindow = WindowManager::getMainWindow())
	{
		auto dpi_scale = mainWindow->getDpiScale();
		ctxSize = mainWindow->getClientSize();
		ctxSize = ivec2(vec2(ctxSize) * dpi_scale);
		ctxPos = mainWindow->getClientPosition();
	}

	_imguiBackend.newFrame(ctxPos, ctxSize);

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
	ImGui::SetNextWindowSize(ImVec2(gui->getWidth(), gui->getHeight()), ImGuiCond_Always);
	auto windowFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
					   ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoMove;

	if (ImGui::Begin("EnhancedInput", nullptr, windowFlags))
	{
		renderWindow();
		ImGui::End();
	}
}

void EIEditorWindow::onRender()
{
	_imguiBackend.render(_imguiTexture);

	Super::onRender();
}

void EIEditorWindow::renderWindow()
{
	if (!_currentWindow)
	{
		auto actionSaver = [](const char *path) -> bool {
			return EISystem::get()->getActionRegistry()->saveDummy(path);
		};
		auto contextSaver = [](const char *path) -> bool {
			return EISystem::get()->getContextRegistry()->saveDummy(path);
		};

		if (ImGui::BeginChild("##child_window", ImVec2(ImGui::GetContentRegionAvail().x,
													ImGui::GetContentRegionAvail().y)))
		{
			renderFileList("##files_actions", "Actions", "input_action",
				actionSaver,
				EISystem::get()->getActionRegistry());

			renderFileList("##files_contexts", "Contexts", "input_context",
				contextSaver,
				EISystem::get()->getContextRegistry());
			ImGui::EndChild();
		}
	} else
	{
		if (ImGui::BeginChild("##child_window", ImVec2(ImGui::GetContentRegionAvail().x,
													ImGui::GetContentRegionAvail().y - 35)))
		{
			_currentWindow->onRender();
			ImGui::EndChild();
		}

		ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x - 210, 0));
		ImGui::SameLine();
		if (ImGui::Button("Save##save", ImVec2(100, 30)))
		{
			_currentWindow->saveToFile(_currentFile);
			_currentWindow = nullptr;
		}

		ImGui::SameLine();

		if (ImGui::Button("Discard##discard", ImVec2(100, 30)))
		{
			_currentWindow = nullptr;
		}
	}
}

void EIEditorWindow::renderFileList(const char *id, const char *name,
	const char *ext, std::function<bool(const char *)> creator,
	EIFileSystemRegistryBase *registry)
{
	bool open = ImGui::TreeNodeEx(id, ImGuiTreeNodeFlags_AllowItemOverlap |
										  ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DrawLinesFull |
										  ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen |
										  (registry->getCount() == 0 ? ImGuiTreeNodeFlags_Leaf : 0));
	defer
	{
		if (open)
			ImGui::TreePop();
	};

	ImGui::SameLine();
	ImGui::TextUnformatted(name);
	ImGui::SameLine();
	if (ImGui::Button(FMT(ICON_FA_PLUS "##%s_add", id)))
	{
		String path = WindowManager::dialogSaveFile("", ext);
		if (path != "" && path.extension() != ext)
			path = String::format("%s.%s", path.get(), ext);

		if (path != "" && creator(path))
			registry->refresh();
	}

	if (open)
	{
		for (int i = 0; i < registry->getCount(); ++i)
		{
			ImGui::Selectable(FMT("%s##%s_files[%i]", Unigine::String(registry->getPath(i)).filename().get(), id, i).get(), false);

			if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
			{
				_currentWindow = nullptr;

				_currentFile = registry->getPath(i);

				auto it = _windows.find(_currentFile.extension());
				if (it != _windows.end())
				{
					_currentWindow.reset(it->data());
					_currentWindow->loadFromFile(_currentFile);
				} else
				{
					Log::error("Unsupported extension\n");
				}
			}
		}
	}
}
