#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <UnigineGame.h>

class EITriggerBase: public EITrigger
{
public:
	EIActionValue lastValue;

	bool isActive(const EIActionValue &v) const { return v.getMagnitude2() >= threshold * threshold; }

	eTriggerState update(EIActionValue v) override;

protected:
	virtual eTriggerState updateImpl(EIActionValue v) = 0;
};

class EITriggerDown: public EITriggerBase
{
public:
	const char *getClassName() const noexcept override { return "Down"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerUp: public EITriggerBase
{
public:
	const char *getClassName() const noexcept override { return "Up"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerPressed: public EITriggerBase
{
public:
	const char *getClassName() const noexcept override { return "Pressed"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerReleased: public EITriggerBase
{
public:
	const char *getClassName() const noexcept override { return "Released"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerTimeBased: public EITriggerBase
{
public:
	float heldDuration = 0.0f;

	float calcHeldDur() const;

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerHold: public EITriggerTimeBased
{
public:
	float holdThreshold = 0.5f;
	// true → fire Triggered once when the threshold is crossed (current default).
	// false → keep firing Triggered every frame while held past threshold,
	// useful for "press to charge" / auto-repeat semantics.
	bool oneShot = true;

	const char *getClassName() const noexcept override { return "Hold"; }

	void serialize(EISerializer &s) override
	{
		EITriggerBase::serialize(s);
		s.io("holdThreshold", holdThreshold);
		s.io("oneShot", oneShot);
	}

protected:
	eTriggerState updateImpl(EIActionValue v) override;

private:
	bool _triggered = false;
};

class EITriggerHoldAndRelease: public EITriggerTimeBased
{
public:
	float holdThreshold = 0.5f;

	const char *getClassName() const noexcept override { return "Hold and Release"; }

	void serialize(EISerializer &s) override
	{
		EITriggerBase::serialize(s);
		s.io("holdThreshold", holdThreshold);
	}

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerTap: public EITriggerTimeBased
{
public:
	float tapReleaseTime = 0.1f;

	const char *getClassName() const noexcept override { return "Tap"; }

	void serialize(EISerializer &s) override
	{
		EITriggerBase::serialize(s);
		s.io("tapReleaseTime", tapReleaseTime);
	}

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

// Fires Triggered every `interval` seconds while held. Optionally caps at
// `triggerLimit` total fires; 0 means no cap. With `triggerOnStart=true` the
// very first frame past actuation also fires (matches Unreal default).
class EITriggerPulse: public EITriggerTimeBased
{
public:
	float interval = 1.0f;
	int triggerLimit = 0;
	bool triggerOnStart = true;

	const char *getClassName() const noexcept override { return "Pulse"; }

	void serialize(EISerializer &s) override
	{
		EITriggerBase::serialize(s);
		s.io("interval", interval);
		s.io("triggerLimit", triggerLimit);
		s.io("triggerOnStart", triggerOnStart);
	}

protected:
	eTriggerState updateImpl(EIActionValue v) override;

private:
	int _triggerCount = 0;
};

// N consecutive short taps within `repeatDelay` of each other trigger the
// action. Default `numberOfTaps=2` gives a classic double-tap. Tapping longer
// than `tapReleaseTime` resets the counter.
class EITriggerRepeatedTap: public EITriggerTimeBased
{
public:
	float tapReleaseTime = 0.2f;
	float repeatDelay = 0.5f;
	int numberOfTaps = 2;

	const char *getClassName() const noexcept override { return "Repeated Tap"; }

	void serialize(EISerializer &s) override
	{
		EITriggerBase::serialize(s);
		s.io("tapReleaseTime", tapReleaseTime);
		s.io("repeatDelay", repeatDelay);
		s.io("numberOfTaps", numberOfTaps);
	}

protected:
	eTriggerState updateImpl(EIActionValue v) override;

private:
	int _tapCount = 0;
	float _timeSinceLastTap = 0.0f;
};
