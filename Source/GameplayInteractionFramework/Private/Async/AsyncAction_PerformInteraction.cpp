#include "Async/AsyncAction_PerformInteraction.h"
#include "Components/InteractionDetectorComponent.h"
#include "Types/InteractionNativeTags.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Actor.h"

UAsyncAction_PerformInteraction* UAsyncAction_PerformInteraction::PerformInteractionAsync(
	UObject* WorldContextObject,
	UInteractionDetectorComponent* DetectorComponent,
	UObject* TargetObject,
	float TimeoutSeconds)
{
	UAsyncAction_PerformInteraction* Action = NewObject<UAsyncAction_PerformInteraction>();
	Action->DetectorWeak = DetectorComponent;
	Action->TargetWeak = TargetObject;
	Action->TimeoutDuration = TimeoutSeconds;

	if (WorldContextObject)
	{
		Action->WorldWeak = WorldContextObject->GetWorld();
	}
	else if (DetectorComponent)
	{
		Action->WorldWeak = DetectorComponent->GetWorld();
	}

	Action->RegisterWithGameInstance(WorldContextObject);
	return Action;
}

void UAsyncAction_PerformInteraction::Activate()
{
	if (!DetectorWeak.IsValid())
	{
		OnFailed.Broadcast(InteractionNativeTags::CancelReason_InstigatorInvalid.GetTag());
		SetReadyToDestroy();
		return;
	}

	UInteractionDetectorComponent* Detector = DetectorWeak.Get();
	Detector->OnInteractionStartedBP.AddDynamic(this, &UAsyncAction_PerformInteraction::HandleInteractionStarted);
	Detector->OnInteractionProgressBP.AddDynamic(this, &UAsyncAction_PerformInteraction::HandleProgressUpdated);
	Detector->OnInteractionCompletedBP.AddDynamic(this, &UAsyncAction_PerformInteraction::HandleInteractionCompleted);
	Detector->OnInteractionCanceledBP.AddDynamic(this, &UAsyncAction_PerformInteraction::HandleInteractionCanceled);

	if (WorldWeak.IsValid() && TimeoutDuration > 0.0f)
	{
		WorldWeak->GetTimerManager().SetTimer(TimeoutTimerHandle, this, &UAsyncAction_PerformInteraction::HandleTimeout, TimeoutDuration, false);
	}

	Detector->StartInteract();
}

void UAsyncAction_PerformInteraction::SetReadyToDestroy()
{
	CleanupDelegates();
	Super::SetReadyToDestroy();
}

void UAsyncAction_PerformInteraction::HandleInteractionStarted(FGameplayTag ModeTag, UObject* Target)
{
	OnStarted.Broadcast();
}

void UAsyncAction_PerformInteraction::HandleProgressUpdated(float ProgressRatio, FGameplayTag ModeTag, UObject* Target)
{
	OnProgress.Broadcast(ProgressRatio);
}

void UAsyncAction_PerformInteraction::HandleInteractionCompleted(UObject* Target)
{
	AActor* Instigator = DetectorWeak.IsValid() ? DetectorWeak->GetOwner() : nullptr;
	OnCompleted.Broadcast(Instigator);
	SetReadyToDestroy();
}

void UAsyncAction_PerformInteraction::HandleInteractionCanceled(FGameplayTag ReasonTag, UObject* Target)
{
	OnFailed.Broadcast(ReasonTag);
	SetReadyToDestroy();
}

void UAsyncAction_PerformInteraction::HandleTimeout()
{
	OnFailed.Broadcast(InteractionNativeTags::CancelReason_TimedOut.GetTag());

	if (DetectorWeak.IsValid())
	{
		DetectorWeak->CancelInteract(InteractionNativeTags::CancelReason_TimedOut.GetTag());
	}

	SetReadyToDestroy();
}

void UAsyncAction_PerformInteraction::CleanupDelegates()
{
	if (WorldWeak.IsValid())
	{
		WorldWeak->GetTimerManager().ClearTimer(TimeoutTimerHandle);
	}

	if (DetectorWeak.IsValid())
	{
		UInteractionDetectorComponent* Detector = DetectorWeak.Get();
		Detector->OnInteractionStartedBP.RemoveDynamic(this, &UAsyncAction_PerformInteraction::HandleInteractionStarted);
		Detector->OnInteractionProgressBP.RemoveDynamic(this, &UAsyncAction_PerformInteraction::HandleProgressUpdated);
		Detector->OnInteractionCompletedBP.RemoveDynamic(this, &UAsyncAction_PerformInteraction::HandleInteractionCompleted);
		Detector->OnInteractionCanceledBP.RemoveDynamic(this, &UAsyncAction_PerformInteraction::HandleInteractionCanceled);
	}
}
