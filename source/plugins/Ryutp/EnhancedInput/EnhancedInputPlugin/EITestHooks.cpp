#include "EITestHooks.h"
#include <UnigineGame.h>
#include <UniginePlayers.h>

namespace EITestHooks
{

float dtOverride = -1.0f;
float fovOverride = -1.0f;

float currentDeltaTime()
{
	return dtOverride >= 0.0f ? dtOverride : Unigine::Game::getIFps();
}

float currentFov()
{
	if (fovOverride >= 0.0f)
		return fovOverride;
	auto p = Unigine::Game::getPlayer();
	return p ? p->getFov() : 0.0f;
}

} // namespace EITestHooks
