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
	xml->setArg("description", v.desctiption.get());
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

	auto mappings = xml->addChild("Mappings");
	for (const auto &mapping : v.getMappings())
		save(mapping, mappings->addChild("Mapping"));
}

void save(const EIKeyActionMapping *v, const Unigine::XmlPtr &xml)
{
	auto action = xml->addChild("Action");
	action->setData(v->action->name.get());

	auto key = xml->addChild("Key");
	key->setData(v->key.getName().get());

	auto modifiers = xml->addChild("Modifiers");
	for (const auto &modifier : v->modifiers)
		save(modifier.get(), modifiers->addChild("Modifier"));

	auto triggers = xml->addChild("Triggers");
	for (const auto &trigger : v->triggers)
		save(trigger.get(), triggers->addChild("Trigger"));
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

	v.desctiption = xml->getArg("description");
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

	auto triggers = xml->getChild("Modifiers");
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

	auto mappings = xml->getChild("Mappings");
	if (mappings)
	{
		int n = mappings->getNumChildren();
		auto &m = v.getMappings();
		m.resize(n);

		for (int i = 0; i < n; ++i)
		{
			m[i] = new EIKeyActionMapping;
			load(m[i], mappings->getChild(i));
		}
	}

	return true;
}

void load(EIKeyActionMapping *v, const Unigine::XmlPtr &xml)
{
	auto action = xml->getChild("Action");
	v->action = EISystem::get()->getActionRegistry()->create(action->getData());

	auto key = xml->getChild("Key");
	v->key = EIKey(key->getData());

	auto modifiers = xml->getChild("Modifiers");
	if (modifiers)
	{
		v->modifiers.resize(modifiers->getNumChildren());
		for (int i = 0; i < modifiers->getNumChildren(); ++i)
		{
			EIModifier *modifier = nullptr;
			load(&modifier, modifiers->getChild(i));
			v->modifiers[i].reset(modifier);
		}
	}

	auto triggers = xml->getChild("Triggers");
	if (triggers)
	{
		v->triggers.resize(triggers->getNumChildren());
		for (int i = 0; i < triggers->getNumChildren(); ++i)
		{
			EITrigger *trigger = nullptr;
			load(&trigger, triggers->getChild(i));
			v->triggers[i].reset(trigger);
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
