#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_InteractionEvent.generated.h"

/**
 * UAnimNotify_InteractionEvent
 *
 * Minimalist, unified animation notify for the Gameplay Interaction Framework.
 * Can be placed on any AnimSequence or AnimMontage keyframe (e.g., hand contacts lever,
 * lock is picked, foot stomps button).
 *
 * Dispatches the configured GameplayTag event to the owning Actor's
 * UInteractionDetectorComponent or UInteractableComponent.
 */
UCLASS(const, hidecategories = Object, collapsecategories, meta = (DisplayName = "Interaction Event"))
class GAMEPLAYINTERACTIONFRAMEWORK_API UAnimNotify_InteractionEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	UAnimNotify_InteractionEvent();

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** Event tag dispatched when this notify triggers (e.g. Interaction.Event.Commit, Interaction.Event.Cancel, Interaction.Event.Custom.*) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (Categories = "Interaction"))
	FGameplayTag EventTag;
};
