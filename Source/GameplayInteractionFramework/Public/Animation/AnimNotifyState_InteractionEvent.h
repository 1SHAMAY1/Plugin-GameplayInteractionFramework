#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "GameplayTagContainer.h"
#include "AnimNotifyState_InteractionEvent.generated.h"

/**
 * UAnimNotifyState_InteractionEvent
 *
 * Defines an interaction engagement window across an animation sequence or montage.
 * Automatically signals start upon entry, commits on complete window exit, handles
 * early montage interruption (e.g. dodging, damage, stunning), and optionally calculates
 * normalized progress ratio (0.0 - 1.0) on tick.
 */
UCLASS(meta = (DisplayName = "Interaction Window"))
class GAMEPLAYINTERACTIONFRAMEWORK_API UAnimNotifyState_InteractionEvent : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	UAnimNotifyState_InteractionEvent();

	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;

	/** Tag dispatched when entering the interaction window (default: Interaction.Event.Start) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (Categories = "Interaction"))
	FGameplayTag BeginEventTag;

	/** Tag dispatched when the window finishes normally (default: Interaction.Event.Commit) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (Categories = "Interaction"))
	FGameplayTag EndEventTag;

	/** Reason tag dispatched if montage is interrupted/cancelled prematurely */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction", meta = (Categories = "Interaction.CancelReason"))
	FGameplayTag InterruptedReasonTag;

	/** If true, continuously calculates normalized progress across window duration and broadcasts progress */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction")
	bool bUpdateProgress = true;
};
