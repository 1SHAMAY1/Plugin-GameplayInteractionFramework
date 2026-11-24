#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "GameplayTagContainer.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractionLocalPlayerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalPlayerFocusedTargetChanged, UObject*, FocusedTarget, const FInteractionPromptData&, PromptData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnLocalPlayerInteractionProgress, float, ProgressRatio, FGameplayTag, ModeTag, UObject*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalPlayerInteractionCanceled, FGameplayTag, ReasonTag, UObject*, Target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalPlayerInteractionCompleted, UObject*, Target);

/**
 * UInteractionLocalPlayerSubsystem
 *
 * Decoupled UI event bus per local player.
 * Allows UI widgets and HUD systems to listen for interaction changes, prompts, and progress
 * without hard-coupling to specific characters, pawns, or detector components.
 */
UCLASS()
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractionLocalPlayerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// --- UI Event Dispatchers ---

	UPROPERTY(BlueprintAssignable, Category = "Interaction|UI")
	FOnLocalPlayerFocusedTargetChanged OnFocusedTargetChanged;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|UI")
	FOnLocalPlayerInteractionProgress OnInteractionProgress;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|UI")
	FOnLocalPlayerInteractionCanceled OnInteractionCanceled;

	UPROPERTY(BlueprintAssignable, Category = "Interaction|UI")
	FOnLocalPlayerInteractionCompleted OnInteractionCompleted;

	// --- Broadcast Handlers (Invoked by Detector Component) ---

	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void NotifyFocusedTargetChanged(UObject* NewTarget, const FInteractionPromptData& PromptData);

	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void NotifyInteractionProgress(float ProgressRatio, FGameplayTag ModeTag, UObject* Target);

	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void NotifyInteractionCanceled(FGameplayTag ReasonTag, UObject* Target);

	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void NotifyInteractionCompleted(UObject* Target);

	// --- State Accessors for UI Initialization ---

	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	UObject* GetCurrentFocusedTarget() const { return CurrentFocusedTarget.Get(); }

	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	FInteractionPromptData GetCurrentPromptData() const { return CurrentPromptData; }

	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	float GetCurrentProgressRatio() const { return CurrentProgressRatio; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> CurrentFocusedTarget = nullptr;

	UPROPERTY(Transient)
	FInteractionPromptData CurrentPromptData;

	UPROPERTY(Transient)
	float CurrentProgressRatio = 0.0f;
};
