#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

template <class T>
class EICreatorRegistryImpl: public EICreatorRegistry<T>
{
private:
	EICreatorRegistryImpl()
		: _names()
		, _creators()
		, _defaultCreator()
	{
		_defaultCreator = []() {
			return nullptr;
		};
	}

public:
	static EICreatorRegistryImpl *get()
	{
		static EICreatorRegistryImpl instance;
		return &instance;
	}

	void setRegistryName(const char *name) { _name = name; }
	const char *getRegistryName() const noexcept override { return _name; }

	void registerCreator(const char *name, std::function<T *()> creator) override
	{
		auto i = _names.findIndex(name);
		if (i == -1)
		{
			_names.append(name);
			_creators.append(creator);
		} else
		{
			_creators[i] = creator;
		}
	}

	void unregisterCreator(const char *name) override
	{
		auto i = _names.findIndex(name);
		if (i != -1)
		{
			_names.remove(i);
			_creators.remove(i);
		}
	}

	int getIndex(const char *name) const override
	{
		auto i = _names.findIndex(name);
		return i;
	}

	int getCount() const override
	{
		return _names.size();
	}

	const char *getName(int i) const override
	{
		return _names[i];
	}

	T *create(int i) override
	{
		return _creators[i]();
	}

	T *create(const char *name) override
	{
		auto i = _names.findIndex(name);
		if (i == -1)
			return nullptr;
		return _creators[i]();
	}

	const Unigine::Vector<Unigine::String> &getNames() const noexcept override { return _names; }

private:
	Unigine::String _name;
	Unigine::Vector<Unigine::String> _names;
	Unigine::Vector<std::function<T *()>> _creators;
	std::function<T *()> _defaultCreator;
};
