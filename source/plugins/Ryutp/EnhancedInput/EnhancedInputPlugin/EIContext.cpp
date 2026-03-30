#include "EIContext.h"

using namespace Unigine;

EIMapping *EIContextImpl::map(const EIAction *action, EIKey key)
{
	if (!action)
		return nullptr;

	// Find or create EIActionMappings for this action
	EIActionMappings *entry = nullptr;
	for (auto &am : _actionMappings)
	{
		if (am.action == action)
		{
			entry = &am;
			break;
		}
	}

	if (!entry)
	{
		_actionMappings.append({action});
		entry = &_actionMappings.last();
	}

	entry->mappings.append(EIMapping{{key}});
	return &entry->mappings.last();
}

void EIContextImpl::unmap(const EIAction *action)
{
	for (auto it = _actionMappings.begin(); it != _actionMappings.end();)
	{
		if (it->action == action)
			it = _actionMappings.erase(it);
		else
			++it;
	}
}

void EIContextImpl::unmap()
{
	_actionMappings.clear();
}

Vector<EIActionValueInstance> EIContextImpl::evaluate(int gamepadIndex, bool useKeyboardMouse, HashSet<int> &consumedKeys)
{
	HashMap<const EIAction *, EIActionValueInstance> triggered;

	for (const auto &actionEntry : _actionMappings)
	{
		auto action = actionEntry.action;
		if (!action)
			continue;

		for (const auto &mapping : actionEntry.mappings)
		{
			const auto &primary = mapping.binding;

			if (primary.key.isKeyboardMouse() && !useKeyboardMouse)
				continue;
			if (primary.key.isGamepad() && gamepadIndex < 0)
				continue;
			if (consumedKeys.contains(primary.key.getPlainValue()))
				continue;

			int gpDevice = gamepadIndex >= 0 ? gamepadIndex : 0;

			float keyValue = primary.key.getValue(gpDevice);
			EIActionValue primaryValue{action->valueType, {keyValue, 0, 0}};

			// Primary binding triggers
			eTriggerState combinedState = eTriggerState::None;
			for (const auto &trigger : primary.triggers)
			{
				if (trigger)
					combinedState |= trigger->update(primaryValue);
			}
			if (combinedState == eTriggerState::None)
				continue;

			// AND keys
			bool allActive = true;
			for (const auto &andKey : mapping.andKeys)
			{
				if (andKey.key.isKeyboardMouse() && !useKeyboardMouse)
				{ allActive = false; break; }
				if (andKey.key.isGamepad() && gamepadIndex < 0)
				{ allActive = false; break; }
				if (consumedKeys.contains(andKey.key.getPlainValue()))
				{ allActive = false; break; }

				float andValue = andKey.key.getValue(gpDevice);
				EIActionValue andBindingValue{action->valueType, {andValue, 0, 0}};

				eTriggerState andState = eTriggerState::None;
				for (const auto &trigger : andKey.triggers)
				{
					if (trigger)
						andState |= trigger->update(andBindingValue);
				}
				if (andState == eTriggerState::None)
				{ allActive = false; break; }

				combinedState = (eTriggerState)((int)combinedState & (int)andState);
			}

			if (!allActive)
				continue;

			// Apply primary binding modifiers
			auto value = EIActionValue{action->valueType, {keyValue, 0, 0}};
			for (const auto &modifier : primary.modifiers)
			{
				if (modifier)
					value = modifier->modify(value);
			}

			// Apply action modifiers
			for (const auto &modifier : action->modifiers)
			{
				if (modifier)
					value = modifier->modify(value);
			}

			// Action-level triggers
			eTriggerState state = combinedState;
			for (const auto &trigger : action->triggers)
			{
				if (trigger)
					state |= trigger->update(value);
			}

			// Consume keys
			if (mapping.consumeInput && (int)(state & eTriggerState::Triggered) != 0)
			{
				consumedKeys.append(primary.key.getPlainValue());
				for (const auto &andKey : mapping.andKeys)
					consumedKeys.append(andKey.key.getPlainValue());
			}

			// Accumulate
			auto current = triggered[action];
			Math::vec3 newVal;
			switch (action->accumulationBehavior)
			{
				case EIActionAccumulationBehavior::Highest:
				{
					auto t = current.getValue().value;
					auto m = value.value;
					newVal.x = abs(t.x) > abs(m.x) ? t.x : m.x;
					newVal.y = abs(t.y) > abs(m.y) ? t.y : m.y;
					newVal.z = abs(t.z) > abs(m.z) ? t.z : m.z;
					break;
				}
				case EIActionAccumulationBehavior::Accumulative:
				{
					newVal = current.getValue().value + value.value;
					break;
				}
			};

			triggered[action] = EIActionValueInstance(
				action,
				{action->valueType, newVal},
				current.getState() | state);
		}
	}

	Vector<EIActionValueInstance> trigList;
	for (const auto &it : triggered)
		trigList.append(it.data);

	return trigList;
}
