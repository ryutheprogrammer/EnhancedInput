#include "EISystem.h"
#include "EICreatorRegistry.h"
#include "EIFileSystemRegistry.h"
#include "EIModifier.h"
#include "EITrigger.h"
#include "EIContext.h"
#include <UnigineXml.h>
#include <UnigineFileSystem.h>
#include <UnigineEditor.h>
#include <UnigineConsole.h>

using namespace Unigine;

EISystemImpl::EISystemImpl()
{
	auto actions = EIFileSystemRegistryImpl<EIAction>::get();
	actions->setExtension("input_action");

	auto contexts = EIFileSystemRegistryImpl<EIContext, EIContextImpl>::get();
	contexts->setExtension("input_context");

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

	if (!Editor::isLoaded())
	{
		auto &ev = Engine::get()->getEventBeginWorldUpdate();
		ev.connect(_worldUpdateConnection, this, &EISystemImpl::onWorldUpdate);
	}
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

void EISystemImpl::addContext(EIContext *context)
{
	auto p = dynamic_cast<EIContextImpl *>(context);
	if (p && !_contexts.contains(p))
		_contexts.append(p);
}

void EISystemImpl::removeContext(EIContext *context)
{
	auto p = dynamic_cast<EIContextImpl *>(context);
	if (p)
		_contexts.removeOne(p);
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

EIBinding *EISystemImpl::bind(const EIAction *action, eTriggerState state, std::function<void(EIActionValueInstance)> callback)
{
	if (!action)
		return nullptr;

	auto binding = new EIBinding{state, callback};
	_binds[action].append(binding);
	return binding;
}

void EISystemImpl::unbind(const EIAction *action, EIBinding *binding)
{
	auto bindsIt = _binds.find(action);
	if (bindsIt != _binds.end())
	{
		bindsIt->data.removeOne(binding);
		delete binding;
	}
}

void EISystemImpl::onWorldUpdate()
{
	if (Console::isActive())
		return;

	for (auto &context : _contexts)
	{
		auto triggereds = context->evaluate();
		for (auto &triggered : triggereds)
		{
			auto it = _binds.find(triggered.getAction());
			if (it != _binds.end())
			{
				auto &actionBinds = it->data;
				for (const auto &actionBind : actionBinds)
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
