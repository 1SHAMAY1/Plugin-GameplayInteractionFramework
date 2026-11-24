#include "Subsystems/InteractionPersistenceSubsystem.h"
#include "Types/InteractionNativeTags.h"
#include "HAL/PlatformTime.h"

void UInteractionPersistenceSubsystem::SetPersistentState(
	FGameplayTag StateTag,
	FGameplayTag NewValue,
	const FString& PlayerId,
	EInteractionPersistenceScope Scope)
{
	if (!StateTag.IsValid())
	{
		return;
	}

	const double Now = FPlatformTime::Seconds();

	if (Scope == EInteractionPersistenceScope::WorldShared)
	{
		FInteractionPersistentRecord& Record = WorldLedger.FindOrAdd(StateTag);
		Record.StateTag = NewValue;
		Record.CompletedByPlayerId = PlayerId;
		Record.CompletionTimestamp = Now;
	}
	else
	{
		const FString EffectivePlayerId = PlayerId.IsEmpty() ? TEXT("DefaultPlayer") : PlayerId;
		FInteractionPersistentRecordMap& Map = PlayerLedgers.FindOrAdd(EffectivePlayerId);
		FInteractionPersistentRecord& Record = Map.Records.FindOrAdd(StateTag);
		Record.StateTag = NewValue;
		Record.CompletedByPlayerId = EffectivePlayerId;
		Record.CompletionTimestamp = Now;
	}

	OnGlobalStateChanged.Broadcast(StateTag, NewValue, PlayerId, Scope);
}

bool UInteractionPersistenceSubsystem::GetPersistentState(
	FGameplayTag StateTag,
	FGameplayTag& OutValue,
	const FString& PlayerId,
	EInteractionPersistenceScope Scope) const
{
	if (!StateTag.IsValid())
	{
		return false;
	}

	if (Scope == EInteractionPersistenceScope::WorldShared)
	{
		if (const FInteractionPersistentRecord* Found = WorldLedger.Find(StateTag))
		{
			OutValue = Found->StateTag;
			return true;
		}
	}
	else
	{
		const FString EffectivePlayerId = PlayerId.IsEmpty() ? TEXT("DefaultPlayer") : PlayerId;
		if (const FInteractionPersistentRecordMap* FoundMap = PlayerLedgers.Find(EffectivePlayerId))
		{
			if (const FInteractionPersistentRecord* Found = FoundMap->Records.Find(StateTag))
			{
				OutValue = Found->StateTag;
				return true;
			}
		}
	}

	return false;
}

bool UInteractionPersistenceSubsystem::HasCompleted(
	FGameplayTag StateTag,
	const FString& PlayerId,
	EInteractionPersistenceScope Scope) const
{
	FGameplayTag Value;
	if (GetPersistentState(StateTag, Value, PlayerId, Scope))
	{
		return Value == InteractionNativeTags::State_Completed.GetTag();
	}

	return false;
}

bool UInteractionPersistenceSubsystem::CheckMultiSwitchRequirements(
	const FGameplayTagContainer& RequiredSwitchTags,
	const FGameplayTag& RequiredStateTag) const
{
	if (RequiredSwitchTags.IsEmpty())
	{
		return true;
	}

	const FGameplayTag TargetState = RequiredStateTag.IsValid() ? RequiredStateTag : InteractionNativeTags::State_Completed.GetTag();

	for (const FGameplayTag& SwitchTag : RequiredSwitchTags)
	{
		FGameplayTag CurrentVal;
		if (!GetPersistentState(SwitchTag, CurrentVal, TEXT(""), EInteractionPersistenceScope::WorldShared))
		{
			return false;
		}

		if (CurrentVal != TargetState)
		{
			return false;
		}
	}

	return true;
}

void UInteractionPersistenceSubsystem::ResetLedger()
{
	WorldLedger.Empty();
	PlayerLedgers.Empty();
}

int32 UInteractionPersistenceSubsystem::GetPlayerRecordCount(const FString& PlayerId) const
{
	const FString EffectivePlayerId = PlayerId.IsEmpty() ? TEXT("DefaultPlayer") : PlayerId;
	if (const FInteractionPersistentRecordMap* FoundMap = PlayerLedgers.Find(EffectivePlayerId))
	{
		return FoundMap->Records.Num();
	}

	return 0;
}
