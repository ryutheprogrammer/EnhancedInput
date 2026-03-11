#include "EITrigger.h"

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

float EITriggerTimeBased::calcHeldDur(const EIActionValue &v) const
{
	return heldDuration + Unigine::Game::getIFps();
}

eTriggerState EITriggerTimeBased::updateImpl(EIActionValue v)
{
	if (!isActive(v))
	{
		heldDuration = 0;
		return eTriggerState::None;
	}

	heldDuration = calcHeldDur(v);
	return eTriggerState::Ongoing;
}

eTriggerState EITriggerHold::updateImpl(EIActionValue v)
{
	auto state = EITriggerTimeBased::updateImpl(v);

	bool isFirstTriggered = !_triggered;
	_triggered = heldDuration >= holdTreshold;
	if (_triggered)
	{
		return isFirstTriggered ? eTriggerState::Triggered : eTriggerState::None;
	}

	return state;
}

eTriggerState EITriggerHoldAndRelease::updateImpl(EIActionValue v)
{
	float d = calcHeldDur(v);

	auto state = EITriggerTimeBased::updateImpl(v);

	if (d >= holdTreshold && state == eTriggerState::None)
		state = eTriggerState::Triggered;

	return state;
}

eTriggerState EITriggerTap::updateImpl(EIActionValue v)
{
	float lastHeldDuration = heldDuration;
	auto state = EITriggerTimeBased::updateImpl(v);

	if (isActive(lastValue) && state == eTriggerState::None && lastHeldDuration < tapReleaseTimeTreshold)
		return eTriggerState::Triggered;

	if (heldDuration >= tapReleaseTimeTreshold)
		return eTriggerState::None;

	return state;
}
