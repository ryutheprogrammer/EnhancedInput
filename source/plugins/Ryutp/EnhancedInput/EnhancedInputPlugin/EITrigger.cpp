#include "EITrigger.h"
#include "EITestHooks.h"

eTriggerState EITriggerBase::update(EIActionValue v)
{
	auto state = updateImpl(v);
	lastValue = v;
	return state;
}

eTriggerState EITriggerDown::updateImpl(EIActionValue v)
{
	return isActive(v) ? eTriggerState::Triggered : eTriggerState::None;
}

eTriggerState EITriggerUp::updateImpl(EIActionValue v)
{
	return isActive(v) ? eTriggerState::None : eTriggerState::Triggered;
}

eTriggerState EITriggerPressed::updateImpl(EIActionValue v)
{
	return isActive(v) && !isActive(lastValue) ? eTriggerState::Triggered : eTriggerState::None;
}

eTriggerState EITriggerReleased::updateImpl(EIActionValue v)
{
	if (isActive(v))
		return eTriggerState::Ongoing;

	if (isActive(lastValue))
		return eTriggerState::Triggered;

	return eTriggerState::None;
}

float EITriggerTimeBased::calcHeldDur() const
{
	return heldDuration + EITestHooks::currentDeltaTime();
}

eTriggerState EITriggerTimeBased::updateImpl(EIActionValue v)
{
	if (!isActive(v))
	{
		heldDuration = 0;
		return eTriggerState::None;
	}

	heldDuration = calcHeldDur();
	return eTriggerState::Ongoing;
}

eTriggerState EITriggerHold::updateImpl(EIActionValue v)
{
	auto state = EITriggerTimeBased::updateImpl(v);

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

eTriggerState EITriggerHoldAndRelease::updateImpl(EIActionValue v)
{
	float d = calcHeldDur();

	auto state = EITriggerTimeBased::updateImpl(v);

	if (d >= holdThreshold && state == eTriggerState::None)
		state = eTriggerState::Triggered;

	return state;
}

eTriggerState EITriggerTap::updateImpl(EIActionValue v)
{
	float lastHeldDuration = heldDuration;
	auto state = EITriggerTimeBased::updateImpl(v);

	if (isActive(lastValue) && state == eTriggerState::None && lastHeldDuration < tapReleaseTime)
		return eTriggerState::Triggered;

	if (heldDuration >= tapReleaseTime)
		return eTriggerState::None;

	return state;
}

eTriggerState EITriggerPulse::updateImpl(EIActionValue v)
{
	auto state = EITriggerTimeBased::updateImpl(v);

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

eTriggerState EITriggerRepeatedTap::updateImpl(EIActionValue v)
{
	float dt = EITestHooks::currentDeltaTime();
	float lastHeldDuration = heldDuration;
	auto state = EITriggerTimeBased::updateImpl(v);

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

	bool justReleased = isActive(lastValue) && state == eTriggerState::None;
	bool isTap = justReleased && lastHeldDuration < tapReleaseTime;

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
