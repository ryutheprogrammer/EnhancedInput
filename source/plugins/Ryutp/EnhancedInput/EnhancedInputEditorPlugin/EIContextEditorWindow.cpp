#include "EIContextEditorWindow.h"
#include "UnigineImGui.h"

using namespace Unigine;

EIContextEditorWindow::~EIContextEditorWindow()
{
	if (_context)
		EISystem::get()->getContextRegistry()->destroy(_context);
}

static const char *getActionNameWrap(void *, int i)
{
	if (i == 0)
		return "None";
	return EISystem::get()->getActionRegistry()->getName(i - 1);
}

static void renderKeyComboAndCapture(const char *baseId, EIKeyBinding &binding)
{
	ImGui::SetNextItemWidth(200);
	if (ImGui::BeginCombo(FMT("##%s_key", baseId), binding.key.getName()))
	{
		auto &keys = EIKey::getKeys();
		auto &names = EIKey::getKeysNames();

		const char *currentCat = nullptr;
		bool catOpen = false;

		for (int ki = 0; ki < keys.size(); ++ki)
		{
			const char *cat = keys[ki].getCategoryName();
			if (cat != currentCat)
			{
				if (catOpen)
					ImGui::TreePop();
				currentCat = cat;
				catOpen = cat ? ImGui::TreeNodeEx(cat, ImGuiTreeNodeFlags_SpanAvailWidth) : false;
			}

			if (cat && !catOpen)
				continue;

			bool sel = (keys[ki] == binding.key);
			if (ImGui::Selectable(FMT("%s##key_%i", names[ki].get(), ki), sel))
				binding.key = keys[ki];
			if (sel)
				ImGui::SetItemDefaultFocus();
		}

		if (catOpen)
			ImGui::TreePop();

		ImGui::EndCombo();
	}

	ImGui::SameLine();
	{
		static ImGuiID captureId = 0;
		static bool captureSkipFrame = false;
		ImGuiID thisId = ImGui::GetID(FMT("##%s_capture", baseId).get());
		if (captureId == thisId)
		{
			ImGui::Button(FMT(ICON_FA_ELLIPSIS "##%s_capture", baseId));
			if (captureSkipFrame)
			{
				captureSkipFrame = false;
			} else if (Input::isKeyPressed(Input::KEY::KEY_ESC))
			{
				captureId = 0;
			} else
			{
				auto &keys = EIKey::getKeys();
				for (int ki = 1; ki < keys.size(); ++ki)
				{
					if (keys[ki].isAxis() || keys[ki].isGamepad())
						continue;
					if (keys[ki].getValue() != 0.0f)
					{
						binding.key = keys[ki];
						captureId = 0;
						break;
					}
				}
			}
		} else
		{
			if (ImGui::Button(FMT(ICON_FA_KEYBOARD "##%s_capture", baseId)))
			{
				captureId = thisId;
				captureSkipFrame = true;
			}
		}
	}
}

static void renderBindingContents(const char *baseId, EIKeyBinding &binding)
{
	render(FMT("%s_triggers", baseId), binding.triggers, EISystem::get()->getTriggerRegistry());
	render(FMT("%s_modifiers", baseId), binding.modifiers, EISystem::get()->getModifierRegistry());
}

void EIContextEditorWindow::onRender()
{
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

	int actionToRemove = -1;

	bool actionsOpen = ImGui::TreeNodeEx("Actions##context_actions", commonFlags);
	defer
	{
		if (actionsOpen)
			ImGui::TreePop();
	};

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_PLUS "##action_add"))
	{
		_context->getActionMappings().append({});
	}

	if (actionsOpen)
	{
		auto &allActions = _context->getActionMappings();
		for (int ai = 0; ai < allActions.size(); ++ai)
		{
			auto &actionEntry = allActions[ai];
			auto baseActionId = FMT("action[%i]", ai);

			bool actionOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseActionId.get()), commonFlags);
			defer
			{
				if (actionOpen)
					ImGui::TreePop();
			};

			// Action combo
			ImGui::SameLine();
			auto actionRegistry = EISystem::get()->getActionRegistry();
			int actionIdx = actionEntry.action ? actionRegistry->getIndexByName(actionEntry.action->name) + 1 : 0;
			ImGui::SetNextItemWidth(150);
			if (ImGui::Combo(FMT("##%s_combo", baseActionId.get()), &actionIdx, getActionNameWrap, nullptr, actionRegistry->getCount() + 1))
			{
				actionEntry.action = actionIdx != 0 ? actionRegistry->create(actionIdx - 1) : nullptr;
			}

			// Add mapping
			ImGui::SameLine();
			if (ImGui::Button(FMT(ICON_FA_PLUS "##%s_add_mapping", baseActionId.get())))
			{
				actionEntry.mappings.append({});
			}

			// Remove action
			ImGui::SameLine();
			if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseActionId.get())))
			{
				actionToRemove = ai;
			}

			if (actionOpen)
			{
				int mappingToRemove = -1;

				for (int mi = 0; mi < actionEntry.mappings.size(); ++mi)
				{
					auto &mapping = actionEntry.mappings[mi];
					auto baseMappingId = FMT("%s_mapping[%i]", baseActionId.get(), mi);
					bool isAnd = !mapping.andKeys.empty();

					if (isAnd)
					{
						// AND combo: show combined key names as label
						String label = mapping.binding.key.getName();
						for (int ak = 0; ak < mapping.andKeys.size() && ak < 2; ++ak)
							label += String::format(" + %s", mapping.andKeys[ak].key.getName().get());

						if (mapping.andKeys.size() >= 3)
							label += " + ...";

						bool mappingOpen = ImGui::TreeNodeEx(FMT("%s##%s_node", label.get(), baseMappingId.get()), commonFlags);
						defer
						{
							if (mappingOpen)
								ImGui::TreePop();
						};

						ImGui::SameLine();
						if (ImGui::Button(FMT(ICON_FA_PLUS " AND##%s_add_and", baseMappingId.get())))
							mapping.andKeys.append({});

						ImGui::SameLine();
						if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseMappingId.get())))
							mappingToRemove = mi;

						if (mappingOpen)
						{
							int andToRemove = -1;

							// Primary binding
							{
								auto baseBind = FMT("%s_primary", baseMappingId.get());
								bool bindOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseBind.get()), commonFlags);
								defer
								{
									if (bindOpen)
										ImGui::TreePop();
								};

								ImGui::SameLine();
								renderKeyComboAndCapture(baseBind.get(), mapping.binding);

								if (bindOpen)
									renderBindingContents(baseBind.get(), mapping.binding);
							}

							// AND keys
							for (int aki = 0; aki < mapping.andKeys.size(); ++aki)
							{
								auto baseAndId = FMT("%s_and[%i]", baseMappingId.get(), aki);
								bool andOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseAndId.get()), commonFlags);
								defer
								{
									if (andOpen)
										ImGui::TreePop();
								};

								ImGui::SameLine();
								renderKeyComboAndCapture(baseAndId.get(), mapping.andKeys[aki]);

								ImGui::SameLine();
								if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseAndId.get())))
									andToRemove = aki;

								if (andOpen)
									renderBindingContents(baseAndId.get(), mapping.andKeys[aki]);
							}

							if (andToRemove >= 0)
								mapping.andKeys.remove(andToRemove);

							// Consume Input
							bool rc = ImGui::TreeNodeEx(FMT("Consume Input##%s_consume", baseMappingId.get()),
								ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed |
									ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_SpanAllColumns |
									ImGuiTreeNodeFlags_Leaf);
							if (rc)
							{
								ImGui::SameLine();
								ImGui::Checkbox(FMT("##%s_consume_cb", baseMappingId.get()), &mapping.consumeInput);
								ImGui::TreePop();
							}
						}
					} else
					{
						// Single key mapping
						bool mappingOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseMappingId.get()), commonFlags);
						defer
						{
							if (mappingOpen)
								ImGui::TreePop();
						};

						ImGui::SameLine();
						renderKeyComboAndCapture(baseMappingId.get(), mapping.binding);

						ImGui::SameLine();
						if (ImGui::Button(FMT(ICON_FA_PLUS " AND##%s_add_and", baseMappingId.get())))
							mapping.andKeys.append({});

						ImGui::SameLine();
						if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseMappingId.get())))
							mappingToRemove = mi;

						if (mappingOpen)
						{
							renderBindingContents(baseMappingId.get(), mapping.binding);

							// Consume Input
							bool rc = ImGui::TreeNodeEx(FMT("Consume Input##%s_consume", baseMappingId.get()),
								ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed |
									ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_SpanAllColumns |
									ImGuiTreeNodeFlags_Leaf);
							if (rc)
							{
								ImGui::SameLine();
								ImGui::Checkbox(FMT("##%s_consume_cb", baseMappingId.get()), &mapping.consumeInput);
								ImGui::TreePop();
							}
						}
					}
				}

				if (mappingToRemove >= 0)
					actionEntry.mappings.remove(mappingToRemove);
			}
		}
	}

	if (actionToRemove >= 0)
		_context->getActionMappings().remove(actionToRemove);
}

void EIContextEditorWindow::saveToFile(const char *path) const
{
	auto registry = EISystem::get()->getContextRegistry();
	if (!registry->save(_context))
	{
		Log::error("Can not save %s\n", path);
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
