#pragma once

// Plugin-internal override seam for the engine-state inputs that modifiers
// pull in (dt from Game::getIFps(), camera FOV from Game::getPlayer()).
// When an override is set (>= 0) the corresponding modifier uses it instead
// of querying the live engine. This makes self-tests reproducible across
// frame rates and scene state; default (< 0) is the normal runtime path.
namespace EITestHooks
{

extern float dtOverride;   // seconds per frame; < 0 means use Game::getIFps()
extern float fovOverride;  // degrees; < 0 means use Game::getPlayer()->getFov()

// Convenience accessors — modifiers call these instead of Game directly.
float currentDeltaTime();
float currentFov();

} // namespace EITestHooks
