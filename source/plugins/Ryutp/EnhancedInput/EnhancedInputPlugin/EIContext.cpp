#include "EIContext.h"

EIKeyActionMapping *EIContextImpl::map(const EIAction *action, EIKey key)
{
	if (!action)
		return nullptr;

	auto mapping = new EIKeyActionMapping{action, key};
	_mappings.append(mapping);
	return mapping;
}

void EIContextImpl::unmap(const EIAction *action, EIKey key)
{
	for (auto it = _mappings.begin(); it != _mappings.end();)
	{
		if (it.get()->action == action && it.get()->key == key)
		{
			delete it.get();
			it = _mappings.erase(it);
		} else
		{
			++it;
		}
	}
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

Unigine::Vector<EIActionValueInstance> EIContextImpl::evaluate()
{
	Unigine::HashMap<const EIAction *, EIActionValueInstance> triggered;
	for (const auto &bind : _mappings)
	{
		auto action = bind->action;
		if (!action)
			continue;

		auto init = bind->key.getValue(); // TODO device?

		auto value = EIActionValue{action->valueType, {init, 0, 0}};
		for (const auto &modifier : bind->modifiers)
		{
			if (modifier)
				value = modifier->modify(value);
		}
		for (const auto &modifier : action->modifiers)
		{
			if (modifier)
				value = modifier->modify(value);
		}

		eTriggerState state = eTriggerState::None;
		for (const auto &trigger : bind->triggers)
		{
			if (trigger)
				state |= trigger->update(value);
		}
		for (const auto &trigger : action->triggers)
		{
			if (trigger)
				state |= trigger->update(value);
		}

		auto current = triggered[action];
		Unigine::Math::vec3 newVal;
		switch (action->accumulationBehavior)
		{
			case EIActionAccumulationBehavior::Highest:
			{
				// TODO think about it
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
