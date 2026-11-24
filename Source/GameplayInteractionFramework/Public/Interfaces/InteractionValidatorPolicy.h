#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractionValidatorPolicy.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractionValidatorPolicy : public UInterface
{
	GENERATED_BODY()
};

/**
 * Strategy interface for pluggable custom interaction validation policies.
 * Enables projects to inject modular rules (stamina, faction, quest state) without modifying framework code.
 */
class GAMEPLAYINTERACTIONFRAMEWORK_API IInteractionValidatorPolicy
{
	GENERATED_BODY()

public:
	/** Validates whether an interaction is permitted by this policy */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Policy")
	EInteractionValidationResult ValidateInteraction(const FInteractionFrameworkContext& Context, const UObject* Target) const;

	/** Returns the failure reason tag if validation fails */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Policy")
	FGameplayTag GetPolicyFailureTag() const;
};
