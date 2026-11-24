#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "InteractionInstigator.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractionInstigator : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface implemented by Actors or Pawns that initiate interactions (Characters, Controllers, AI Agents).
 */
class GAMEPLAYINTERACTIONFRAMEWORK_API IInteractionInstigator
{
	GENERATED_BODY()

public:
	/** Returns the origin and direction for interaction traces (e.g., Camera or Eyes location) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Instigator")
	void GetInteractionViewPoint(FVector& OutLocation, FRotator& OutRotation) const;

	/** Returns current gameplay tags carried by this instigator (for tag requirement queries) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Instigator")
	FGameplayTagContainer GetInstigatorTags() const;

	/** Checks whether this instigator is currently permitted to interact (not dead, stunned, climbing, etc.) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Instigator")
	bool CanInitiateInteractions() const;
};
