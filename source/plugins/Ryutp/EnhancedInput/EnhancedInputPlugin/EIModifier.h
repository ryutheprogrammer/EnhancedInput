#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Per-axis negate / scale.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifierNegate: public EIModifier
{
public:
	bool x = true;
	bool y = true;
	bool z = true;

	const char *getClassName() const noexcept override { return "Negate"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

class EIModifierScale: public EIModifier
{
public:
	float x = 1.0f;
	float y = 1.0f;
	float z = 1.0f;

	const char *getClassName() const noexcept override { return "Scale"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Axis swizzling.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
ENUM(EISwizzleAxis, XYZ, XZY, YXZ, YZX, ZXY, ZYX);

class EIModifierSwizzleAxis: public EIModifier
{
public:
	EISwizzleAxis type = EISwizzleAxis::XYZ;

	const char *getClassName() const noexcept override { return "Swizzle Axis"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

class EIModifierToWorldSpace: public EIModifier
{
public:
	const char *getClassName() const noexcept override { return "To World Space"; }
	EIActionValue modify(EIActionValue v) override;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Dead zone with three flavors (matches Unreal's Enhanced Input).
//  - Axial: per-axis remap.
//  - Radial: smooth radial remap on the magnitude (2D/3D), per-axis on 1D.
//  - UnscaledRadial: hard cutoff + clamp, no smoothing.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
ENUM(EIDeadZoneType, Axial, Radial, UnscaledRadial);

class EIModifierDeadZone: public EIModifier
{
public:
	float lowerThreshold = 0.2f;
	float upperThreshold = 1.0f;
	EIDeadZoneType type = EIDeadZoneType::Radial;

	const char *getClassName() const noexcept override { return "Dead Zone"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Exponent curve per axis: out = sign(v) * pow(|v|, exp).
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifierResponseCurveExponential: public EIModifier
{
public:
	float x = 1.0f;
	float y = 1.0f;
	float z = 1.0f;

	const char *getClassName() const noexcept override
	{
		return "Response Curve - Exponential";
	}
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Per-axis clamp + saturate(-1,1) + abs() with per-component enable.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifierClamp: public EIModifier
{
public:
	float minX = -1.0f, minY = -1.0f, minZ = -1.0f;
	float maxX = 1.0f, maxY = 1.0f, maxZ = 1.0f;

	const char *getClassName() const noexcept override { return "Clamp"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

class EIModifierSaturate: public EIModifier
{
public:
	bool x = true;
	bool y = true;
	bool z = true;

	const char *getClassName() const noexcept override { return "Saturate"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

class EIModifierAbsoluteValue: public EIModifier
{
public:
	bool x = true;
	bool y = true;
	bool z = true;

	const char *getClassName() const noexcept override { return "Absolute Value"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// dt-dependent (pulls dt from Game::getIFps()).
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifierScaleByDeltaTime: public EIModifier
{
public:
	bool x = true;
	bool y = true;
	bool z = true;

	const char *getClassName() const noexcept override { return "Scale By Delta Time"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};

// Exponential smoothing toward target. alpha = 1 - exp(-speed * dt) → fps-independent.
class EIModifierSmooth: public EIModifier
{
public:
	float speed = 10.0f;

	const char *getClassName() const noexcept override { return "Smooth"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;

private:
	Unigine::Math::vec3 _last{0.0f, 0.0f, 0.0f};
	bool _primed = false;
};

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// FOV scaling — multiplies input by a factor derived from active player's
// camera FOV (tangent ratio relative to an 80° baseline).
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
class EIModifierFOVScaling: public EIModifier
{
public:
	float scale = 1.0f;

	const char *getClassName() const noexcept override { return "FOV Scaling"; }
	EIActionValue modify(EIActionValue v) override;
	void serialize(EISerializer &s) override;
};
