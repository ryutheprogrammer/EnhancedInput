#include "EIModifier.h"
#include "EITestHooks.h"
#include <UnigineGame.h>
#include <UniginePlayers.h>
#include <UnigineMathLib.h>

using namespace Unigine;
using namespace Unigine::Math;

namespace
{

// Number of meaningful axes for a given value type. Boolean/Axis1D use only x.
inline int axisCount(EIActionValueType t)
{
	switch (t)
	{
		case EIActionValueType::Axis3D: return 3;
		case EIActionValueType::Axis2D: return 2;
		default: return 1;
	}
}

} // namespace

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Negate / Scale (per-axis).
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierNegate::modify(EIActionValue v)
{
	v.value *= vec3(x ? -1.0f : 1.0f, y ? -1.0f : 1.0f, z ? -1.0f : 1.0f);
	return v;
}

void EIModifierNegate::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

EIActionValue EIModifierScale::modify(EIActionValue v)
{
	v.value *= vec3(x, y, z);
	return v;
}

void EIModifierScale::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Swizzle / To World Space.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierSwizzleAxis::modify(EIActionValue v)
{
	switch (type)
	{
		case EISwizzleAxis::XZY: v.value = xzy(v.value); break;
		case EISwizzleAxis::YXZ: v.value = yxz(v.value); break;
		case EISwizzleAxis::YZX: v.value = yzx(v.value); break;
		case EISwizzleAxis::ZXY: v.value = zxy(v.value); break;
		case EISwizzleAxis::ZYX: v.value = zyx(v.value); break;
		default: break;
	}
	return v;
}

void EIModifierSwizzleAxis::serialize(EISerializer &s) { s.io("type", type); }

EIActionValue EIModifierToWorldSpace::modify(EIActionValue v)
{
	// Mirrors Unreal: device Z→world X (fwd), device X→world Y (right),
	// device Y→world Z (up) for 3D; for 2D swap X and Y.
	switch (v.valueType)
	{
		case EIActionValueType::Axis3D:
			v.value = vec3(v.value.z, v.value.x, v.value.y);
			break;
		case EIActionValueType::Axis2D:
		{
			float t = v.value.x;
			v.value.x = v.value.y;
			v.value.y = t;
			break;
		}
		default: break;
	}
	return v;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Dead Zone.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierDeadZone::modify(EIActionValue v)
{
	float range = upperThreshold - lowerThreshold;
	if (range <= 0.0f)
		return v;

	auto dz = [&](float a) -> float {
		float r = (abs(a) - lowerThreshold) / range;
		return clamp(r, 0.0f, 1.0f) * sign(a);
	};
	auto unscaledDz = [&](float a) -> float {
		float abs_a = abs(a);
		if (abs_a < lowerThreshold) return 0.0f;
		return min(abs_a, upperThreshold) * sign(a);
	};

	const int n = axisCount(v.valueType);

	switch (type)
	{
		case EIDeadZoneType::Axial:
		{
			if (n >= 1) v.value.x = dz(v.value.x);
			if (n >= 2) v.value.y = dz(v.value.y);
			if (n >= 3) v.value.z = dz(v.value.z);
			break;
		}
		case EIDeadZoneType::Radial:
		case EIDeadZoneType::UnscaledRadial:
		{
			auto remap = [&](float a) {
				return type == EIDeadZoneType::Radial ? dz(a) : unscaledDz(a);
			};
			if (n == 1)
			{
				v.value.x = remap(v.value.x);
			}
			else if (n == 2)
			{
				float len = sqrt(v.value.x * v.value.x + v.value.y * v.value.y);
				if (len > 0.0f)
				{
					float r = remap(len) / len;
					v.value.x *= r;
					v.value.y *= r;
				}
			}
			else // 3D
			{
				float len = v.value.length();
				if (len > 0.0f)
				{
					float r = remap(len) / len;
					v.value *= r;
				}
			}
			break;
		}
	}
	return v;
}

void EIModifierDeadZone::serialize(EISerializer &s)
{
	s.io("lower", lowerThreshold);
	s.io("upper", upperThreshold);
	s.io("type", type);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Response Curve - Exponential.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierResponseCurveExponential::modify(EIActionValue v)
{
	auto curve = [](float a, float e) -> float {
		if (e == 1.0f) return a;
		return sign(a) * pow(abs(a), e);
	};
	// Touching only the active axes prevents spurious values from leaking
	// into components the action's value type doesn't carry — same pattern
	// as DeadZone above.
	const int n = axisCount(v.valueType);
	if (n >= 1) v.value.x = curve(v.value.x, x);
	if (n >= 2) v.value.y = curve(v.value.y, y);
	if (n >= 3) v.value.z = curve(v.value.z, z);
	return v;
}

void EIModifierResponseCurveExponential::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

namespace
{
// Default curve: linear identity across [-1, 1] — two keys at (-1,-1) and
// (1,1). Editor viewport matches this range, so the user can shape the
// negative half independently if they need an asymmetric response.
Curve2dPtr makeLinearCurve()
{
	auto c = Curve2d::create();
	c->addKey(vec2(-1.0f, -1.0f));
	c->addKey(vec2(1.0f, 1.0f));
	return c;
}
}

EIModifierResponseCurveUser::EIModifierResponseCurveUser()
	: x(makeLinearCurve())
	, y(makeLinearCurve())
	, z(makeLinearCurve())
{
}

EIActionValue EIModifierResponseCurveUser::modify(EIActionValue v)
{
	// Direct mapping. Inputs outside [-1, 1] saturate at the curve's
	// endpoints via Curve2d's REPEAT_MODE_CLAMP — chain a Saturate/Clamp
	// upstream if you need finer control over out-of-range inputs.
	auto apply = [](float a, const Curve2dPtr &c) -> float {
		// evaluate() on a zero-key curve is UB. Pass-through is the only
		// meaningful behavior — the editor blocks deleting the last key,
		// so this is a safety net for hand-edited XML / null Ptr.
		if (!c || c->getNumKeys() == 0)
			return a;
		return c->evaluate(a);
	};
	const int n = axisCount(v.valueType);
	if (n >= 1) v.value.x = apply(v.value.x, x);
	if (n >= 2) v.value.y = apply(v.value.y, y);
	if (n >= 3) v.value.z = apply(v.value.z, z);
	return v;
}

void EIModifierResponseCurveUser::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Clamp / Saturate / Absolute Value.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierClamp::modify(EIActionValue v)
{
	v.value = clamp(v.value, vec3(minX, minY, minZ), vec3(maxX, maxY, maxZ));
	return v;
}

void EIModifierClamp::serialize(EISerializer &s)
{
	s.io("minX", minX);
	s.io("minY", minY);
	s.io("minZ", minZ);
	s.io("maxX", maxX);
	s.io("maxY", maxY);
	s.io("maxZ", maxZ);
}

EIActionValue EIModifierSaturate::modify(EIActionValue v)
{
	if (x) v.value.x = clamp(v.value.x, -1.0f, 1.0f);
	if (y) v.value.y = clamp(v.value.y, -1.0f, 1.0f);
	if (z) v.value.z = clamp(v.value.z, -1.0f, 1.0f);
	return v;
}

void EIModifierSaturate::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

EIActionValue EIModifierAbsoluteValue::modify(EIActionValue v)
{
	if (x) v.value.x = abs(v.value.x);
	if (y) v.value.y = abs(v.value.y);
	if (z) v.value.z = abs(v.value.z);
	return v;
}

void EIModifierAbsoluteValue::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dt-dependent: Scale By Delta Time, Smooth.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierScaleByDeltaTime::modify(EIActionValue v)
{
	float dt = EITestHooks::currentDeltaTime();
	if (x) v.value.x *= dt;
	if (y) v.value.y *= dt;
	if (z) v.value.z *= dt;
	return v;
}

void EIModifierScaleByDeltaTime::serialize(EISerializer &s)
{
	s.io("x", x);
	s.io("y", y);
	s.io("z", z);
}

EIActionValue EIModifierSmooth::modify(EIActionValue v)
{
	float dt = EITestHooks::currentDeltaTime();
	if (!_primed || speed <= 0.0f)
	{
		_last = v.value;
		_primed = true;
	}
	else
	{
		float alpha = clamp(1.0f - exp(-speed * dt), 0.0f, 1.0f);
		_last = _last + (v.value - _last) * alpha;
	}
	v.value = _last;
	return v;
}

void EIModifierSmooth::serialize(EISerializer &s) { s.io("speed", speed); }

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// FOV Scaling — tangent ratio relative to an 80° baseline.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
EIActionValue EIModifierFOVScaling::modify(EIActionValue v)
{
	float fov = EITestHooks::currentFov();
	if (fov <= 0.0f)
		return v;

	const float baseFOV = 80.0f;
	const float deg2rad = Consts::PI / 180.0f;
	float baseTan = tan(baseFOV * 0.5f * deg2rad);
	float curTan = tan(fov * 0.5f * deg2rad);
	float k = scale;
	if (baseTan > 0.0f)
		k *= (curTan / baseTan);

	v.value *= k;
	return v;
}

void EIModifierFOVScaling::serialize(EISerializer &s) { s.io("scale", scale); }
