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

	int mappingToRemove = -1;

	bool mappingsOpen = ImGui::TreeNodeEx("Mappings##context_mappings", commonFlags);
	defer
	{
		if (mappingsOpen)
			ImGui::TreePop();
	};

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_PLUS "##ctx_mapping_add"))
	{
		_context->getMappings().append(new EIKeyActionMapping);
	}

	if (mappingsOpen)
	{
		auto &allMappings = _context->getMappings();
		for (int i = 0; i < allMappings.size(); ++i)
		{
			auto mapping = allMappings[i];
			auto baseId = FMT("ctx_mapping[%i]", i);

			bool mappingOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseId.get()), commonFlags);
			defer
			{
				if (mappingOpen)
					ImGui::TreePop();
			};

			// Action combo
			ImGui::SameLine();
			auto actionRegistry = EISystem::get()->getActionRegistry();
			int ai = mapping->action ? actionRegistry->getIndexByName(mapping->action->name) + 1 : 0;
			ImGui::SetNextItemWidth(150);
			if (ImGui::Combo(FMT("##%s_action", baseId.get()), &ai, getActionNameWrap, nullptr, actionRegistry->getCount() + 1))
			{
				mapping->action = ai != 0 ? actionRegistry->create(ai - 1) : nullptr;
			}

			ImGui::SameLine();
			if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseId.get())))
			{
				mappingToRemove = i;
			}

			if (mappingOpen)
			{
				// Consume Input
				bool rc = ImGui::TreeNodeEx(FMT("Consume Input##%s_consume", baseId.get()),
					ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_Framed |
						ImGuiTreeNodeFlags_DrawLinesToNodes | ImGuiTreeNodeFlags_SpanAllColumns |
						ImGuiTreeNodeFlags_Leaf);
				if (rc)
				{
					ImGui::SameLine();
					ImGui::Checkbox(FMT("##%s_consume_cb", baseId.get()), &mapping->consumeInput);
					ImGui::TreePop();
				}

				// Bindings
				int bindingToRemove = -1;
				bool bindingsOpen = ImGui::TreeNodeEx(FMT("Bindings##%s_bindings", baseId.get()), commonFlags);
				defer
				{
					if (bindingsOpen)
						ImGui::TreePop();
				};

				ImGui::SameLine();
				if (ImGui::Button(FMT(ICON_FA_PLUS "##%s_binding_add", baseId.get())))
				{
					mapping->bindings.append({});
				}

				if (bindingsOpen)
				{
					for (int bi = 0; bi < mapping->bindings.size(); ++bi)
					{
						auto &binding = mapping->bindings[bi];
						auto baseBindId = FMT("%s_binding[%i]", baseId.get(), bi);

						bool bindingOpen = ImGui::TreeNodeEx(FMT("##%s_node", baseBindId.get()), commonFlags);
						defer
						{
							if (bindingOpen)
								ImGui::TreePop();
						};

						// Key combo
						ImGui::SameLine();
						ImGui::SetNextItemWidth(200);
						if (ImGui::BeginCombo(FMT("##%s_key", baseBindId.get()), binding.key.getName()))
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

						// Key capture
						ImGui::SameLine();
						{
							static ImGuiID captureId = 0;
							static bool captureSkipFrame = false;
							ImGuiID thisId = ImGui::GetID(FMT("##%s_capture", baseBindId.get()).get());
							if (captureId == thisId)
							{
								ImGui::Button(FMT(ICON_FA_ELLIPSIS "##%s_capture", baseBindId.get()));
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
								if (ImGui::Button(FMT(ICON_FA_KEYBOARD "##%s_capture", baseBindId.get())))
								{
									captureId = thisId;
									captureSkipFrame = true;
								}
							}
						}

						ImGui::SameLine();
						if (ImGui::Button(FMT(ICON_FA_TRASH_CAN "##%s_remove", baseBindId.get())))
						{
							bindingToRemove = bi;
						}

						if (bindingOpen)
						{
							render(FMT("%s_triggers", baseBindId.get()), binding.triggers, EISystem::get()->getTriggerRegistry());
						}
					}
				}

				if (bindingToRemove >= 0)
					mapping->bindings.remove(bindingToRemove);

				// Modifiers (mapping-level)
				render(FMT("%s_modifiers", baseId.get()), mapping->modifiers, EISystem::get()->getModifierRegistry());
			}
		}
	}

	if (mappingToRemove >= 0)
	{
		auto &m = _context->getMappings();
		delete m[mappingToRemove];
		m.remove(mappingToRemove);
	}
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
