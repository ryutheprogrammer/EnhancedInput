#include "EISystem.h"
#include "EIModifier.h"
#include "EITrigger.h"
#include "EITestHooks.h"
#include <UnigineGame.h>
#include <UnigineLog.h>
#include <UnigineMathLib.h>

using namespace Unigine;
using namespace Unigine::Math;

namespace
{

int g_passed = 0;
int g_failed = 0;

bool nearEq(float a, float b, float eps = 1e-3f)
{
	return abs(a - b) < eps;
}

bool nearEq(vec3 a, vec3 b, float eps = 1e-3f)
{
	return nearEq(a.x, b.x, eps) && nearEq(a.y, b.y, eps) && nearEq(a.z, b.z, eps);
}

const char *stateName(eTriggerState s)
{
	switch ((int)s)
	{
		case (int)eTriggerState::None:      return "None";
		case (int)eTriggerState::Triggered: return "Triggered";
		case (int)eTriggerState::Started:   return "Started";
		case (int)eTriggerState::Ongoing:   return "Ongoing";
		case (int)eTriggerState::Canceled:  return "Canceled";
		case (int)eTriggerState::Completed: return "Completed";
		default: return "?";
	}
}

void expectVec(const char *label, vec3 actual, vec3 expected, float eps = 1e-3f)
{
	if (nearEq(actual, expected, eps))
	{
		++g_passed;
	}
	else
	{
		++g_failed;
		Log::error("[EISelfTest] FAIL %s: expected (%.4f, %.4f, %.4f), got (%.4f, %.4f, %.4f)\n",
			label, expected.x, expected.y, expected.z, actual.x, actual.y, actual.z);
	}
}

void expectState(const char *label, eTriggerState actual, eTriggerState expected)
{
	if ((int)actual == (int)expected)
	{
		++g_passed;
	}
	else
	{
		++g_failed;
		Log::error("[EISelfTest] FAIL %s: expected %s, got %s\n",
			label, stateName(expected), stateName(actual));
	}
}

void expectTrue(const char *label, bool cond)
{
	if (cond)
	{
		++g_passed;
	}
	else
	{
		++g_failed;
		Log::error("[EISelfTest] FAIL %s\n", label);
	}
}

EIActionValue val(float x, float y = 0, float z = 0,
	EIActionValueType t = EIActionValueType::Axis3D)
{
	return EIActionValue{t, vec3(x, y, z)};
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Modifiers
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void testNegate()
{
	EIModifierNegate m;
	m.x = true;  m.y = false; m.z = true;
	expectVec("Negate(x,!y,z)", m.modify(val(1, 1, 1)).value, vec3(-1, 1, -1));

	m.x = false; m.y = false; m.z = false;
	expectVec("Negate(noop)", m.modify(val(2, 3, 4)).value, vec3(2, 3, 4));
}

void testScale()
{
	EIModifierScale m;
	m.x = 2.0f; m.y = 3.0f; m.z = -4.0f;
	expectVec("Scale", m.modify(val(1, 1, 1)).value, vec3(2, 3, -4));

	m.x = 1; m.y = 1; m.z = 1;
	expectVec("Scale(identity)", m.modify(val(5, 6, 7)).value, vec3(5, 6, 7));
}

void testSwizzleAxis()
{
	EIModifierSwizzleAxis m;
	auto in = val(1, 2, 3);

	m.type = EISwizzleAxis::XYZ;
	expectVec("Swizzle XYZ", m.modify(in).value, vec3(1, 2, 3));

	m.type = EISwizzleAxis::XZY;
	expectVec("Swizzle XZY", m.modify(in).value, vec3(1, 3, 2));

	m.type = EISwizzleAxis::YXZ;
	expectVec("Swizzle YXZ", m.modify(in).value, vec3(2, 1, 3));

	m.type = EISwizzleAxis::YZX;
	expectVec("Swizzle YZX", m.modify(in).value, vec3(2, 3, 1));

	m.type = EISwizzleAxis::ZXY;
	expectVec("Swizzle ZXY", m.modify(in).value, vec3(3, 1, 2));

	m.type = EISwizzleAxis::ZYX;
	expectVec("Swizzle ZYX", m.modify(in).value, vec3(3, 2, 1));
}

void testToWorldSpace()
{
	EIModifierToWorldSpace m;
	// 3D: device Z→world X, device X→world Y, device Y→world Z.
	auto in3 = val(1, 2, 3, EIActionValueType::Axis3D);
	expectVec("ToWorldSpace Axis3D", m.modify(in3).value, vec3(3, 1, 2));
	// 2D: swap X and Y.
	auto in2 = val(1, 2, 0, EIActionValueType::Axis2D);
	expectVec("ToWorldSpace Axis2D", m.modify(in2).value, vec3(2, 1, 0));
}

void testDeadZone()
{
	{
		EIModifierDeadZone m;
		m.lowerThreshold = 0.2f;
		m.upperThreshold = 1.0f;
		m.type = EIDeadZoneType::Axial;
		auto in = val(0.1f, 0.6f, 0, EIActionValueType::Axis2D);
		// x: 0.1 < 0.2 → 0. y: (0.6-0.2)/0.8 = 0.5.
		expectVec("DeadZone Axial", m.modify(in).value, vec3(0, 0.5f, 0));
	}
	{
		EIModifierDeadZone m;
		m.lowerThreshold = 0.2f;
		m.upperThreshold = 1.0f;
		m.type = EIDeadZoneType::Radial;
		// len = 1.0 → r = (1-0.2)/0.8 = 1.0 → output = input * 1.
		expectVec("DeadZone Radial(at upper)",
			m.modify(val(0.6f, 0.8f, 0, EIActionValueType::Axis2D)).value,
			vec3(0.6f, 0.8f, 0));
		// len = 0.5 → r = (0.5-0.2)/0.8 = 0.375 → output dir * 0.375.
		// dir = (0.3,0.4)/0.5 = (0.6,0.8). output = (0.6*0.375, 0.8*0.375).
		expectVec("DeadZone Radial(mid)",
			m.modify(val(0.3f, 0.4f, 0, EIActionValueType::Axis2D)).value,
			vec3(0.225f, 0.3f, 0));
	}
	{
		EIModifierDeadZone m;
		m.lowerThreshold = 0.2f;
		m.upperThreshold = 1.0f;
		m.type = EIDeadZoneType::UnscaledRadial;
		// len = 0.5 > lower → output direction * clamp(0.5, upper)=0.5.
		expectVec("DeadZone UnscaledRadial(mid)",
			m.modify(val(0.3f, 0.4f, 0, EIActionValueType::Axis2D)).value,
			vec3(0.3f, 0.4f, 0));
	}
}

void testResponseCurveExponential()
{
	EIModifierResponseCurveExponential m;
	m.x = 2.0f; m.y = 1.0f; m.z = 0.5f;
	// 0.5^2 = 0.25; identity; 0.5^0.5 = sqrt(0.5) ≈ 0.7071.
	expectVec("ResponseCurveExp(2,1,0.5)",
		m.modify(val(0.5f, 0.5f, 0.5f)).value, vec3(0.25f, 0.5f, 0.7071f));

	// Negative input keeps sign.
	expectVec("ResponseCurveExp(negative)",
		m.modify(val(-0.5f, -0.5f, -0.5f)).value, vec3(-0.25f, -0.5f, -0.7071f));
}

void testClamp()
{
	EIModifierClamp m;
	m.minX = -1; m.minY = 0; m.minZ = -2;
	m.maxX = 1; m.maxY = 1; m.maxZ = 2;
	expectVec("Clamp", m.modify(val(2, -1, 0.5f)).value, vec3(1, 0, 0.5f));
}

void testSaturate()
{
	EIModifierSaturate m;
	m.x = true; m.y = true; m.z = false;
	// x: 5 → 1. y: -3 → -1. z: 2 stays (disabled).
	expectVec("Saturate(x,y,!z)", m.modify(val(5, -3, 2)).value, vec3(1, -1, 2));
}

void testAbsoluteValue()
{
	EIModifierAbsoluteValue m;
	m.x = true; m.y = false; m.z = true;
	expectVec("AbsoluteValue(x,!y,z)", m.modify(val(-2, -3, -4)).value, vec3(2, -3, 4));
}

void testScaleByDeltaTime()
{
	// dt is injected via EITestHooks::dtOverride at the top of runSelfTest,
	// so the modifier sees exactly 1/60 — no Game::getIFps() jitter.
	const float dt = 1.0f / 60.0f;
	EIModifierScaleByDeltaTime m;
	m.x = true; m.y = false; m.z = true;
	expectVec("ScaleByDeltaTime(x,!y,z)",
		m.modify(val(1, 1, 1)).value, vec3(dt, 1, dt));
}

void testSmooth()
{
	const float dt = 1.0f / 60.0f;
	const float speed = 10.0f;
	const float alpha = 1.0f - exp(-speed * dt);  // ≈ 0.15352

	EIModifierSmooth m;
	m.speed = speed;
	auto one = val(1, 0, 0, EIActionValueType::Axis1D);
	auto zero = val(0, 0, 0, EIActionValueType::Axis1D);

	// Frame 1: _primed=false → _last := input → output is the raw target.
	expectVec("Smooth(first frame primes)",
		m.modify(one).value, vec3(1, 0, 0));
	// Subsequent frames at the same target: _last + (1 - _last) * alpha = 1
	// always, so the output stays pinned regardless of alpha.
	for (int i = 0; i < 10; ++i)
		m.modify(one);
	expectVec("Smooth(held at target)",
		m.modify(one).value, vec3(1, 0, 0));

	// Switch target to 0: _last_new = 1 + (0 - 1) * alpha = 1 - alpha.
	float expectedAfter1 = 1.0f - alpha;
	expectVec("Smooth(1 frame toward 0)",
		m.modify(zero).value, vec3(expectedAfter1, 0, 0));
	// Frame 2: _last_new = expectedAfter1 + (0 - expectedAfter1) * alpha
	//        = expectedAfter1 * (1 - alpha) = (1 - alpha)^2
	float expectedAfter2 = expectedAfter1 * (1.0f - alpha);
	expectVec("Smooth(2 frames toward 0)",
		m.modify(zero).value, vec3(expectedAfter2, 0, 0));
	// After N more frames the value is (1-alpha)^(2+N).
	const int N = 100;
	float expectedAfterN = expectedAfter2;
	for (int i = 0; i < N; ++i)
		expectedAfterN *= (1.0f - alpha);
	for (int i = 0; i < N; ++i)
		m.modify(zero);
	auto settled = m.modify(zero);
	// Each step is exact float math, but small drift across N=100 multiplies
	// → loosen eps to 1e-5.
	expectVec("Smooth(N frames toward 0)",
		settled.value, vec3(expectedAfterN, 0, 0), 1e-5f);
}

void testFOVScaling()
{
	// fov is injected → no dependency on a scene player. Set FOV exactly to
	// the modifier's baseline (80°) so tan ratio = 1 and the output passes
	// through unchanged when scale=1.
	float saved = EITestHooks::fovOverride;
	EITestHooks::fovOverride = 80.0f;
	{
		EIModifierFOVScaling m;
		m.scale = 1.0f;
		expectVec("FOVScaling(fov=baseline)",
			m.modify(val(1, 0, 0)).value, vec3(1, 0, 0));
	}
	// Drop FOV to 60° → tan(30°)/tan(40°). Reproducible.
	EITestHooks::fovOverride = 60.0f;
	{
		EIModifierFOVScaling m;
		m.scale = 1.0f;
		float deg2rad = Consts::PI / 180.0f;
		float expected = tan(30.0f * deg2rad) / tan(40.0f * deg2rad);
		expectVec("FOVScaling(fov=60)",
			m.modify(val(1, 0, 0)).value, vec3(expected, 0, 0));
	}
	EITestHooks::fovOverride = saved;
}

// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%
// Triggers — drive update() through state transitions.
// %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

void testTriggerDown()
{
	EITriggerDown t;
	t.threshold = 0.5f;
	expectState("Down: inactive", t.update(val(0.1f)), eTriggerState::None);
	expectState("Down: active",   t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Down: held",     t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Down: released", t.update(val(0.1f)), eTriggerState::None);
}

void testTriggerUp()
{
	EITriggerUp t;
	t.threshold = 0.5f;
	expectState("Up: inactive",   t.update(val(0.1f)), eTriggerState::Triggered);
	expectState("Up: active",     t.update(val(0.8f)), eTriggerState::None);
	expectState("Up: released",   t.update(val(0.1f)), eTriggerState::Triggered);
}

void testTriggerPressed()
{
	EITriggerPressed t;
	t.threshold = 0.5f;
	expectState("Pressed: initial",       t.update(val(0.1f)), eTriggerState::None);
	expectState("Pressed: rising edge",   t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Pressed: held (no re-fire)", t.update(val(0.8f)), eTriggerState::None);
	expectState("Pressed: released",      t.update(val(0.1f)), eTriggerState::None);
	expectState("Pressed: re-press",      t.update(val(0.8f)), eTriggerState::Triggered);
}

void testTriggerReleased()
{
	EITriggerReleased t;
	t.threshold = 0.5f;
	expectState("Released: initial",      t.update(val(0.1f)), eTriggerState::None);
	expectState("Released: holding 1",    t.update(val(0.8f)), eTriggerState::Ongoing);
	expectState("Released: holding 2",    t.update(val(0.8f)), eTriggerState::Ongoing);
	expectState("Released: falling edge", t.update(val(0.1f)), eTriggerState::Triggered);
	expectState("Released: settled",      t.update(val(0.1f)), eTriggerState::None);
}

void testTriggerHold()
{
	EITriggerHold t;
	t.threshold = 0.5f;
	// Tiny threshold so a single Game::getIFps() exceeds it on the first frame.
	t.holdThreshold = 1e-6f;
	expectState("Hold: 1st active (hits threshold)",
		t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Hold: 2nd active (no re-fire)",
		t.update(val(0.8f)), eTriggerState::None);
	expectState("Hold: release",  t.update(val(0.1f)), eTriggerState::None);
}

void testTriggerHoldAndRelease()
{
	EITriggerHoldAndRelease t;
	t.threshold = 0.5f;
	t.holdThreshold = 1e-6f;
	expectState("HoldAndRelease: holding 1",
		t.update(val(0.8f)), eTriggerState::Ongoing);
	expectState("HoldAndRelease: holding 2",
		t.update(val(0.8f)), eTriggerState::Ongoing);
	expectState("HoldAndRelease: release past threshold",
		t.update(val(0.1f)), eTriggerState::Triggered);
}

void testTriggerTap()
{
	EITriggerTap t;
	t.threshold = 0.5f;
	// Large window — any release counts as a tap.
	t.tapReleaseTime = 1000.0f;
	expectState("Tap: holding", t.update(val(0.8f)), eTriggerState::Ongoing);
	expectState("Tap: quick release", t.update(val(0.1f)), eTriggerState::Triggered);
}

void testTriggerHoldOneShot()
{
	// oneShot=false — must keep firing Triggered every frame past threshold.
	EITriggerHold t;
	t.threshold = 0.5f;
	t.holdThreshold = 1e-6f;
	t.oneShot = false;
	expectState("Hold(!oneShot): 1st past threshold",
		t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Hold(!oneShot): 2nd past threshold (still firing)",
		t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Hold(!oneShot): 3rd past threshold (still firing)",
		t.update(val(0.8f)), eTriggerState::Triggered);
	expectState("Hold(!oneShot): release", t.update(val(0.1f)), eTriggerState::None);
}

void testTriggerPulse()
{
	// Interval tiny so a single Game::getIFps() exceeds it.
	{
		EITriggerPulse t;
		t.threshold = 0.5f;
		t.interval = 1e-6f;
		t.triggerLimit = 0;
		t.triggerOnStart = true;
		// triggerOnStart=true: first frame past actuation fires.
		expectState("Pulse(start): 1st fire",
			t.update(val(0.8f)), eTriggerState::Triggered);
		expectState("Pulse(start): 2nd fire",
			t.update(val(0.8f)), eTriggerState::Triggered);
		expectState("Pulse(start): released",
			t.update(val(0.1f)), eTriggerState::None);
	}
	// Trigger limit caps the number of fires.
	{
		EITriggerPulse t;
		t.threshold = 0.5f;
		t.interval = 1e-6f;
		t.triggerLimit = 2;
		t.triggerOnStart = true;
		expectState("Pulse(limit=2): #1", t.update(val(0.8f)), eTriggerState::Triggered);
		expectState("Pulse(limit=2): #2", t.update(val(0.8f)), eTriggerState::Triggered);
		expectState("Pulse(limit=2): #3 capped",
			t.update(val(0.8f)), eTriggerState::None);
	}
	// triggerOnStart=false delays the first fire by an interval — a huge
	// interval ensures the first frame still fires nothing.
	{
		EITriggerPulse t;
		t.threshold = 0.5f;
		t.interval = 1000.0f;
		t.triggerLimit = 0;
		t.triggerOnStart = false;
		expectState("Pulse(!start): first frame no fire",
			t.update(val(0.8f)), eTriggerState::Ongoing);
	}
}

void testTriggerRepeatedTap()
{
	// Double-tap: numberOfTaps=2, generous windows.
	{
		EITriggerRepeatedTap t;
		t.threshold = 0.5f;
		t.tapReleaseTime = 1000.0f;
		t.repeatDelay = 1000.0f;
		t.numberOfTaps = 2;
		expectState("RepeatedTap(2): press 1", t.update(val(0.8f)), eTriggerState::Ongoing);
		expectState("RepeatedTap(2): release 1 (Ongoing — 1 tap counted)",
			t.update(val(0.1f)), eTriggerState::Ongoing);
		expectState("RepeatedTap(2): press 2",
			t.update(val(0.8f)), eTriggerState::Ongoing);
		expectState("RepeatedTap(2): release 2 (triggers)",
			t.update(val(0.1f)), eTriggerState::Triggered);
	}
	// Holding past tapReleaseTime should invalidate the sequence.
	{
		EITriggerRepeatedTap t;
		t.threshold = 0.5f;
		t.tapReleaseTime = 1e-6f;  // any hold is "too long"
		t.repeatDelay = 1000.0f;
		t.numberOfTaps = 2;
		// First frame already past tapReleaseTime → sequence resets, None.
		expectState("RepeatedTap: hold too long resets",
			t.update(val(0.8f)), eTriggerState::None);
	}
}

void testTriggerValueFlow()
{
	// lastValue must mirror the full vector passed in (not just .x).
	// Pressed/Released rely on this for edge detection; if the base ever
	// trims components, their fire moments will desync from the input.
	{
		EITriggerDown t;
		t.threshold = 0.5f;
		t.update(val(0.7f, -0.3f, 0.8f));
		expectVec("lastValue stores all axes",
			t.lastValue.value, vec3(0.7f, -0.3f, 0.8f));
	}
	// On a state transition Pressed must compare the new value against the
	// PREVIOUS lastValue, so it fires only on the rising edge — verify by
	// alternating a 3D vector that flips magnitude across threshold.
	{
		EITriggerPressed t;
		t.threshold = 0.5f;
		// magnitude = sqrt(0.04 + 0.04 + 0.04) ≈ 0.346 < 0.5 → inactive
		expectState("Pressed(3D): inactive",
			t.update(val(0.2f, 0.2f, 0.2f)), eTriggerState::None);
		// magnitude = sqrt(0.36 + 0.36 + 0.36) ≈ 1.04 > 0.5 → active → edge
		expectState("Pressed(3D): rising edge",
			t.update(val(0.6f, 0.6f, 0.6f)), eTriggerState::Triggered);
		// Re-feed same magnitude — should NOT re-fire because lastValue was
		// already active. This proves lastValue is honored, not just magnitude.
		expectState("Pressed(3D): held (no re-fire)",
			t.update(val(0.6f, 0.6f, 0.6f)), eTriggerState::None);
	}
	// TimeBased heldDuration must reset to 0 on inactive frame, not drift.
	{
		EITriggerHold t;
		t.threshold = 0.5f;
		t.holdThreshold = 100.0f;  // never trigger in this test
		t.update(val(0.8f));
		float afterActive = t.heldDuration;
		expectTrue("TimeBased: heldDuration grew on active", afterActive > 0.0f);
		t.update(val(0.1f));
		expectTrue("TimeBased: heldDuration reset on inactive",
			nearEq(t.heldDuration, 0.0f));
		// Re-actuate — counter must start fresh, not continue from before.
		t.update(val(0.8f));
		expectTrue("TimeBased: re-actuation restarts counter, doesn't accumulate",
			t.heldDuration <= afterActive + 1e-3f);
	}
}

void testTriggerThreshold()
{
	// Boundary: isActive uses |v|² >= threshold², so input == threshold is active.
	{
		EITriggerDown t;
		t.threshold = 0.5f;
		expectState("Threshold: at boundary",
			t.update(val(0.5f)), eTriggerState::Triggered);
	}
	// Just below threshold — must stay inactive.
	{
		EITriggerDown t;
		t.threshold = 0.5f;
		expectState("Threshold: just below",
			t.update(val(0.499f)), eTriggerState::None);
	}
	// Different threshold must shift the boundary (not a hard-coded constant).
	{
		EITriggerDown t;
		t.threshold = 0.9f;
		expectState("Threshold=0.9: 0.5 inactive",
			t.update(val(0.5f)), eTriggerState::None);
		expectState("Threshold=0.9: 0.95 active",
			t.update(val(0.95f)), eTriggerState::Triggered);
	}
	// Threshold = 0 → any non-zero magnitude triggers.
	{
		EITriggerDown t;
		t.threshold = 0.0f;
		expectState("Threshold=0: 0.0001 active",
			t.update(val(0.0001f)), eTriggerState::Triggered);
	}
	// Magnitude is the vector length, not the x component. A 2D vector (0.3, 0.4)
	// has |v|=0.5 and must trigger at threshold=0.5 even though x and y alone are below.
	{
		EITriggerDown t;
		t.threshold = 0.5f;
		auto v2d = EIActionValue{EIActionValueType::Axis2D, vec3(0.3f, 0.4f, 0)};
		expectState("Threshold: 2D magnitude (0.3, 0.4)",
			t.update(v2d), eTriggerState::Triggered);
	}
	// Same vector with threshold=0.6 — magnitude 0.5 < 0.6 → inactive.
	{
		EITriggerDown t;
		t.threshold = 0.6f;
		auto v2d = EIActionValue{EIActionValueType::Axis2D, vec3(0.3f, 0.4f, 0)};
		expectState("Threshold=0.6: 2D (0.3, 0.4) inactive",
			t.update(v2d), eTriggerState::None);
	}
	// 3D magnitude.
	{
		EITriggerDown t;
		t.threshold = 1.0f;
		// |(0.5, 0.5, 0.5)| = sqrt(0.75) ≈ 0.866 < 1.0
		auto v3d = EIActionValue{EIActionValueType::Axis3D, vec3(0.5f, 0.5f, 0.5f)};
		expectState("Threshold=1: 3D (0.5)³ inactive",
			t.update(v3d), eTriggerState::None);
		// |(1, 1, 1)| = sqrt(3) ≈ 1.732 > 1.0
		auto v3d2 = EIActionValue{EIActionValueType::Axis3D, vec3(1, 1, 1)};
		expectState("Threshold=1: 3D (1)³ active",
			t.update(v3d2), eTriggerState::Triggered);
	}
	// Threshold applies to TimeBased triggers too — Hold ignores activity below threshold.
	{
		EITriggerHold t;
		t.threshold = 0.5f;
		t.holdThreshold = 1e-6f;
		// Below threshold: should never count as holding.
		expectState("Hold(thr=0.5): 0.3 not active",
			t.update(val(0.3f)), eTriggerState::None);
		expectState("Hold(thr=0.5): 0.3 still not active",
			t.update(val(0.3f)), eTriggerState::None);
	}
}

} // namespace

int EISystemImpl::runSelfTest()
{
	g_passed = 0;
	g_failed = 0;

	Log::message("[EISelfTest] Running tests...\n");

	// Pin dt to 1/60s for the whole run — makes Smooth / ScaleByDeltaTime and
	// every TimeBased trigger (Hold / HoldAndRelease / Tap / Pulse / Repeated
	// Tap) produce identical results regardless of the host frame rate.
	float savedDt = EITestHooks::dtOverride;
	EITestHooks::dtOverride = 1.0f / 60.0f;

	testNegate();
	testScale();
	testSwizzleAxis();
	testToWorldSpace();
	testDeadZone();
	testResponseCurveExponential();
	testClamp();
	testSaturate();
	testAbsoluteValue();
	testScaleByDeltaTime();
	testSmooth();
	testFOVScaling();

	testTriggerDown();
	testTriggerUp();
	testTriggerPressed();
	testTriggerReleased();
	testTriggerHold();
	testTriggerHoldOneShot();
	testTriggerHoldAndRelease();
	testTriggerTap();
	testTriggerPulse();
	testTriggerRepeatedTap();
	testTriggerThreshold();
	testTriggerValueFlow();

	EITestHooks::dtOverride = savedDt;

	if (g_failed == 0)
		Log::message("[EISelfTest] %d passed, 0 failed\n", g_passed);
	else
		Log::error("[EISelfTest] %d passed, %d FAILED\n", g_passed, g_failed);
	return g_failed;
}
