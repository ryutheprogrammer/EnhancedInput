#include "EIModifier.h"

EIActionValue EIModifierNegate::modify(EIActionValue v)
{
	vec3 m(x ? -1 : 1, y ? -1 : 1, z ? -1 : 1);
	v.value *= m;
	return v;
}

EIActionValue EIModifierScale::modify(EIActionValue v)
{
	vec3 m(x, y, z);
	v.value *= m;
	return v;
}

EIActionValue EIModifierSwizzleAxis::modify(EIActionValue v)
{
	switch (type)
	{
		case EISwizzleAxis::XZY: v.value = xzy(v.value); break;
		case EISwizzleAxis::YXZ: v.value = yxz(v.value); break;
		case EISwizzleAxis::YZX: v.value = yzx(v.value); break;
		case EISwizzleAxis::ZXY: v.value = zxy(v.value); break;
		case EISwizzleAxis::ZYX: v.value = zyx(v.value); break;
		default:
			break;
	}

	return v;
}