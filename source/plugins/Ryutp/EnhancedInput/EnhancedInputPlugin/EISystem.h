#pragma once
#include <EnhancedInput/EnhancedInput.h>
#include <UnigineHashMap.h>
#include <UnigineString.h>

class EIContextImpl;

namespace Unigine
{
template <>
struct Hasher<eTriggerState>
{
	using HashType = Hasher<int>::HashType;
	UNIGINE_INLINE static HashType create(const eTriggerState &state)
	{
		return Hasher<int>::create((int)state);
	}
};

} // namespace Unigine

class EISystemImpl: public EISystem
{
private:
	EISystemImpl();
	~EISystemImpl();

public:
	static EISystemImpl *get();

	void refresh();

	void addContext(EIContext *context) override;
	void removeContext(EIContext *context) override;

	EICreatorRegistry<EIModifier> *getModifierRegistry() override;
	EICreatorRegistry<EITrigger> *getTriggerRegistry() override;

	EIFileSystemRegistry<EIAction> *getActionRegistry() override;
	EIFileSystemRegistry<EIContext> *getContextRegistry() override;

	EIBinding *bind(const EIAction *action, eTriggerState state, std::function<void(EIActionValueInstance)> callback) override;
	void unbind(const EIAction *action, EIBinding *binding) override;

private:
	void onWorldUpdate();

private:
	Unigine::EventConnection _worldUpdateConnection;
	Unigine::Vector<EIContextImpl *> _contexts;
	Unigine::HashMap<const EIAction *, Unigine::Vector<EIBinding *>> _binds;
};
