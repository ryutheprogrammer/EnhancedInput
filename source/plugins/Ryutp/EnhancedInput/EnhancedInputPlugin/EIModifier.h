#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class EIModifierNegate: public EIModifier
{
public:
	bool x = true;
	bool y = true;
	bool z = true;

	const char *getClassName() const noexcept override { return "Negate"; }

	EIActionValue modify(EIActionValue v) override;

	void serialize(EISerializer &s) override
	{
		s.io("x", x);
		s.io("y", y);
		s.io("z", z);
	}
};

class EIModifierScale: public EIModifier
{
public:
	float x = 1.0f;
	float y = 1.0f;
	float z = 1.0f;

	const char *getClassName() const noexcept override { return "Scale"; }

	EIActionValue modify(EIActionValue v) override;

	void serialize(EISerializer &s) override
	{
		s.io("x", x);
		s.io("y", y);
		s.io("z", z);
	}
};

ENUM(EISwizzleAxis, XYZ, XZY, YXZ, YZX, ZXY, ZYX);

class EIModifierSwizzleAxis: public EIModifier
{
public:
	EISwizzleAxis type = EISwizzleAxis::XYZ;

	const char *getClassName() const noexcept override { return "Swizzle Axis"; }

	EIActionValue modify(EIActionValue v) override;

	void serialize(EISerializer &s) override
	{
		s.io("type", type);
	}
};
