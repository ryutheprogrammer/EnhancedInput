#include "EIFileSystemRegistry.h"
#include "EISystem.h"

void autoregister(EIAction *action) {}

void autoregister(EIContext *context)
{
	if (!context)
		return;

	if (context->autoRegistration)
		EISystem::get()->addContext(context);
}
