#include "EISystem.h"
#include "EICreatorRegistry.h"
#include "EIFileSystemRegistry.h"
#include "EIModifier.h"
#include "EITrigger.h"
#include "EIContext.h"

using namespace Unigine;

EISystemImpl::EISystemImpl()
{
	auto actions = EIFileSystemRegistryImpl<EIAction>::get();
	actions->setExtension("input_action");

	auto contexts = EIFileSystemRegistryImpl<EIContext, EIContextImpl>::get();
	contexts->setExtension("input_context");
	contexts->setCacheEnabled(false);

	auto mods = EICreatorRegistryImpl<EIModifier>::get();
	mods->setRegistryName("Modifiers");
	mods->registerCreator("Negate", [] { return new EIModifierNegate; });
	mods->registerCreator("Scale", [] { return new EIModifierScale; });
	mods->registerCreator("Swizzle Axis", [] { return new EIModifierSwizzleAxis; });

	auto trigs = EICreatorRegistryImpl<EITrigger>::get();
	trigs->setRegistryName("Triggers");
	trigs->registerCreator("Down", [] { return new EITriggerDown; });
	trigs->registerCreator("Up", [] { return new EITriggerUp; });
	trigs->registerCreator("Pressed", [] { return new EITriggerPressed; });
	trigs->registerCreator("Released", [] { return new EITriggerReleased; });
	trigs->registerCreator("Hold", [] { return new EITriggerHold; });
	trigs->registerCreator("Hold and Release", [] { return new EITriggerHoldAndRelease; });
	trigs->registerCreator("Tap", [] { return new EITriggerTap; });
}

EISystemImpl::~EISystemImpl()
{
}

EISystemImpl *EISystemImpl::get()
{
	static EISystemImpl instance;
	return &instance;
}

void EISystemImpl::refresh()
{
	EIFileSystemRegistryImpl<EIAction>::get()->refresh();
	EIFileSystemRegistryImpl<EIContext, EIContextImpl>::get()->refresh();
}

EICreatorRegistry<EIModifier> *EISystemImpl::getModifierRegistry()
{
	return EICreatorRegistryImpl<EIModifier>::get();
}

EICreatorRegistry<EITrigger> *EISystemImpl::getTriggerRegistry()
{
	return EICreatorRegistryImpl<EITrigger>::get();
}

EIFileSystemRegistry<EIAction> *EISystemImpl::getActionRegistry()
{
	return EIFileSystemRegistryImpl<EIAction>::get();
}

EIFileSystemRegistry<EIContext> *EISystemImpl::getContextRegistry()
{
	return EIFileSystemRegistryImpl<EIContext, EIContextImpl>::get();
}
