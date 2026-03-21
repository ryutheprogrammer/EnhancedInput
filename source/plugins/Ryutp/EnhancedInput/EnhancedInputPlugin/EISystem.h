#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>

class EISystemImpl: public EISystem
{
private:
	EISystemImpl();
	~EISystemImpl();

public:
	static EISystemImpl *get();

	void refresh();

	EICreatorRegistry<EIModifier> *getModifierRegistry() override;
	EICreatorRegistry<EITrigger> *getTriggerRegistry() override;

	EIFileSystemRegistry<EIAction> *getActionRegistry() override;
	EIFileSystemRegistry<EIContext> *getContextRegistry() override;
};
