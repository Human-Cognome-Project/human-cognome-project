
#include "HCPEngineModule.h"

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), HCPEngine::HCPEngineModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_HCPEngine, HCPEngine::HCPEngineModule)
#endif
