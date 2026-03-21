#include "EIContextEditorWindow.h"
#include "UnigineImGui.h"

using namespace Unigine;

static const char *getActionNameWrap(void *, int i)
{
	if (i == 0)
		return "None";
	return EISystem::get()->getActionRegistry()->getName(i - 1);
}

EIContextEditorWindow::~EIContextEditorWindow()
{
	EISystem::get()->getContextRegistry()->destroy(_context);
}

void EIContextEditorWindow::onRender()
{
	HashMap<const EIAction *, Vector<EIKeyActionMapping *>> mappingsMap;
	bool hasNone = false;

	Vector<const EIAction *> actionRemove;
	Vector<Pair<const EIAction *, EIKey>> actionKeyRemove;
	Vector<const EIAction *> actionKeyAdd;

	for (const auto &x : _context->getMappings())
	{
		mappingsMap[x->action].append(x);
		hasNone |= !x->action;
	}

	if (ImGui::BeginTable("##ctx_props", 2))
	{
		auto registry = EISystem::get()->getContextRegistry();
		auto i = registry->getIndex(_context);

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Type");
		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Context");

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
		ImGui::InputText("##ctx_description", _context->description);

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Auto registration");
		ImGui::TableNextColumn();
		ImGui::Checkbox("##ctx_auto_registration", &_context->autoRegistration);

		ImGui::EndTable();
	}

	const int commonFlags = ImGuiTreeNodeFlags_AllowItemOverlap |
							ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DrawLinesToNodes |
							ImGuiTreeNodeFlags_SpanAvailWidth;
	bool mappingsOpen = ImGui::TreeNodeEx("Mappings##context_mappings", commonFlags);
	defer
	{
		if (mappingsOpen)
			ImGui::TreePop();
	};

	ImGui::SameLine();
	ImGui::BeginDisabled(hasNone);
	if (ImGui::Button(ICON_FA_PLUS "##ctx_action_add"))
	{
		_context->getMappings().append(new EIKeyActionMapping);
	}
	ImGui::EndDisabled();

	if (hasNone && ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
	{
		ImGui::SetTooltip("None already exists");
	}

	if (mappingsOpen)
	{
		int i = -1;
		for (auto &it : mappingsMap)
		{
			++i;
			auto &mappings = it.data;

			auto baseId = FMT("ctx_action[%i]", i);

			auto action = it.key;

			bool actionOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseId.get()), commonFlags);
			defer
			{
				if (actionOpen)
					ImGui::TreePop();
			};

			ImGui::SameLine();
			int ai = action ? EISystem::get()->getActionRegistry()->getIndexByName(action->name) + 1 : 0;
			ImGui::SetNextItemWidth(150);
			if (ImGui::Combo(FMT("##%s_combo", baseId.get()), &ai, getActionNameWrap, nullptr, EISystem::get()->getActionRegistry()->getCount() + 1))
			{
				auto newAction = ai != 0 ? EISystem::get()->getActionRegistry()->create(ai - 1) : nullptr;

				for (auto &mapping : mappings)
					mapping->action = newAction;
			}

			ImGui::SameLine();
			if (ImGui::Button(FMT(ICON_FA_PLUS "##%s_remove", baseId.get())))
			{
				actionKeyAdd.append(action);
			}

			ImGui::SameLine();
			if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseId.get())))
			{
				actionRemove.append(action);
			}

			if (actionOpen)
			{
				int j = -1;
				for (auto &mapping : mappings)
				{
					++j;

					auto baseMapId = FMT("%s_mapping[%i]", baseId.get(), j);

					bool mappingOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseMapId.get()), commonFlags);
					defer
					{
						if (mappingOpen)
							ImGui::TreePop();
					};

					ImGui::SameLine();
					int mi = EIKey::getKeys().findIndex(mapping->key);
					ImGui::SetNextItemWidth(150);
					if (Combo(FMT("##%s_combo", baseMapId.get()), mi, EIKey::getKeysNames()))
					{
						mapping->key = EIKey::getKeys()[mi];
					}
					ImGui::SameLine();
					if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseMapId.get())))
					{
						actionKeyRemove.append({mapping->action, mapping->key});
					}

					if (mappingOpen)
					{
						render(FMT("%s_modifiers", baseMapId.get()), mapping->modifiers, EISystem::get()->getModifierRegistry());
						render(FMT("%s_triggers", baseMapId.get()), mapping->triggers, EISystem::get()->getTriggerRegistry());
					}
				}
			}
		}
	}


	for (const auto &x : actionRemove)
		_context->unmap(x);

	for (const auto &x : actionKeyRemove)
		_context->unmap(x.first, x.second);

	for (const auto &x : actionKeyAdd)
		_context->map(x, EIKey());
}

void EIContextEditorWindow::saveToFile(const char *path) const
{
	auto registry = EISystem::get()->getContextRegistry();
	auto i = registry->getIndex(_context);
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

void EIContextEditorWindow::loadFromFile(const char *path)
{
	_path = "";
	auto registry = EISystem::get()->getContextRegistry();
	auto i = registry->getIndexByPath(path);
	if (i == -1)
	{
		Log::error("Can not load %s\n", path);
		return;
	}

	_context = registry->create(i);
	if (!_context)
	{
		Log::error("It's imposible load\n");
		return;
	}

	_path = path;
}
