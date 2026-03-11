#include "UnigineImGui.h"

#define INPUT_SCALAR(PARAM_TYPE, CType, IType, Type)                                         \
	case Unigine::Property::PARAMETER_##PARAM_TYPE:                                          \
	{                                                                                        \
		auto p = dynamic_cast<Unigine::ComponentVariable##CType *>(variable);                \
																							 \
		Type tmp = p->get();                                                                 \
		ImGui::TableNextColumn();                                                            \
		ImGui::TextUnformatted(variable->getName());                                         \
		ImGui::TableNextColumn();                                                            \
		ImGui::SetNextItemWidth(100);                                                        \
		ImGui::IType(Unigine::String::format("##%s_%s", baseId, variable->getName()), &tmp); \
		if (tmp != (Type)p->get())                                                           \
			*p = tmp;                                                                        \
		break;                                                                               \
	}

namespace ImGui
{

struct InputTextCallbackUnigineString_Data
{
	Unigine::String *Str;
	ImGuiInputTextCallback ChainCallback;
	void *ChainCallbackUserData;
};

static int InputTextCallbackUnigineString(ImGuiInputTextCallbackData *data)
{
	InputTextCallbackUnigineString_Data *user_data = (InputTextCallbackUnigineString_Data *)data->UserData;
	if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
	{
		Unigine::String *str = user_data->Str;
		IM_ASSERT(data->Buf == str->get());
		str->resize(data->BufTextLen);
		data->Buf = (char *)str->get();
	} else if (user_data->ChainCallback)
	{
		data->UserData = user_data->ChainCallbackUserData;
		return user_data->ChainCallback(data);
	}
	return 0;
}

bool InputText(const char *label, Unigine::String &str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void *user_data)
{
	IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallbackUnigineString_Data cb_user_data;
	cb_user_data.Str = &str;
	cb_user_data.ChainCallback = callback;
	cb_user_data.ChainCallbackUserData = user_data;
	return InputText(label, (char *)str.get(), str.space() + 1, flags, InputTextCallbackUnigineString, &cb_user_data);
}

void Input(const char *baseId, Unigine::ComponentVariable *variable)
{
	if (!variable)
		return;

	switch (variable->getType())
	{
		INPUT_SCALAR(TOGGLE, Toggle, Checkbox, bool);
		INPUT_SCALAR(INT, Int, InputInt, int);
		INPUT_SCALAR(FLOAT, Float, InputFloat, float);
		INPUT_SCALAR(DOUBLE, Double, InputDouble, double);
		case Unigine::Property::PARAMETER_SWITCH:
		{
			auto p = dynamic_cast<Unigine::ComponentVariableSwitch *>(variable);

			char *buff = strdup(p->getItems());
			for (char *p = buff; *p; ++p)
			{
				if (*p == ',')
					*p = '\0';
			}

			int tmp = p->get();

			ImGui::TableNextColumn();
			ImGui::TextUnformatted(variable->getName());
			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(100);
			ImGui::Combo(Unigine::String::format("##%s_%s", baseId, variable->getName()), &tmp, buff);
			if (tmp != p->get())
			{
				*p = tmp;
			}

			delete buff;
		}
		default:
			break;
	}
}

void Input(const char *baseId, Unigine::ComponentStruct *structure)
{
	if (!structure)
		return;

	for (const auto &variable : structure->variables)
		Input(baseId, variable);
}
} // namespace ImGui
