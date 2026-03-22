#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include "EIFileManager.h"
#include <UnigineFileSystem.h>

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

	void setCacheEnabled(bool enabled)
	{
		_cache = enabled;
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

		if (_cache)
		{
			auto it = _loaded.find(_paths[i]);
			if (it != _loaded.end())
				return it->data;
		}

		auto v = new U;
		auto xml = Unigine::Xml::create();
		xml->load(_paths[i]);
		load(*v, xml);
		v->name = _names[i];
		v->guid = Unigine::FileSystem::getGUID(_paths[i]);

		if (_cache)
			_loaded[_paths[i]] = v;

		return v;
	}

	T *create(const char *name) override
	{
		return create(_names.findIndex(name));
	}

	T *create(const Unigine::UGUID &guid) override
	{
		for (int i = 0; i < _paths.size(); i++)
		{
			auto v = create(i);
			if (v && v->guid == guid)
				return v;
		}
		return nullptr;
	}

	void refresh() override
	{
		_names.clear();
		_paths.clear();

		Unigine::Vector<Unigine::String> paths;
		Unigine::FileSystem::getVirtualFiles(paths);

		Unigine::HashSet<Unigine::String> activePaths;
		for (const auto &path : paths)
		{
			if (path.extension() == _extension)
			{
				auto name = path.filename();
				if (!name.empty())
				{
					_names.append(name);
					_paths.append(path);
					activePaths.append(path);
				}
			}
		}

		// remove cached entries that no longer exist on disk
		Unigine::Vector<Unigine::String> stale;
		for (const auto &it : _loaded)
		{
			if (!activePaths.contains(it.key))
				stale.append(it.key);
		}
		for (const auto &key : stale)
		{
			delete _loaded[key];
			_loaded.remove(key);
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

		if (_cache)
		{
			int idx = getIndex(v);
			if (idx < 0)
				return;

			auto it = _loaded.find(getPath(idx));
			if (it != _loaded.end())
			{
				delete it->data;
				_loaded.erase(it);
			}
		} else
		{
			delete v;
		}
	}

	bool save(int i) override
	{
		if (!_cache)
			return false;

		auto v = create(i);
		auto p = dynamic_cast<U *>(v);
		return ::save(*p, _paths[i]);
	}

	bool save(T *v) override
	{
		int idx = getIndex(v);
		if (idx < 0)
			return false;

		auto p = dynamic_cast<U *>(v);
		return ::save(*p, _paths[idx]);
	}

	bool saveDummy(const char *path) override
	{
		auto v = new U;
		if (!::save(*v, path))
		{
			delete v;
			return false;
		}

		delete v;
		refresh();
		return true;
	}

private:
	bool _cache = true;
	Unigine::String _extension;
	Unigine::Vector<Unigine::String> _names;
	Unigine::Vector<Unigine::String> _paths;
	Unigine::HashMap<Unigine::String, T *> _loaded;
};
