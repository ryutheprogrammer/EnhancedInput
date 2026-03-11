#pragma once
#include <EnhancedInput/EnhancedInput.h>

class EIContextImpl: public EIContext
{
public:
	EIKeyActionMapping *map(const EIAction *action, EIKey key) override;
	void unmap(const EIAction *action, EIKey key) override;
	void unmap(const EIAction *action) override;
	void unmap() override;

	Unigine::Vector<EIKeyActionMapping *> &getMappings() noexcept override
	{
		return _mappings;
	}

	const Unigine::Vector<EIKeyActionMapping *> &getMappings() const noexcept override
	{
		return _mappings;
	}

	Unigine::Vector<EIActionValueInstance> evaluate();

private:
	Unigine::Vector<EIKeyActionMapping *> _mappings;
};
