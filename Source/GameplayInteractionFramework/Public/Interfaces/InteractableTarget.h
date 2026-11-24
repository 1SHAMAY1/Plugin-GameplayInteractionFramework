#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractableTarget.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractableTarget : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface implemented by any Actor or Component that can be interacted with.
 * Allows custom actors to participate in the interaction framework without requiring UInteractableComponent.
 */
class GAMEPLAYINTERACTIONFRAMEWORK_API IInteractableTarget
{
	GENERATED_BODY()

public:
	/** Checks whether the target is currently available and valid for interaction */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	bool CanInteract(const FInteractionFrameworkContext& Context, FGameplayTagContainer& OutFailureTags) const;

	/** Returns UI display information for this target or specific smart point */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	FInteractionPromptData GetPromptData(int32 PointIndex) const;

	/** Returns array of smart interaction sockets/offsets */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	TArray<FSmartInteractionPoint> GetSmartPoints() const;

	/** Called when interaction begins (e.g., button down) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	void OnInteractionStarted(const FInteractionFrameworkContext& Context);

	/** Called on tick updates during continuous interactions (e.g., Hold progress ratio 0.0 - 1.0) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	void OnInteractionProgress(const FInteractionFrameworkContext& Context, float ProgressRatio);

	/** Called when interaction condition is successfully satisfied */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	void OnInteractionCompleted(const FInteractionFrameworkContext& Context);

	/** Called when interaction is canceled, released early, or interrupted */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	void OnInteractionCanceled(const FInteractionFrameworkContext& Context, FGameplayTag ReasonTag);

	/** Called when detector begins focusing this target (for custom outlines, highlights) */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	void OnFocusGained(const FInteractionFrameworkContext& Context);

	/** Called when detector stops focusing this target */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction|Target")
	void OnFocusLost(const FInteractionFrameworkContext& Context);
};
