#include "EILocalPlayer.h"
#include <UnigineConsole.h>
#include <UnigineInput.h>
#include <UnigineHashSet.h>

REGISTER_COMPONENT(EILocalPlayerImpl);

using namespace Unigine;

void EILocalPlayerImpl::init()
{
	_playerIndex = _players.size();
	_players.append(this);

	Input::getEventGamepadConnected().connect(
		_gamepadConnectedConnection, this, &EILocalPlayerImpl::onGamepadConnected);
	Input::getEventGamepadDisconnected().connect(
		_gamepadDisconnectedConnection, this, &EILocalPlayerImpl::onGamepadDisconnected);

	if (useGamepad && _resolvedGamepadIndex < 0)
	{
		int numPads = Input::getNumGamePads();
		for (int i = 0; i < numPads; i++)
		{
			auto pad = Input::getGamePad(i);
			if (pad && pad->isAvailable())
				tryClaimGamepad(i);
			if (_resolvedGamepadIndex >= 0)
				break;
		}
	}
}

void EILocalPlayerImpl::update()
{
	if (Console::isActive())
		return;

	int gpIndex = useGamepad ? _resolvedGamepadIndex : -1;

	HashSet<int> consumedKeys;
	for (auto &entry : _contexts)
	{
		auto triggereds = entry.context->evaluate(gpIndex, useKeyboardMouse, consumedKeys);
		for (auto &triggered : triggereds)
		{
			auto it = _binds.find(triggered.getAction()->guid);
			if (it != _binds.end())
			{
				for (const auto &actionBind : it->data)
				{
					if ((int)(actionBind->state & triggered.getState()) != 0)
					{
						actionBind->callback(triggered);
					}
				}
			}
		}
	}
}

void EILocalPlayerImpl::shutdown()
{
	for (auto &it : _binds)
		for (auto &binding : it.data)
			delete binding;
	_binds.clear();
	_contexts.clear();

	_gamepadConnectedConnection.disconnect();
	_gamepadDisconnectedConnection.disconnect();

	_players.removeOne(this);
	for (int i = 0; i < _players.size(); i++)
		_players[i]->_playerIndex = i;
}

void EILocalPlayerImpl::onGamepadConnected(int index)
{
	if (!useGamepad || _resolvedGamepadIndex >= 0)
		return;

	tryClaimGamepad(index);
}

void EILocalPlayerImpl::onGamepadDisconnected(int index)
{
	if (_resolvedGamepadIndex != index)
		return;

	_resolvedGamepadIndex = -1;

	int numPads = Input::getNumGamePads();
	for (int i = 0; i < numPads; i++)
	{
		if (i == index)
			continue;
		auto pad = Input::getGamePad(i);
		if (pad && pad->isAvailable())
			tryClaimGamepad(i);
		if (_resolvedGamepadIndex >= 0)
			break;
	}
}

void EILocalPlayerImpl::tryClaimGamepad(int index)
{
	for (auto *other : _players)
	{
		if (other != this && other->_resolvedGamepadIndex == index)
			return;
	}

	_resolvedGamepadIndex = index;
}

void EILocalPlayerImpl::addContext(EIContext *context, int priority)
{
	auto p = dynamic_cast<EIContextImpl *>(context);
	if (!p)
		return;

	for (auto &entry : _contexts)
	{
		if (entry.context == p)
			return;
	}

	_contexts.append({p, priority});

	for (int i = 0; i < _contexts.size() - 1; i++)
	{
		for (int j = i + 1; j < _contexts.size(); j++)
		{
			if (_contexts[j].priority > _contexts[i].priority)
			{
				auto tmp = _contexts[i];
				_contexts[i] = _contexts[j];
				_contexts[j] = tmp;
			}
		}
	}
}

void EILocalPlayerImpl::removeContext(EIContext *context)
{
	auto p = dynamic_cast<EIContextImpl *>(context);
	if (!p)
		return;

	for (auto it = _contexts.begin(); it != _contexts.end(); ++it)
	{
		if (it->context == p)
		{
			_contexts.erase(it);
			return;
		}
	}
}

EIBinding *EILocalPlayerImpl::bind(const EIAction *action, eTriggerState state, std::function<void(EIActionValueInstance)> callback)
{
	if (!action)
		return nullptr;

	auto binding = new EIBinding{action, state, callback};
	_binds[action->guid].append(binding);
	return binding;
}

void EILocalPlayerImpl::unbind(EIBinding *binding)
{
	if (!binding || !binding->action)
		return;

	auto bindsIt = _binds.find(binding->action->guid);
	if (bindsIt != _binds.end())
	{
		bindsIt->data.removeOne(binding);
		delete binding;
	}
}
