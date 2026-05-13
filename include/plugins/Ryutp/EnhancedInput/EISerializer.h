#pragma once
#include "Defines.h"
#include <UnigineString.h>
#include <type_traits>

class EISerializer
{
public:
	virtual ~EISerializer() = default;

	virtual void io(const char *name, bool &v) = 0;
	virtual void io(const char *name, int &v) = 0;
	virtual void io(const char *name, float &v) = 0;
	virtual void io(const char *name, Unigine::String &v) = 0;

	template <class E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
	void io(const char *name, E &v)
	{
		int tmp = static_cast<int>(v);
		ioEnum(name, tmp, Enum<E>::StringItems, Enum<E>::Count);
		v = static_cast<E>(tmp);
	}

protected:
	virtual void ioEnum(const char *name, int &v, const char *const *items, int count) = 0;
};
