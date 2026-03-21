#pragma once
#include "imgui/imgui.h"
#include <UnigineInput.h>
#include <UnigineMaterials.h>
#include <UnigineMeshDynamic.h>
#include <UnigineTextures.h>

struct ImGuiContext;
class ImFont;

struct FontInfo
{
	const char *path;
	float size;
	const ImWchar *glyph_ranges = nullptr;
	bool merge = false;
};

class ImGuiBackend
{
public:
	virtual ~ImGuiBackend() = default;

	void init(const char* materialPath, const Unigine::Vector<FontInfo> &fonts);
	void shutdown();

	void setMaterialPath(const char* path);

	void addFontFromFileTTF(const char *filename, float size_pixels, const ImWchar *glyph_ranges = nullptr, bool merge = false);
	void rebuildFontAtlas();

	void newFrame(Unigine::Math::ivec2 context_pos, Unigine::Math::ivec2 context_size);
	void render(const Unigine::TexturePtr &texture);

private:
	void create_font_texture();
	void create_mesh();

	void init_keymap();
	void init_theme();

	int key_event(Unigine::Input::KEY key, bool down);
	int key_pressed(Unigine::Input::KEY key);
	int key_released(Unigine::Input::KEY key);
	int button_event(Unigine::Input::MOUSE_BUTTON button, bool down);
	int button_pressed(Unigine::Input::MOUSE_BUTTON button);
	int button_released(Unigine::Input::MOUSE_BUTTON button);
	int unicode_key_pressed(unsigned int key);
	void render_callback();

private:
	ImGuiContext *ctx_{};
	ImFont *default_font_{};
	Unigine::TexturePtr font_texture_;
	Unigine::MeshDynamicPtr mesh_;
	Unigine::MaterialPtr material_;
	Unigine::EventConnections event_connections_;
};
