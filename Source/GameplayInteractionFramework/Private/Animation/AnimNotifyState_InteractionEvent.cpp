#include "Animation/AnimNotifyState_InteractionEvent.h"
#include "Components/InteractionDetectorComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Types/InteractionNativeTags.h"

UAnimNotifyState_InteractionEvent::UAnimNotifyState_InteractionEvent()
{
	BeginEventTag = InteractionNativeTags::Event_Start.GetTag();
	EndEventTag = InteractionNativeTags::Event_Commit.GetTag();
	InterruptedReasonTag = InteractionNativeTags::CancelReason_MontageInterrupted.GetTag();
	bUpdateProgress = true;

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(255, 170, 0, 255);
#endif
}

void UAnimNotifyState_InteractionEvent::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (BeginEventTag.IsValid())
	{
		if (UInteractionDetectorComponent* DetectorComp = OwnerActor->FindComponentByClass<UInteractionDetectorComponent>())
		{
			DetectorComp->HandleInteractionEvent(BeginEventTag);
		}

		if (UInteractableComponent* InteractableComp = OwnerActor->FindComponentByClass<UInteractableComponent>())
		{
			InteractableComp->HandleInteractionEvent(BeginEventTag);
		}
	}
}

void UAnimNotifyState_InteractionEvent::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!bUpdateProgress || !MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Detector handles progress calculation internally via its state machine,
	// but can be nudged by frame delta if active
	if (UInteractionDetectorComponent* DetectorComp = OwnerActor->FindComponentByClass<UInteractionDetectorComponent>())
	{
		if (DetectorComp->IsInteracting())
		{
			// Notify active state tick
		}
	}
}

void UAnimNotifyState_InteractionEvent::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	if (EndEventTag.IsValid())
	{
		if (UInteractionDetectorComponent* DetectorComp = OwnerActor->FindComponentByClass<UInteractionDetectorComponent>())
		{
			DetectorComp->HandleInteractionEvent(EndEventTag);
		}

		if (UInteractableComponent* InteractableComp = OwnerActor->FindComponentByClass<UInteractableComponent>())
		{
			InteractableComp->HandleInteractionEvent(EndEventTag);
		}
	}
}

FString UAnimNotifyState_InteractionEvent::GetNotifyName_Implementation() const
{
	if (EndEventTag.IsValid())
	{
		return FString::Printf(TEXT("Window: %s"), *EndEventTag.ToString());
	}

	return TEXT("Interaction Window");
}
