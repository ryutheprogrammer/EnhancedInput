#pragma once
#include <plugins/Ryutp/EnhancedInput/EnhancedInput.h>
#include <UnigineHashSet.h>

class EIContextImpl: public EIContext
{
public:
	EIMapping *map(const EIAction *action, EIKey key) override;
	void unmap(const EIAction *action) override;
	void unmap() override;

	Unigine::Vector<EIActionMappings> &getActionMappings() noexcept override
	{
		return _actionMappings;
	}

	const Unigine::Vector<EIActionMappings> &getActionMappings() const noexcept override
	{
		return _actionMappings;
	}

	Unigine::Vector<EIActionValueInstance> evaluate(int gamepadIndex, bool useKeyboardMouse, Unigine::HashSet<int> &consumedKeys);

private:
	Unigine::Vector<EIActionMappings> _actionMappings;
};
