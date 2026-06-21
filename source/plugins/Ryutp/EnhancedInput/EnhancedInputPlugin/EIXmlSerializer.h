#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <UnigineHashMap.h>
#include <UnigineXml.h>

class EIXmlReadSerializer: public EISerializer
{
public:
	EIXmlReadSerializer(const Unigine::XmlPtr &xml)
	{
		if (!xml)
			return;

		for (int i = 0; i < xml->getNumChildren(); ++i)
		{
			auto c = xml->getChild(i);
			if (Unigine::String::compare(c->getName(), "parameter") == 0)
				_params[c->getArg("name")] = c;
		}
	}

	using EISerializer::io;

	void io(const char *name, bool &v) override
	{
		if (auto x = find(name))
			v = x->getIntData() != 0;
	}
	void io(const char *name, int &v) override
	{
		if (auto x = find(name))
			v = x->getIntData();
	}
	void io(const char *name, float &v) override
	{
		if (auto x = find(name))
			v = x->getFloatData();
	}
	void io(const char *name, Unigine::String &v) override
	{
		if (auto x = find(name))
			v = x->getData();
	}
	void io(const char *name, const Unigine::Curve2dPtr &v) override
	{
		if (!v)
			return;
		if (auto x = find(name))
		{
			// Curve2d::load appears to append; without an explicit clear the
			// ctor-installed default keys would stack on top of the loaded
			// state. Cheap call, removes the ambiguity.
			v->clear();
			v->load(x);
		}
	}

protected:
	void ioEnum(const char *name, int &v, const char *const *items, int count) override
	{
		auto x = find(name);
		if (!x)
			return;

		const char *s = x->getData();
		for (int i = 0; i < count; ++i)
		{
			if (Unigine::String::compare(items[i], s) == 0)
			{
				v = i;
				return;
			}
		}
	}

private:
	// Renames in public API → fallback to old typo'd names for existing data files.
	static const char *legacyAlias(const char *name)
	{
		struct A
		{
			const char *current;
			const char *legacy;
		};
		static constexpr A aliases[] = {
			{"threshold", "treshold"},
			{"holdThreshold", "holdTreshold"},
			{"tapReleaseTime", "tapReleaseTimeTreshold"},
		};
		for (const auto &a : aliases)
			if (Unigine::String::compare(a.current, name) == 0)
				return a.legacy;
		return nullptr;
	}

	Unigine::XmlPtr find(const char *name)
	{
		auto it = _params.find(name);
		if (it != _params.end())
			return it->data;
		if (const char *legacy = legacyAlias(name))
		{
			it = _params.find(legacy);
			if (it != _params.end())
				return it->data;
		}
		return Unigine::XmlPtr{};
	}

	Unigine::HashMap<Unigine::String, Unigine::XmlPtr> _params;
};

class EIXmlWriteSerializer: public EISerializer
{
public:
	EIXmlWriteSerializer(const Unigine::XmlPtr &xml)
		: _xml(xml)
	{
	}

	using EISerializer::io;

	void io(const char *name, bool &v) override { param(name)->setIntData(v ? 1 : 0); }
	void io(const char *name, int &v) override { param(name)->setIntData(v); }
	void io(const char *name, float &v) override { param(name)->setFloatData(v); }
	void io(const char *name, Unigine::String &v) override { param(name)->setData(v.get()); }
	void io(const char *name, const Unigine::Curve2dPtr &v) override
	{
		if (v)
			v->save(param(name));
	}

protected:
	void ioEnum(const char *name, int &v, const char *const *items, int count) override
	{
		if (v >= 0 && v < count)
			param(name)->setData(items[v]);
	}

private:
	Unigine::XmlPtr param(const char *name)
	{
		auto c = _xml->addChild("parameter");
		c->setArg("name", name);
		return c;
	}

	Unigine::XmlPtr _xml;
};
