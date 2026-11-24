#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractionPromptWidgetBase.generated.h"

class UInteractionLocalPlayerSubsystem;

/**
 * UInteractionPromptWidgetBase
 *
 * Decoupled UMG User Widget base class for interaction prompts.
 * Automatically binds to the local player's UInteractionLocalPlayerSubsystem on construct
 * and forwards events to Blueprint-implementable events.
 */
UCLASS(Abstract, Blueprintable, ClassGroup = (Interaction))
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractionPromptWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** Gets the current focused target object */
	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	UObject* GetFocusedTarget() const { return CachedFocusedTarget.Get(); }

	/** Gets the current interaction prompt data */
	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	FInteractionPromptData GetPromptData() const { return CachedPromptData; }

	/** Gets the current interaction progress ratio (0.0 - 1.0) */
	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	float GetProgressRatio() const { return CachedProgressRatio; }

	/** Whether a target is currently focused */
	UFUNCTION(BlueprintPure, Category = "Interaction|UI")
	bool HasFocusedTarget() const { return CachedFocusedTarget.IsValid(); }

protected:
	// --- Blueprint Implementable Events for UI Customization ---

	/** Called when a new interactable target gains focus */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|UI", meta = (DisplayName = "On Focused Target Changed"))
	void BP_OnFocusedTargetChanged(UObject* NewTarget, const FInteractionPromptData& PromptData);

	/** Called as continuous hold or rapid-tap interaction progresses (0.0 to 1.0) */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|UI", meta = (DisplayName = "On Interaction Progress"))
	void BP_OnInteractionProgress(float ProgressRatio, FGameplayTag ModeTag, UObject* Target);

	/** Called when an interaction is canceled or rejected by the server */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|UI", meta = (DisplayName = "On Interaction Canceled"))
	void BP_OnInteractionCanceled(FGameplayTag ReasonTag, UObject* Target);

	/** Called when an interaction completes successfully */
	UFUNCTION(BlueprintImplementableEvent, Category = "Interaction|UI", meta = (DisplayName = "On Interaction Completed"))
	void BP_OnInteractionCompleted(UObject* Target);

	// --- Internal Callbacks from Subsystem ---

	UFUNCTION()
	virtual void HandleFocusedTargetChanged(UObject* NewTarget, const FInteractionPromptData& PromptData);

	UFUNCTION()
	virtual void HandleInteractionProgress(float ProgressRatio, FGameplayTag ModeTag, UObject* Target);

	UFUNCTION()
	virtual void HandleInteractionCanceled(FGameplayTag ReasonTag, UObject* Target);

	UFUNCTION()
	virtual void HandleInteractionCompleted(UObject* Target);

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> CachedFocusedTarget = nullptr;

	UPROPERTY(Transient)
	FInteractionPromptData CachedPromptData;

	UPROPERTY(Transient)
	float CachedProgressRatio = 0.0f;
};
