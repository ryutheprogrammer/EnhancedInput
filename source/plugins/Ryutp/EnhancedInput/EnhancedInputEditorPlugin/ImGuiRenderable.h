#pragma once

class ImGuiRenderable
{
public:
	virtual ~ImGuiRenderable() = default;

	virtual void onRender() = 0;

	virtual void saveToFile(const char *path) const = 0;
	virtual void loadFromFile(const char *path) = 0;
};
