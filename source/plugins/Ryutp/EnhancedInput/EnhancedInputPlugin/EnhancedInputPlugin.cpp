#include "EISystem.h"
#include <UniginePlugin.h>

using namespace Unigine;

class EnhancedInputPlugin: public Plugin
{
public:
	const char *get_name() override
	{
		return "RyutpEnhancedInput";
	}

	void *get_data() override
	{
		return static_cast<EISystem *>(EISystemImpl::get());
	}

	int init() override
	{
		Log::message("Init EnhancedInput\n");
		EISystemImpl::get()->refresh();
		return 1;
	}

	int shutdown() override
	{
		Log::message("Shutdown EnhancedInput\n");
		return 1;
	}

	void update() override
	{
		// TODO
	}
};

extern "C" UNIGINE_EXPORT void *CreatePlugin()
{
#ifdef USE_EVALUATION
	if (!Unigine::Engine::isEvaluation())
	{
		return nullptr;
	}
#endif
	return new EnhancedInputPlugin();
}

extern "C" UNIGINE_EXPORT void ReleasePlugin(void *plugin)
{
	delete static_cast<EnhancedInputPlugin *>(plugin);
}
