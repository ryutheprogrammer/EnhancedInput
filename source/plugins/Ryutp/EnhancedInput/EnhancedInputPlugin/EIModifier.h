#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

using namespace Unigine;
using namespace Unigine::Math;

class EIModifierNegate: public EIModifier
{
public:
	PROP_PARAM(Toggle, x, true)
	PROP_PARAM(Toggle, y, true)
	PROP_PARAM(Toggle, z, true)

	const char *getClassName() const noexcept override { return "Negate"; }

	EIActionValue modify(EIActionValue v) override;
};

class EIModifierScale: public EIModifier
{
public:
	PROP_PARAM(Float, x, true)
	PROP_PARAM(Float, y, true)
	PROP_PARAM(Float, z, true)

	const char *getClassName() const noexcept override { return "Scale"; }

	EIActionValue modify(EIActionValue v) override;
};

ENUM(EISwizzleAxis, XYZ, XZY, YXZ, YZX, ZXY, ZYX);

class EIModifierSwizzleAxis: public EIModifier
{
public:
	PROP_PARAM(Switch, type, EISwizzleAxis::XYZ, Enum<EISwizzleAxis>::StringSwitch)

	const char *getClassName() const noexcept override { return "Swizzle Axis"; }

	EIActionValue modify(EIActionValue v) override;
};
