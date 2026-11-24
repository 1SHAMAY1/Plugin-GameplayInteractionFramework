#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
class FSlateStyleSet;
#endif

class GAMEPLAYINTERACTIONFRAMEWORK_API FGameplayInteractionFrameworkModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

#if WITH_EDITOR
	/** Returns the Slate Style Set registered for this module */
	static TSharedPtr<FSlateStyleSet> GetStyleSet();

private:
	static TSharedPtr<FSlateStyleSet> StyleSetInstance;
#endif
};
