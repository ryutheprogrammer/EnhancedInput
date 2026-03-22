#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include "EIContext.h"

struct EIContextEntry
{
	EIContextImpl *context = nullptr;
	int priority = 0;
};

class EILocalPlayerImpl: public EILocalPlayer
{
public:
	COMPONENT_DEFINE(EILocalPlayerImpl, EILocalPlayer)
	COMPONENT_INIT(init, INT_MIN)
	COMPONENT_UPDATE(update)
	COMPONENT_SHUTDOWN(shutdown)

	void addContext(EIContext *context, int priority = 0) override;
	void removeContext(EIContext *context) override;

	EIBinding *bind(const EIAction *action, eTriggerState state, std::function<void(EIActionValueInstance)> callback) override;
	void unbind(EIBinding *binding) override;

private:
	void init();
	void update();
	void shutdown();

	void onGamepadConnected(int index);
	void onGamepadDisconnected(int index);
	void tryClaimGamepad(int index);

	int _playerIndex = -1;
	int _resolvedGamepadIndex = -1;
	Unigine::Vector<EIContextEntry> _contexts;
	Unigine::HashMap<Unigine::UGUID, Unigine::Vector<EIBinding *>> _binds;
	Unigine::EventConnection _gamepadConnectedConnection;
	Unigine::EventConnection _gamepadDisconnectedConnection;

	static inline Unigine::Vector<EILocalPlayerImpl *> _players;
};
