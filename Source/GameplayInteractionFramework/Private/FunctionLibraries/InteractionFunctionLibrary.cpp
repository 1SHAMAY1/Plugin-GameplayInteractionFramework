#include "FunctionLibraries/InteractionFunctionLibrary.h"
#include "Interfaces/InteractionInstigator.h"
#include "GameFramework/Actor.h"

float UInteractionFunctionLibrary::CalculateDirectionalScore(float Distance, float MaxDistance, float DotAngle, int32 Priority, float WeightDot, float WeightDist, float WeightPriority)
{
	const float SafeMaxDistance = FMath::Max(1.0f, MaxDistance);
	const float NormalizedDist = FMath::Clamp(1.0f - (Distance / SafeMaxDistance), 0.0f, 1.0f);
	const float NormalizedDot = FMath::Clamp((DotAngle + 1.0f) * 0.5f, 0.0f, 1.0f);
	const float NormalizedPriority = FMath::Clamp(static_cast<float>(Priority) / 100.0f, 0.0f, 1.0f);

	return (NormalizedDot * WeightDot) + (NormalizedDist * WeightDist) + (NormalizedPriority * WeightPriority);
}

bool UInteractionFunctionLibrary::EvaluateActorTags(const AActor* Actor, const FGameplayTagQuery& RequiredTags, const FGameplayTagContainer& BlockedTags)
{
	if (!Actor)
	{
		return false;
	}

	FGameplayTagContainer ActorTags;
	if (Actor->Implements<UInteractionInstigator>())
	{
		ActorTags = IInteractionInstigator::Execute_GetInstigatorTags(Actor);
	}

	if (!BlockedTags.IsEmpty() && ActorTags.HasAny(BlockedTags))
	{
		return false;
	}

	if (!RequiredTags.IsEmpty() && !RequiredTags.Matches(ActorTags))
	{
		return false;
	}

	return true;
}

int32 UInteractionFunctionLibrary::FindClosestPointIndex(const TArray<FVector>& Points, const FVector& QueryLocation)
{
	if (Points.Num() == 0)
	{
		return INDEX_NONE;
	}

	int32 ClosestIndex = 0;
	float ClosestDistSq = MAX_flt;

	for (int32 i = 0; i < Points.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(Points[i], QueryLocation);
		if (DistSq < ClosestDistSq)
		{
			ClosestDistSq = DistSq;
			ClosestIndex = i;
		}
	}

	return ClosestIndex;
}

FText UInteractionFunctionLibrary::GetValidationResultDescription(EInteractionValidationResult Result)
{
	switch (Result)
	{
	case EInteractionValidationResult::Valid:
		return NSLOCTEXT("Interaction", "Valid", "Interaction available");
	case EInteractionValidationResult::OutOfRange:
		return NSLOCTEXT("Interaction", "OutOfRange", "Target is too far away");
	case EInteractionValidationResult::ObstructedLineOfSight:
		return NSLOCTEXT("Interaction", "ObstructedLoS", "Line of sight is obstructed");
	case EInteractionValidationResult::MissingRequiredTags:
		return NSLOCTEXT("Interaction", "MissingTags", "Missing required items or abilities");
	case EInteractionValidationResult::BlockedByTags:
		return NSLOCTEXT("Interaction", "BlockedTags", "Interaction is currently blocked");
	case EInteractionValidationResult::OnCooldown:
		return NSLOCTEXT("Interaction", "OnCooldown", "Interaction is on cooldown");
	case EInteractionValidationResult::Locked:
		return NSLOCTEXT("Interaction", "Locked", "Locked");
	case EInteractionValidationResult::AlreadyInteracted:
		return NSLOCTEXT("Interaction", "AlreadyInteracted", "Already interacted");
	case EInteractionValidationResult::TargetBusy:
		return NSLOCTEXT("Interaction", "TargetBusy", "Target is in use by another player");
	case EInteractionValidationResult::TimingViolation:
		return NSLOCTEXT("Interaction", "TimingViolation", "Hold duration requirement not met");
	case EInteractionValidationResult::CustomPolicyFailed:
	default:
		return NSLOCTEXT("Interaction", "Failed", "Interaction cannot be performed");
	}
}
