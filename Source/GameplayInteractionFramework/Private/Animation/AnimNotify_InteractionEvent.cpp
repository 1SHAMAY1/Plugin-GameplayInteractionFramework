#include "Animation/AnimNotify_InteractionEvent.h"
#include "Components/InteractionDetectorComponent.h"
#include "Components/InteractableComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Types/InteractionNativeTags.h"

UAnimNotify_InteractionEvent::UAnimNotify_InteractionEvent()
{
	EventTag = InteractionNativeTags::Event_Commit.GetTag();

#if WITH_EDITORONLY_DATA
	NotifyColor = FColor(0, 200, 255, 255);
#endif
}

void UAnimNotify_InteractionEvent::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// 1. Dispatch to InteractionDetectorComponent if present (Player / Instigator)
	if (UInteractionDetectorComponent* DetectorComp = OwnerActor->FindComponentByClass<UInteractionDetectorComponent>())
	{
		DetectorComp->HandleInteractionEvent(EventTag);
	}

	// 2. Dispatch to InteractableComponent if present (Target / Prop)
	if (UInteractableComponent* InteractableComp = OwnerActor->FindComponentByClass<UInteractableComponent>())
	{
		InteractableComp->HandleInteractionEvent(EventTag);
	}
}

FString UAnimNotify_InteractionEvent::GetNotifyName_Implementation() const
{
	if (EventTag.IsValid())
	{
		return FString::Printf(TEXT("Interaction: %s"), *EventTag.ToString());
	}

	return TEXT("Interaction Event");
}
