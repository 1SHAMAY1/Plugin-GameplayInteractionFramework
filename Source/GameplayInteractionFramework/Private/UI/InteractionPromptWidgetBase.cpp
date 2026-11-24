#include "UI/InteractionPromptWidgetBase.h"
#include "Subsystems/InteractionLocalPlayerSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"

void UInteractionPromptWidgetBase::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UInteractionLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UInteractionLocalPlayerSubsystem>())
			{
				Subsystem->OnFocusedTargetChanged.AddDynamic(this, &UInteractionPromptWidgetBase::HandleFocusedTargetChanged);
				Subsystem->OnInteractionProgress.AddDynamic(this, &UInteractionPromptWidgetBase::HandleInteractionProgress);
				Subsystem->OnInteractionCanceled.AddDynamic(this, &UInteractionPromptWidgetBase::HandleInteractionCanceled);
				Subsystem->OnInteractionCompleted.AddDynamic(this, &UInteractionPromptWidgetBase::HandleInteractionCompleted);

				if (Subsystem->GetCurrentFocusedTarget())
				{
					HandleFocusedTargetChanged(Subsystem->GetCurrentFocusedTarget(), Subsystem->GetCurrentPromptData());
				}
			}
		}
	}
}

void UInteractionPromptWidgetBase::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UInteractionLocalPlayerSubsystem* Subsystem = LP->GetSubsystem<UInteractionLocalPlayerSubsystem>())
			{
				Subsystem->OnFocusedTargetChanged.RemoveDynamic(this, &UInteractionPromptWidgetBase::HandleFocusedTargetChanged);
				Subsystem->OnInteractionProgress.RemoveDynamic(this, &UInteractionPromptWidgetBase::HandleInteractionProgress);
				Subsystem->OnInteractionCanceled.RemoveDynamic(this, &UInteractionPromptWidgetBase::HandleInteractionCanceled);
				Subsystem->OnInteractionCompleted.RemoveDynamic(this, &UInteractionPromptWidgetBase::HandleInteractionCompleted);
			}
		}
	}

	Super::NativeDestruct();
}

void UInteractionPromptWidgetBase::HandleFocusedTargetChanged(UObject* NewTarget, const FInteractionPromptData& PromptData)
{
	CachedFocusedTarget = NewTarget;
	CachedPromptData = PromptData;
	CachedProgressRatio = 0.0f;

	BP_OnFocusedTargetChanged(NewTarget, PromptData);
}

void UInteractionPromptWidgetBase::HandleInteractionProgress(float ProgressRatio, FGameplayTag ModeTag, UObject* Target)
{
	CachedProgressRatio = ProgressRatio;
	BP_OnInteractionProgress(ProgressRatio, ModeTag, Target);
}

void UInteractionPromptWidgetBase::HandleInteractionCanceled(FGameplayTag ReasonTag, UObject* Target)
{
	CachedProgressRatio = 0.0f;
	BP_OnInteractionCanceled(ReasonTag, Target);
}

void UInteractionPromptWidgetBase::HandleInteractionCompleted(UObject* Target)
{
	CachedProgressRatio = 1.0f;
	BP_OnInteractionCompleted(Target);
}
