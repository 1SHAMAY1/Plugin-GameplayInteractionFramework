#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "GameplayTagContainer.h"
#include "Engine/EngineTypes.h"
#include "AsyncAction_PerformInteraction.generated.h"

class UInteractionDetectorComponent;
class AActor;
class UObject;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPerformInteractionSimpleOutputPin);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformInteractionProgressOutputPin, float, ProgressRatio);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformInteractionCompletedOutputPin, AActor*, Instigator);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPerformInteractionFailedOutputPin, FGameplayTag, CancelReason);

/**
 * UAsyncAction_PerformInteraction
 *
 * Blueprint Async Action node providing a clean latent execution flow for interactions.
 * Bridges local presentation prediction (0ms OnStarted / OnProgress) with server-authoritative
 * validation and completion (OnCompleted / OnFailed).
 */
UCLASS()
class GAMEPLAYINTERACTIONFRAMEWORK_API UAsyncAction_PerformInteraction : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	/** Fires immediately when interaction begins (useful for local animations, sound, camera focus) */
	UPROPERTY(BlueprintAssignable)
	FOnPerformInteractionSimpleOutputPin OnStarted;

	/** Fires smoothly on each progress update with normalized 0.0 - 1.0 ratio (for HUD progress bars) */
	UPROPERTY(BlueprintAssignable)
	FOnPerformInteractionProgressOutputPin OnProgress;

	/** Fires when authoritative server acknowledges successful interaction completion */
	UPROPERTY(BlueprintAssignable)
	FOnPerformInteractionCompletedOutputPin OnCompleted;

	/** Fires when interaction is rejected by server, canceled, timed out, or interrupted */
	UPROPERTY(BlueprintAssignable)
	FOnPerformInteractionFailedOutputPin OnFailed;

	/**
	 * Latent Blueprint node to initiate and monitor an interaction.
	 *
	 * @param WorldContextObject Context object to access World
	 * @param DetectorComponent The player's detector component
	 * @param TargetObject Optional specific target object (if null, uses detector's focused target)
	 * @param TimeoutSeconds Maximum time before failing with Timeout (default 10s)
	 */
	UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "WorldContextObject", DefaultToSelf = "DetectorComponent"), Category = "Interaction|Async")
	static UAsyncAction_PerformInteraction* PerformInteractionAsync(
		UObject* WorldContextObject,
		UInteractionDetectorComponent* DetectorComponent,
		UObject* TargetObject = nullptr,
		float TimeoutSeconds = 10.0f);

	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;

private:
	UFUNCTION()
	void HandleInteractionStarted(FGameplayTag ModeTag, UObject* Target);

	UFUNCTION()
	void HandleProgressUpdated(float ProgressRatio, FGameplayTag ModeTag, UObject* Target);

	UFUNCTION()
	void HandleInteractionCompleted(UObject* Target);

	UFUNCTION()
	void HandleInteractionCanceled(FGameplayTag ReasonTag, UObject* Target);

	void HandleTimeout();

	void CleanupDelegates();

	UPROPERTY()
	TWeakObjectPtr<UInteractionDetectorComponent> DetectorWeak = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UObject> TargetWeak = nullptr;

	UPROPERTY()
	TWeakObjectPtr<UWorld> WorldWeak = nullptr;

	float TimeoutDuration = 10.0f;
	FTimerHandle TimeoutTimerHandle;
};
