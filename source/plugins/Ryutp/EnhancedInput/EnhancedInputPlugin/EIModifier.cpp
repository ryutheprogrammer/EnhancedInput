#include "EIModifier.h"

EIActionValue EIModifierNegate::modify(EIActionValue v)
{
	v.value.x = x ? -v.value.x : v.value.x;
	v.value.y = y ? -v.value.y : v.value.y;
	v.value.z = z ? -v.value.z : v.value.z;
	return v;
}

EIActionValue EIModifierScale::modify(EIActionValue v)
{
	v.value.x *= x;
	v.value.y *= y;
	v.value.z *= z;
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
