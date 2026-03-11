#pragma once
#include <EnhancedInput/EnhancedInput.h>
#include "ImGuiRenderable.h"
#include <UnigineString.h>
#include <UnigineComponentSystem.h>

class EIActionEditorWindow: public ImGuiRenderable
{
public:
	void onRender() override;

	void saveToFile(const char *path) const override;
	void loadFromFile(const char *path) override;

private:
	Unigine::String _path;
	EIAction *_action;
};
