#include "EIActionEditorWindow.h"
#include "UnigineImGui.h"

using namespace Unigine;
using namespace Unigine::Math;

void EIActionEditorWindow::onRender()
{
	if (ImGui::BeginTable("##ctx_props", 2))
	{
		auto registry = EISystem::get()->getActionRegistry();
		auto i = registry->getIndex(_action);

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Type");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Action");

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Name");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(registry->getName(i));

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Path");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted(registry->getPath(i));

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Description");
		ImGui::TableNextColumn();
		ImGui::InputText("##action_description", _action->description);

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Value type");
		ImGui::TableNextColumn();
		ImGui::ComboEnum("##action_value_type", _action->valueType);

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Accumulation behaior");
		ImGui::TableNextColumn();
		ImGui::ComboEnum("##action_accumulation_behavior", _action->accumulationBehavior);

		ImGui::EndTable();
	}

	render("action_modifiers", _action->modifiers, EISystem::get()->getModifierRegistry());
	render("action_triggers", _action->triggers, EISystem::get()->getTriggerRegistry());
}

void EIActionEditorWindow::saveToFile(const char *path) const
{
	auto registry = EISystem::get()->getActionRegistry();
	auto i = registry->getIndex(_action);
	if (i == -1)
	{
		Log::error("Can not save %s\n", path);
		return;
	}

	if (!registry->save(i))
	{
		Log::error("It's imposible save\n");
		return;
	}
}

void EIActionEditorWindow::loadFromFile(const char *path)
{
	_path = "";
	auto registry = EISystem::get()->getActionRegistry();
	auto i = registry->getIndexByPath(path);
	if (i == -1)
	{
		Log::error("Can not load %s\n", path);
		return;
	}

	_action = registry->create(i);
	if (!_action)
	{
		Log::error("It's imposible load\n");
		return;
	}

	_path = path;
}
