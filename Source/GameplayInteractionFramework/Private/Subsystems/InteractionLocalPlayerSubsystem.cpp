#include "Subsystems/InteractionLocalPlayerSubsystem.h"

void UInteractionLocalPlayerSubsystem::NotifyFocusedTargetChanged(UObject* NewTarget, const FInteractionPromptData& PromptData)
{
	CurrentFocusedTarget = NewTarget;
	CurrentPromptData = PromptData;
	CurrentProgressRatio = 0.0f;

	OnFocusedTargetChanged.Broadcast(NewTarget, PromptData);
}

void UInteractionLocalPlayerSubsystem::NotifyInteractionProgress(float ProgressRatio, FGameplayTag ModeTag, UObject* Target)
{
	CurrentProgressRatio = ProgressRatio;
	OnInteractionProgress.Broadcast(ProgressRatio, ModeTag, Target);
}

void UInteractionLocalPlayerSubsystem::NotifyInteractionCanceled(FGameplayTag ReasonTag, UObject* Target)
{
	CurrentProgressRatio = 0.0f;
	OnInteractionCanceled.Broadcast(ReasonTag, Target);
}

void UInteractionLocalPlayerSubsystem::NotifyInteractionCompleted(UObject* Target)
{
	CurrentProgressRatio = 1.0f;
	OnInteractionCompleted.Broadcast(Target);
}
