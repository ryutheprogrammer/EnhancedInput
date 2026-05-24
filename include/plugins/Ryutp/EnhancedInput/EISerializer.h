#pragma once
#include "Defines.h"
#include <UnigineString.h>
#include <UnigineCurve2d.h>

class EISerializer
{
public:
	virtual ~EISerializer() = default;

	virtual void io(const char *name, bool &v) = 0;
	virtual void io(const char *name, int &v) = 0;
	virtual void io(const char *name, float &v) = 0;
	virtual void io(const char *name, Unigine::String &v) = 0;
	// Curves are always heap objects held via Ptr<> — pass the smart pointer
	// itself by const-ref. Implementations mutate the curve through ->, which
	// remains legal because const Ptr& doesn't propagate const to the pointee.
	virtual void io(const char *name, const Unigine::Curve2dPtr &v) = 0;

	// String editor hint for long free-form text. UI serializers should render
	// a tall multi-line text area; XML/serializers just persist as a string,
	// so the default falls back to the single-line io().
	virtual void ioMultiline(const char *name, Unigine::String &v) { io(name, v); }

	template <class E, std::enable_if_t<std::is_enum<E>::value, int> = 0>
	void io(const char *name, E &v)
	{
		// ENUM-macro enums use int as underlying type. Bind ioEnum's int& to
		// the enum's storage directly — Qt serializers register a widget that
		// writes back asynchronously when the user changes it, so a local
		// staging int would be a dangling reference by then.
		static_assert(sizeof(E) == sizeof(int),
			"ENUM macro is expected to produce an int-sized enum");
		ioEnum(name, reinterpret_cast<int &>(v),
			Enum<E>::StringItems, Enum<E>::Count);
	}

protected:
	virtual void ioEnum(const char *name, int &v, const char *const *items, int count) = 0;
};
