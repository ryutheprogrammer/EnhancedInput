#include "EITrigger.h"
#include "EITestHooks.h"

namespace
{
// InputEvent::getTimestamp() returns microseconds. Helper converts a span
// between two timestamps into seconds for trigger duration math.
inline float seconds_between(unsigned long long t_later, unsigned long long t_earlier)
{
	if (t_later <= t_earlier)
		return 0.0f;
	return (t_later - t_earlier) / 1'000'000.0f;
}
}

eTriggerState EITriggerBase::update(EIActionValue v, const EIKeyFrameEvents &events)
{
	auto state = updateImpl(v, events);
	lastValue = v;
	return state;
}

eTriggerState EITriggerDown::updateImpl(EIActionValue v, const EIKeyFrameEvents &)
{
	// Continuous: state of v this frame is all that matters. Sub-frame
	// press+release would still leave isActive=false here, but that's the
	// right semantic for Down — "Pressed" is the trigger to use for taps.
	return isActive(v) ? eTriggerState::Triggered : eTriggerState::None;
}

eTriggerState EITriggerUp::updateImpl(EIActionValue v, const EIKeyFrameEvents &)
{
	return isActive(v) ? eTriggerState::None : eTriggerState::Triggered;
}

eTriggerState EITriggerPressed::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	// Fires on any press event this frame — catches sub-frame taps that
	// would be lost by the old lastValue-vs-v comparison.
	return wasPressed(v, events) ? eTriggerState::Triggered : eTriggerState::None;
}

eTriggerState EITriggerReleased::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	if (wasReleased(v, events))
		return eTriggerState::Triggered;
	if (isActive(v))
		return eTriggerState::Ongoing;
	return eTriggerState::None;
}

float EITriggerTimeBased::calcHeldDur() const
{
	return heldDuration + EITestHooks::currentDeltaTime();
}

eTriggerState EITriggerTimeBased::updateImpl(EIActionValue v, const EIKeyFrameEvents &)
{
	if (!isActive(v))
	{
		heldDuration = 0;
		return eTriggerState::None;
	}

	heldDuration = calcHeldDur();
	return eTriggerState::Ongoing;
}

eTriggerState EITriggerHold::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	auto state = EITriggerTimeBased::updateImpl(v, events);

	bool isFirstTriggered = !_triggered;
	_triggered = heldDuration >= holdThreshold;
	if (_triggered)
	{
		// One-shot: fire once on the crossing frame, silent afterward.
		// Continuous: fire every frame past the threshold.
		return (isFirstTriggered || !oneShot) ? eTriggerState::Triggered : eTriggerState::None;
	}

	return state;
}

eTriggerState EITriggerHoldAndRelease::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	float d = calcHeldDur();

	auto state = EITriggerTimeBased::updateImpl(v, events);

	if (d >= holdThreshold && state == eTriggerState::None)
		state = eTriggerState::Triggered;

	return state;
}

eTriggerState EITriggerTap::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	// Sub-frame tap (press AND release in this frame): use event timestamps
	// for a precise duration — frame-rounded heldDuration would be 0 here
	// since isActive(v) is false. Without this branch a tap entirely inside
	// one frame is lost.
	if (events.pressCount > 0 && events.releaseCount > 0 && !isActive(v))
	{
		float dur = seconds_between(events.lastReleaseTime, events.firstPressTime);
		EITriggerTimeBased::updateImpl(v, events);  // resets heldDuration to 0
		if (dur <= tapReleaseTime)
			return eTriggerState::Triggered;
		return eTriggerState::None;
	}

	float lastHeldDuration = heldDuration;
	auto state = EITriggerTimeBased::updateImpl(v, events);

	// Multi-frame tap: released after some active frames.
	if (isActive(lastValue) && state == eTriggerState::None && lastHeldDuration < tapReleaseTime)
		return eTriggerState::Triggered;

	if (heldDuration >= tapReleaseTime)
		return eTriggerState::None;

	return state;
}

eTriggerState EITriggerPulse::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	auto state = EITriggerTimeBased::updateImpl(v, events);

	if (state == eTriggerState::Ongoing)
	{
		if (triggerLimit == 0 || _triggerCount < triggerLimit)
		{
			// triggerOnStart=true counts the actuation tick as fire #0, so the
			// first fire happens immediately and the next at heldDuration > interval.
			// triggerOnStart=false delays the first fire by one full interval.
			float fireAt = interval * (triggerOnStart ? _triggerCount : _triggerCount + 1);
			if (heldDuration > fireAt)
			{
				++_triggerCount;
				return eTriggerState::Triggered;
			}
		}
		else
		{
			return eTriggerState::None;
		}
	}
	else
	{
		// Released — reset the counter for the next press.
		_triggerCount = 0;
	}

	return state;
}

eTriggerState EITriggerRepeatedTap::updateImpl(EIActionValue v, const EIKeyFrameEvents &events)
{
	float dt = EITestHooks::currentDeltaTime();
	float lastHeldDuration = heldDuration;
	auto state = EITriggerTimeBased::updateImpl(v, events);

	// Accumulate inter-tap time only when we already have a tap pending —
	// otherwise a freshly-loaded trigger would tick this for nothing.
	if (_tapCount > 0)
		_timeSinceLastTap += dt;

	// Holding past the per-tap window invalidates the in-progress sequence.
	if (heldDuration >= tapReleaseTime)
	{
		_tapCount = 0;
		_timeSinceLastTap = 0;
		return eTriggerState::None;
	}

	// Idle too long between taps — drop the sequence.
	if (_tapCount > 0 && _timeSinceLastTap > repeatDelay)
	{
		_tapCount = 0;
		_timeSinceLastTap = 0;
		return eTriggerState::None;
	}

	// Sub-frame tap: precise duration via timestamps.
	bool subFrameTap = events.pressCount > 0 && events.releaseCount > 0
		&& !isActive(v)
		&& seconds_between(events.lastReleaseTime, events.firstPressTime) <= tapReleaseTime;
	// Multi-frame tap: just released after a short hold.
	bool multiFrameTap = isActive(lastValue) && state == eTriggerState::None
		&& lastHeldDuration < tapReleaseTime;
	bool isTap = subFrameTap || multiFrameTap;

	if (isTap)
	{
		++_tapCount;
		_timeSinceLastTap = 0;
		if (_tapCount >= numberOfTaps)
		{
			_tapCount = 0;
			return eTriggerState::Triggered;
		}
		return eTriggerState::Ongoing;
	}

	// Holding or waiting between taps — keep Ongoing so callbacks see progress.
	if (_tapCount > 0 || state == eTriggerState::Ongoing)
		return eTriggerState::Ongoing;

	return state;
}
