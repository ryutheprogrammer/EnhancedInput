#include "EIFileManager.h"
#include "EISystem.h"
#include <UnigineLog.h>

#define SOS1(PTYPE, TYPE, XTYPE)                             \
	case Property::PARAMETER_##PTYPE:                        \
	{                                                        \
		auto p = dynamic_cast<ComponentVariable##TYPE *>(v); \
		*p = xml->get##XTYPE##Data();                        \
		break;                                               \
	}

#define SOS(PTYPE, TYPE) SOS1(PTYPE, TYPE, TYPE)

#define SAS1(PTYPE, TYPE, XTYPE)                                   \
	case Property::PARAMETER_##PTYPE:                              \
	{                                                              \
		auto p = dynamic_cast<const ComponentVariable##TYPE *>(v); \
		xml->set##XTYPE##Data(p->get());                           \
		break;                                                     \
	}

#define SAS(PTYPE, TYPE) SAS1(PTYPE, TYPE, TYPE)

using namespace Unigine;

static void save(const ComponentVariable *v, const Unigine::XmlPtr &xml)
{
	xml->setArg("name", v->getName());

	switch (v->getType())
	{
		SAS(INT, Int);
		SAS(FLOAT, Float);
		SAS(DOUBLE, Double);
		SAS1(TOGGLE, Toggle, Int);
		SAS1(SWITCH, Switch, Int);
		SAS1(STRING, String, );
		SAS1(COLOR, Color, Vec4);
		SAS(VEC2, Vec2);
		SAS(VEC3, Vec3);
		SAS(VEC4, Vec4);
		SAS(DVEC2, DVec2);
		SAS(DVEC3, DVec3);
		SAS(DVEC4, DVec4);
		SAS(IVEC2, IVec2);
		SAS(IVEC3, IVec3);
		SAS(IVEC4, IVec4);
		SAS1(MASK, Mask, Int);
		SAS1(FILE, File, );
		// SOS(PROPERTY); TODO
		// SOS(MATERIAL); TODO
		// SOS(NODE); TODO
		// SOS(CURVE2D); TODO
		// SOS(ARRAY); TODO
		// SOS(STRUCT); TODO
		default:
			break;
	}
}

static void save(const ComponentStruct *v, const Unigine::XmlPtr &xml)
{
	if (!v)
		return;

	for (const auto &variable : v->variables)
		save(variable, xml->addChild("parameter"));
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
static void load(ComponentVariable *v, const Unigine::XmlPtr &xml)
{
	switch (v->getType())
	{
		SOS(INT, Int);
		SOS(FLOAT, Float);
		SOS(DOUBLE, Double);
		SOS1(TOGGLE, Toggle, Int);
		SOS1(SWITCH, Switch, Int);
		SOS1(STRING, String, );
		SOS1(COLOR, Color, Vec4);
		SOS(VEC2, Vec2);
		SOS(VEC3, Vec3);
		SOS(VEC4, Vec4);
		SOS(DVEC2, DVec2);
		SOS(DVEC3, DVec3);
		SOS(DVEC4, DVec4);
		SOS(IVEC2, IVec2);
		SOS(IVEC3, IVec3);
		SOS(IVEC4, IVec4);
		SOS1(MASK, Mask, Int);
		SOS1(FILE, File, );
		// SOS(PROPERTY); TODO
		// SOS(MATERIAL); TODO
		// SOS(NODE); TODO
		// SOS(CURVE2D); TODO
		// SOS(ARRAY); TODO
		// SOS(STRUCT); TODO
		default:
			break;
	}
}

static void load(ComponentVariable *v, const HashMap<String, Unigine::XmlPtr> &xmls)
{
	auto it = xmls.find(v->getName());
	if (it == xmls.end())
		return;

	load(v, it->data);
}

static void load(ComponentStruct *v, const Unigine::XmlPtr &xml)
{
	if (!v)
		return;

	HashMap<String, Unigine::XmlPtr> xmls;
	for (int i = 0; i < xml->getNumChildren(); ++i)
		xmls[xml->getChild(i)->getArg("name")] = xml->getChild(i);

	for (const auto &variable : v->variables)
		load(variable, xmls);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

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

	for (const auto &mapping : v.mappings)
		save(mapping, xml->addChild("Mapping"));
}

void save(const EIMapping &v, const Unigine::XmlPtr &xml)
{
	xml->setArg("consume_input", String::itoa(v.consumeInput));

	save(v.binding, xml->addChild("Binding"));
	for (const auto &andKey : v.andKeys)
		save(andKey, xml->addChild("Binding"));
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
	save((const ComponentStruct *)v, xml);
}

void save(const EITrigger *v, const Unigine::XmlPtr &xml)
{
	xml->setArg("type", v ? v->getClassName() : "None");
	save((const ComponentStruct *)v, xml);
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
				if (n > 0)
				{
					load(mapping.binding, bindingsXml->getChild(0));
					for (int bi = 1; bi < n; ++bi)
					{
						EIKeyBinding andKey;
						load(andKey, bindingsXml->getChild(bi));
						mapping.andKeys.append(std::move(andKey));
					}
				}
			}
			else
			{
				// Even older format: <Key> + <Triggers> directly in <Mapping>
				auto key = mappingXml->getChild("Key");
				if (key)
				{
					mapping.binding.key = EIKey(key->getData());
					auto triggers = mappingXml->getChild("Triggers");
					if (triggers)
					{
						for (int ti = 0; ti < triggers->getNumChildren(); ++ti)
						{
							EITrigger *trigger = nullptr;
							load(&trigger, triggers->getChild(ti));
							if (trigger)
								mapping.binding.triggers.append(SPtr<EITrigger>(trigger));
						}
					}
				}
			}

			// Old format: modifiers at mapping level -> put on primary binding
			auto modifiers = mappingXml->getChild("Modifiers");
			if (modifiers)
			{
				for (int mi = 0; mi < modifiers->getNumChildren(); ++mi)
				{
					EIModifier *modifier = nullptr;
					load(&modifier, modifiers->getChild(mi));
					if (modifier)
						mapping.binding.modifiers.append(SPtr<EIModifier>(modifier));
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

	bool first = true;
	for (int i = 0; i < xml->getNumChildren(); ++i)
	{
		auto child = xml->getChild(i);
		if (String::compare(child->getName(), "Binding") == 0)
		{
			if (first)
			{
				load(v.binding, child);
				first = false;
			}
			else
			{
				EIKeyBinding andKey;
				load(andKey, child);
				v.andKeys.append(std::move(andKey));
			}
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
	load((ComponentStruct *)*v, xml);
}

void load(EITrigger **v, const Unigine::XmlPtr &xml)
{
	auto type = xml->getArg("type");
	if (String::compare(type, "None") == 0)
		return;

	*v = EISystem::get()->getTriggerRegistry()->create(type);
	load((ComponentStruct *)*v, xml);
}
