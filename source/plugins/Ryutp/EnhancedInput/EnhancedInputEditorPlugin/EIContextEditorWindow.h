#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include "ImGuiRenderable.h"

class EIContextEditorWindow: public ImGuiRenderable
{
public:
	~EIContextEditorWindow();

	void onRender() override;

	void saveToFile(const char *path) const override;
	void loadFromFile(const char *path) override;

private:
	Unigine::String _path;
	EIContext *_context;
};
