#pragma once
#include <EnhancedInput/EnhancedInput.h>
#include <UnigineGame.h>

class EITriggerBase: public EITrigger
{
public:
	EIActionValue lastValue;

	bool isActive(const EIActionValue &v) const { return v.getMagnitude2() >= treshold * treshold; }

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
	PROP_PARAM(Float, heldDuration, 0.0f);

	float calcHeldDur(const EIActionValue &v) const;

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerHold: public EITriggerTimeBased
{
public:
	PROP_PARAM(Float, holdTreshold, 0.5f)

	const char *getClassName() const noexcept override { return "Hold"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;

private:
	bool _triggered = false;
};

class EITriggerHoldAndRelease: public EITriggerTimeBased
{
public:
	PROP_PARAM(Float, holdTreshold, 0.5f)

	const char *getClassName() const noexcept override { return "Hold and Release"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};

class EITriggerTap: public EITriggerTimeBased
{
public:
	PROP_PARAM(Float, tapReleaseTimeTreshold, 0.1f)

	const char *getClassName() const noexcept override { return "Tap"; }

protected:
	eTriggerState updateImpl(EIActionValue v) override;
};
