#include "EIFileManager.h"
#include "EISystem.h"
#include "EIXmlSerializer.h"
#include <UnigineLog.h>

using namespace Unigine;

bool save(const EIAction &v, const char *path)
{
	auto xml = Xml::create();
	save(v, xml);
	return xml->save(path);
}

bool save(const EIContextImpl &v, const char *path)
{
	auto xml = Xml::create();
	save(v, xml);
	return xml->save(path);
}

void save(const EIAction &v, const Unigine::XmlPtr &xml)
{
	xml->setName("EIAction");
	xml->setArg("description", v.description.get());
	xml->setArg("value_type", Enum<EIActionValueType>::toString(v.valueType));
	xml->setArg("accumulation_behavior", Enum<EIActionAccumulationBehavior>::toString(v.accumulationBehavior));

	auto modifiers = xml->addChild("Modifiers");
	for (const auto &modifier : v.modifiers)
		save(modifier.get(), modifiers->addChild("Modifier"));

	auto triggers = xml->addChild("Triggers");
	for (const auto &trigger : v.triggers)
		save(trigger.get(), triggers->addChild("Trigger"));
}

void save(const EIContextImpl &v, const Unigine::XmlPtr &xml)
{
	xml->setName("EIContext");
	xml->setArg("description", v.description);
	xml->setArg("auto_registration", String::itoa(v.autoRegistration));

	auto actions = xml->addChild("Actions");
	for (const auto &actionMappings : v.getActionMappings())
		save(actionMappings, actions->addChild("Action"));
}

void save(const EIActionMappings &v, const Unigine::XmlPtr &xml)
{
	if (v.action)
	{
		xml->setArg("guid", v.action->guid.makeString().get());
		xml->setData(v.action->name.get());
	}
	xml->setArg("description", v.description.get());

	for (const auto &mapping : v.mappings)
		save(mapping, xml->addChild("Mapping"));
}

void save(const EIMapping &v, const Unigine::XmlPtr &xml)
{
	xml->setArg("consume_input", String::itoa(v.consumeInput));

	// bindings[0] = primary, bindings[1..] = AND gates. Serialized in order.
	for (const auto &b : v.bindings)
		save(b, xml->addChild("Binding"));
}

void save(const EIKeyBinding &v, const Unigine::XmlPtr &xml)
{
	auto key = xml->addChild("Key");
	key->setData(v.key.getName().get());

	auto triggers = xml->addChild("Triggers");
	for (const auto &trigger : v.triggers)
		save(trigger.get(), triggers->addChild("Trigger"));

	auto modifiers = xml->addChild("Modifiers");
	for (const auto &modifier : v.modifiers)
		save(modifier.get(), modifiers->addChild("Modifier"));
}

void save(const EIModifier *v, const Unigine::XmlPtr &xml)
{
	xml->setArg("type", v ? v->getClassName() : "None");
	if (v)
	{
		EIXmlWriteSerializer s(xml);
		const_cast<EIModifier *>(v)->serialize(s);
	}
}

void save(const EITrigger *v, const Unigine::XmlPtr &xml)
{
	xml->setArg("type", v ? v->getClassName() : "None");
	if (v)
	{
		EIXmlWriteSerializer s(xml);
		const_cast<EITrigger *>(v)->serialize(s);
	}
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
bool load(EIAction &v, const Unigine::XmlPtr &xml)
{
	if (String::compare(xml->getName(), "EIAction"))
	{
		Log::error("Not an EIAction\n");
		return false;
	}

	v.description = xml->getArg("description");
	v.valueType = Enum<EIActionValueType>::fromString(xml->getArg("value_type"));
	v.accumulationBehavior = Enum<EIActionAccumulationBehavior>::fromString(xml->getArg("accumulation_behavior"));

	auto modifiers = xml->getChild("Modifiers");
	if (modifiers)
	{
		v.modifiers.resize(modifiers->getNumChildren());
		for (int i = 0; i < modifiers->getNumChildren(); ++i)
		{
			EIModifier *modifier = nullptr;
			load(&modifier, modifiers->getChild(i));
			v.modifiers[i].reset(modifier);
		}
	}

	auto triggers = xml->getChild("Triggers");
	if (triggers)
	{
		v.triggers.resize(triggers->getNumChildren());
		for (int i = 0; i < triggers->getNumChildren(); ++i)
		{
			EITrigger *trigger = nullptr;
			load(&trigger, triggers->getChild(i));
			v.triggers[i].reset(trigger);
		}
	}

	return true;
}

bool load(EIContextImpl &v, const Unigine::XmlPtr &xml)
{
	if (String::compare(xml->getName(), "EIContext"))
	{
		Log::error("Not an EIContext\n");
		return false;
	}

	v.description = xml->getArg("description");
	v.autoRegistration = String::atoi(xml->getArg("auto_registration"));

	// New format: <Actions><Action>...</Action></Actions>
	auto actions = xml->getChild("Actions");
	if (actions)
	{
		auto &am = v.getActionMappings();
		int n = actions->getNumChildren();
		am.resize(n);
		for (int i = 0; i < n; ++i)
			load(am[i], actions->getChild(i));
		return true;
	}

	// Backward compat: old format <Mappings><Mapping>...</Mapping></Mappings>
	auto mappings = xml->getChild("Mappings");
	if (mappings)
	{
		auto &am = v.getActionMappings();
		auto actionRegistry = EISystem::get()->getActionRegistry();

		for (int i = 0; i < mappings->getNumChildren(); ++i)
		{
			auto mappingXml = mappings->getChild(i);
			auto actionXml = mappingXml->getChild("Action");
			if (!actionXml)
				continue;

			const EIAction *action = nullptr;
			if (actionXml->isArg("guid"))
				action = actionRegistry->create(UGUID(actionXml->getArg("guid")));
			if (!action)
				action = actionRegistry->create(actionXml->getData());
			if (!action)
				continue;

			// Find or create EIActionMappings for this action
			EIActionMappings *entry = nullptr;
			for (auto &existing : am)
			{
				if (existing.action == action)
				{
					entry = &existing;
					break;
				}
			}
			if (!entry)
			{
				am.append({action});
				entry = &am.last();
			}

			EIMapping mapping;
			mapping.consumeInput = String::atoi(mappingXml->getArg("consume_input"));

			auto bindingsXml = mappingXml->getChild("Bindings");
			if (bindingsXml)
			{
				int n = bindingsXml->getNumChildren();
				for (int bi = 0; bi < n; ++bi)
				{
					EIKeyBinding b;
					load(b, bindingsXml->getChild(bi));
					mapping.bindings.append(std::move(b));
				}
			}
			else
			{
				// Even older format: <Key> + <Triggers> directly in <Mapping>
				auto key = mappingXml->getChild("Key");
				if (key)
				{
					EIKeyBinding primary;
					primary.key = EIKey(key->getData());
					auto triggers = mappingXml->getChild("Triggers");
					if (triggers)
					{
						for (int ti = 0; ti < triggers->getNumChildren(); ++ti)
						{
							EITrigger *trigger = nullptr;
							load(&trigger, triggers->getChild(ti));
							if (trigger)
								primary.triggers.append(SPtr<EITrigger>(trigger));
						}
					}
					mapping.bindings.append(std::move(primary));
				}
			}

			// Old format: modifiers at mapping level -> put on primary binding (bindings[0]).
			auto modifiers = mappingXml->getChild("Modifiers");
			if (modifiers && mapping.bindings.size() > 0)
			{
				for (int mi = 0; mi < modifiers->getNumChildren(); ++mi)
				{
					EIModifier *modifier = nullptr;
					load(&modifier, modifiers->getChild(mi));
					if (modifier)
						mapping.bindings[0].modifiers.append(SPtr<EIModifier>(modifier));
				}
			}

			entry->mappings.append(std::move(mapping));
		}
	}

	return true;
}

void load(EIActionMappings &v, const Unigine::XmlPtr &xml)
{
	auto actionRegistry = EISystem::get()->getActionRegistry();
	if (xml->isArg("guid"))
		v.action = actionRegistry->create(UGUID(xml->getArg("guid")));
	if (!v.action)
		v.action = actionRegistry->create(xml->getData());
	v.description = xml->getArg("description");

	for (int i = 0; i < xml->getNumChildren(); ++i)
	{
		auto child = xml->getChild(i);
		if (String::compare(child->getName(), "Mapping") == 0)
		{
			EIMapping mapping;
			load(mapping, child);
			v.mappings.append(std::move(mapping));
		}
	}
}

void load(EIMapping &v, const Unigine::XmlPtr &xml)
{
	v.consumeInput = String::atoi(xml->getArg("consume_input"));

	// All <Binding> entries go into v.bindings in order; bindings[0] is primary.
	for (int i = 0; i < xml->getNumChildren(); ++i)
	{
		auto child = xml->getChild(i);
		if (String::compare(child->getName(), "Binding") == 0)
		{
			EIKeyBinding b;
			load(b, child);
			v.bindings.append(std::move(b));
		}
	}
}

void load(EIKeyBinding &v, const Unigine::XmlPtr &xml)
{
	auto key = xml->getChild("Key");
	if (key)
		v.key = EIKey(key->getData());

	auto triggers = xml->getChild("Triggers");
	if (triggers)
	{
		v.triggers.resize(triggers->getNumChildren());
		for (int i = 0; i < triggers->getNumChildren(); ++i)
		{
			EITrigger *trigger = nullptr;
			load(&trigger, triggers->getChild(i));
			v.triggers[i].reset(trigger);
		}
	}

	auto modifiers = xml->getChild("Modifiers");
	if (modifiers)
	{
		v.modifiers.resize(modifiers->getNumChildren());
		for (int i = 0; i < modifiers->getNumChildren(); ++i)
		{
			EIModifier *modifier = nullptr;
			load(&modifier, modifiers->getChild(i));
			v.modifiers[i].reset(modifier);
		}
	}
}

void load(EIModifier **v, const Unigine::XmlPtr &xml)
{
	auto type = xml->getArg("type");
	if (String::compare(type, "None") == 0)
		return;

	*v = EISystem::get()->getModifierRegistry()->create(type);
	if (*v)
	{
		EIXmlReadSerializer s(xml);
		(*v)->serialize(s);
	}
}

void load(EITrigger **v, const Unigine::XmlPtr &xml)
{
	auto type = xml->getArg("type");
	if (String::compare(type, "None") == 0)
		return;

	*v = EISystem::get()->getTriggerRegistry()->create(type);
	if (*v)
	{
		EIXmlReadSerializer s(xml);
		(*v)->serialize(s);
	}
}
