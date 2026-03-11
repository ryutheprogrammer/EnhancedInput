#pragma once
#include <EnhancedInput/EnhancedInput.h>
#include "EIFileManager.h"
#include <UnigineFileSystem.h>

void autoregister(EIAction *action);
void autoregister(EIContext *context);

template <class T, class U = T, class = std::enable_if_t<std::is_base_of_v<T, U>, void *>>
class EIFileSystemRegistryImpl: public EIFileSystemRegistry<T>
{
private:
	EIFileSystemRegistryImpl() = default;

public:
	static EIFileSystemRegistryImpl *get()
	{
		static EIFileSystemRegistryImpl instance;
		return &instance;
	}

	void setExtension(const char *extension)
	{
		_extension = extension;
	}

	const char *getExtension() const override
	{
		return _extension;
	}

	int getIndexByName(const char *name) const override
	{
		return _names.findIndex(name);
	}
	int getIndexByPath(const char *name) const override
	{
		return _paths.findIndex(name);
	}
	int getCount() const override
	{
		return _names.size();
	}
	const char *getName(int i) const override
	{
		return i >= 0 && i < _names.size() ? _names[i].get() : nullptr;
	}
	const char *getPath(int i) const override
	{
		return i >= 0 && i < _paths.size() ? _paths[i].get() : nullptr;
	}

	T *create(int i) override
	{
		if (i < 0 || i >= _paths.size())
			return nullptr;

		auto it = _loaded.find(_paths[i]);
		if (it == _loaded.end())
		{
			auto v = new U;
			auto xml = Unigine::Xml::create();
			xml->load(_paths[i]);
			load(*v, xml);
			v->name = _names[i];
			_loaded[_paths[i]] = v;
			return v;
		}
		return it->data;
	}

	T *create(const char *name) override
	{
		return create(_names.findIndex(name));
	}

	void refresh() override
	{
		_names.clear();
		_paths.clear();

		Unigine::Vector<Unigine::String> paths;
		Unigine::FileSystem::getVirtualFiles(paths);

		for (const auto &path : paths)
		{
			if (path.extension() == _extension)
			{
				auto name = path.filename();
				if (!name.empty())
				{
					_names.append(name);
					_paths.append(path);
				}
			}
		}

		for (int i = 0; i < getCount(); ++i)
		{
			auto v = create(i);
			autoregister(v);
		}
	}

	const Unigine::Vector<Unigine::String> &getPaths() const noexcept { return _paths; }

	int getIndex(T *v) override
	{
		return _names.findIndex(v->name);
	}

	void destroy(T *v) override
	{
		if (!v)
			return;

		auto it = _loaded.find(getPath(getIndex(v)));
		if (it != _loaded.end())
		{
			delete it->data;
			_loaded.erase(it);
		}
	}

	bool save(int i) override
	{
		auto v = create(i);
		auto xml = Unigine::Xml::create();
		auto p = dynamic_cast<U *>(v);
		return ::save(*p, _paths[i]);
	}

	bool saveDummy(const char *path) override
	{
		auto v = new U;
		if (!::save(*v, path))
		{
			return false;
		}

		refresh();
		return true;
	}

private:
	Unigine::String _extension;
	Unigine::Vector<Unigine::String> _names;
	Unigine::Vector<Unigine::String> _paths;
	Unigine::HashMap<Unigine::String, T *> _loaded;
};
