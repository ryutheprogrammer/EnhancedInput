#include "EIContext.h"

EIKeyActionMapping *EIContextImpl::map(const EIAction *action, EIKey key)
{
	if (!action)
		return nullptr;

	auto mapping = new EIKeyActionMapping;
	mapping->action = action;
	mapping->bindings.append({key});
	_mappings.append(mapping);
	return mapping;
}

void EIContextImpl::unmap(const EIAction *action)
{
	for (auto it = _mappings.begin(); it != _mappings.end();)
	{
		if (it.get()->action == action)
		{
			delete it.get();
			it = _mappings.erase(it);
		} else
		{
			++it;
		}
	}
}

void EIContextImpl::unmap()
{
	for (auto &x : _mappings)
		delete x;
	_mappings.clear();
}

Unigine::Vector<EIActionValueInstance> EIContextImpl::evaluate(int gamepadIndex, bool useKeyboardMouse, Unigine::HashSet<int> &consumedKeys)
{
	Unigine::HashMap<const EIAction *, EIActionValueInstance> triggered;

	for (const auto &mapping : _mappings)
	{
		auto action = mapping->action;
		if (!action || mapping->bindings.empty())
			continue;

		// Check ALL bindings (AND logic). Combined state = AND of all binding states.
		eTriggerState combinedState = eTriggerState::None;
		bool allActive = true;

		for (int bi = 0; bi < mapping->bindings.size(); ++bi)
		{
			const auto &binding = mapping->bindings[bi];

			if (binding.key.isKeyboardMouse() && !useKeyboardMouse)
			{
				allActive = false;
				break;
			}
			if (binding.key.isGamepad() && gamepadIndex < 0)
			{
				allActive = false;
				break;
			}

			int keyPlain = binding.key.getPlainValue();
			if (consumedKeys.contains(keyPlain))
			{
				allActive = false;
				break;
			}

			float keyValue = binding.key.getValue(gamepadIndex >= 0 ? gamepadIndex : 0);
			EIActionValue bindingValue{action->valueType, {keyValue, 0, 0}};

			eTriggerState bindingState = eTriggerState::None;
			for (const auto &trigger : binding.triggers)
			{
				if (trigger)
					bindingState |= trigger->update(bindingValue);
			}

			if (bindingState == eTriggerState::None)
			{
				allActive = false;
				break;
			}

			if (bi == 0)
				combinedState = bindingState;
			else
				combinedState = (eTriggerState)((int)combinedState & (int)bindingState);
		}

		if (!allActive)
			continue;

		// Primary value from bindings[0]
		float primaryKeyValue = mapping->bindings[0].key.getValue(gamepadIndex >= 0 ? gamepadIndex : 0);
		auto value = EIActionValue{action->valueType, {primaryKeyValue, 0, 0}};

		for (const auto &modifier : mapping->modifiers)
		{
			if (modifier)
				value = modifier->modify(value);
		}
		for (const auto &modifier : action->modifiers)
		{
			if (modifier)
				value = modifier->modify(value);
		}

		// Action-level triggers (OR'd into combined binding state)
		eTriggerState state = combinedState;
		for (const auto &trigger : action->triggers)
		{
			if (trigger)
				state |= trigger->update(value);
		}

		// Consume ALL binding keys on Triggered
		if (mapping->consumeInput && (int)(state & eTriggerState::Triggered) != 0)
		{
			for (const auto &binding : mapping->bindings)
				consumedKeys.append(binding.key.getPlainValue());
		}

		auto current = triggered[action];
		Unigine::Math::vec3 newVal;
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

	Unigine::Vector<EIActionValueInstance> trigList;
	for (const auto &it : triggered)
		trigList.append(it.data);

	return trigList;
}
