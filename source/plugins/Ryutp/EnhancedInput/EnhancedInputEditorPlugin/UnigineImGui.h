#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include "imgui/imgui.h"
#include "IconButton.h"
#include <UnigineComponentSystem.h>

#define IMGUI_SIND(W) ImGui::Indenter __indenter_##__COUNTER__(W)
#define defer deferrer TOKEN_CONCAT(__deferred, __COUNTER__) = [&]

template <typename T>
struct deferrer
{
	T f;
	deferrer(T f)
		: f(f) {};
	deferrer(const deferrer &) = delete;
	~deferrer() { f(); }
};

#define TOKEN_CONCAT_NX(a, b) a##b
#define TOKEN_CONCAT(a, b) TOKEN_CONCAT_NX(a, b)


namespace ImGui
{

class Indenter
{
public:
	Indenter(float w)
		: _w(w)
	{
		ImGui::Indent(_w);
	}
	~Indenter() { ImGui::Unindent(_w); }

private:
	float _w;
};

bool InputText(const char *label, Unigine::String &str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback callback = nullptr, void *user_data = nullptr);
void Input(const char *baseId, Unigine::ComponentVariable *variable);
void Input(const char *baseId, Unigine::ComponentStruct *structure);

template <class T, typename std::enable_if_t<std::is_enum_v<T>, void *> = nullptr>
bool ComboEnum(const char *label, T &e, int height_in_items = -1)
{
	int i = 0;
	while (i < Enum<T>::Count && Enum<T>::Items[i] != e)
		++i;

	bool ret = ImGui::Combo(label, &i, Enum<T>::StringItems, Enum<T>::Count, height_in_items);

	e = Enum<T>::Items[i];
	return ret;
}

} // namespace ImGui

inline bool ToggleArrowButton(const char *id, bool &state)
{
	bool clicked = ImGui::ArrowButton(id, state ? ImGuiDir_Down : ImGuiDir_Right);
	if (clicked)
		state = !state;

	return clicked;
}

inline const char *getStringFromVec(void *data, int i)
{
	return ((Unigine::Vector<Unigine::String> *)data)->get(i);
}

inline bool Combo(const char *id, int &current, const Unigine::Vector<Unigine::String> &list)
{
	return ImGui::Combo(id, &current, getStringFromVec, (void *)&list, list.size());
}

template <class T>
inline const char *getNamePoly(void *d, int i)
{
	if (i == 0)
		return "None";

	return ((EICreatorRegistry<T> *)d)->getName(i - 1);
}

template <class T>
void render(const char *baseId, Unigine::Vector<std::shared_ptr<T>> &elements, EICreatorRegistry<T> *registry,
	std::function<void(const char *, std::shared_ptr<T> &)> extraRender = nullptr)
{
	constexpr auto FLAGS = ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_SpanAllColumns;

	bool elementsOpen = ImGui::TreeNodeEx(FMT("%s##%s_node", registry->getRegistryName(), baseId),
		FLAGS | (!elements.empty() ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf));
	defer
	{
		if (elementsOpen)
			ImGui::TreePop();
	};
	ImGui::SameLine();
	if (ImGui::Button(FMT(ICON_FA_PLUS "##%s", baseId)))
		elements.append();

	if (elementsOpen)
	{
		for (int i = 0; i < elements.size();)
		{
			auto baseElId = FMT("%s[%i]", baseId, i);
			bool elementOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseElId.get()),
				FLAGS | (elements[i] ? ImGuiTreeNodeFlags_None : ImGuiTreeNodeFlags_Leaf));
			defer
			{
				if (elementOpen)
					ImGui::TreePop();
			};
			ImGui::SameLine();
			int ei = elements[i] ? registry->getIndex(elements[i]->getClassName()) + 1 : 0;
			ImGui::SetNextItemWidth(150);
			if (ImGui::Combo(FMT("##%s_combo", baseElId.get()), &ei, getNamePoly<T>, registry, registry->getCount() + 1))
			{
				elements[i].reset(ei != 0 ? registry->create(ei - 1) : nullptr);
			}
			ImGui::SameLine();
			if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseElId.get())))
			{
				elements.remove(i);
				continue;
			}

			if (elementOpen)
			{
				bool tableOpen = ImGui::BeginTable(FMT("##%s_table", baseElId.get()), 2);
				defer
				{
					if (tableOpen)
						ImGui::EndTable();
				};

				if (tableOpen)
				{
					ImGui::Input(FMT("##%s", baseElId.get()), elements[i].get());
					if (extraRender)
						extraRender(baseElId.get(), elements[i]);
				}
			}

			++i;
		}
	}
}
