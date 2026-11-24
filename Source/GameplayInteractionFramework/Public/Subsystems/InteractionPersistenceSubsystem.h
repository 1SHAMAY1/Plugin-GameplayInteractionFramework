#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GameplayTagContainer.h"
#include "Types/GameplayInteractionTypes.h"
#include "InteractionPersistenceSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnInteractionGlobalStateChanged,
	FGameplayTag, StateTag,
	FGameplayTag, NewValue,
	const FString&, PlayerId,
	EInteractionPersistenceScope, Scope);

/**
 * UInteractionPersistenceSubsystem
 *
 * GameInstance subsystem providing a persistent global state ledger for:
 * - Cross-level interactions and Metroidvania progression
 * - World Partition streaming cell hydration
 * - 4-player multi-mission state partitioning (WorldShared vs InstigatorScoped)
 * - Distributed multi-switch puzzle coordination across separate levels
 *
 * Survives level streaming, world unloads, and player map travel with zero dangling actor pointers.
 */
UCLASS()
class GAMEPLAYINTERACTIONFRAMEWORK_API UInteractionPersistenceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** Broadcast whenever a persistent state record is created or updated */
	UPROPERTY(BlueprintAssignable, Category = "Interaction|Persistence")
	FOnInteractionGlobalStateChanged OnGlobalStateChanged;

	/**
	 * Sets persistent state for a gameplay tag.
	 *
	 * @param StateTag The persistent state identifier (e.g. World.Door.GateA_Unlocked)
	 * @param NewValue The state value tag (e.g. Interaction.State.Completed)
	 * @param PlayerId Optional unique player ID (used when Scope is InstigatorScoped)
	 * @param Scope WorldShared (affects all players) or InstigatorScoped (personal mission)
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Persistence")
	void SetPersistentState(
		FGameplayTag StateTag,
		FGameplayTag NewValue,
		const FString& PlayerId = TEXT(""),
		EInteractionPersistenceScope Scope = EInteractionPersistenceScope::WorldShared);

	/**
	 * Retrieves persistent state for a gameplay tag.
	 *
	 * @param StateTag The persistent state identifier
	 * @param OutValue Out parameter receiving the state value tag
	 * @param PlayerId Optional player ID for InstigatorScoped lookups
	 * @param Scope WorldShared or InstigatorScoped
	 * @return True if a record exists
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction|Persistence")
	bool GetPersistentState(
		FGameplayTag StateTag,
		FGameplayTag& OutValue,
		const FString& PlayerId = TEXT(""),
		EInteractionPersistenceScope Scope = EInteractionPersistenceScope::WorldShared) const;

	/**
	 * Checks whether a persistent interaction has been completed.
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction|Persistence")
	bool HasCompleted(
		FGameplayTag StateTag,
		const FString& PlayerId = TEXT(""),
		EInteractionPersistenceScope Scope = EInteractionPersistenceScope::WorldShared) const;

	/**
	 * Verifies whether all switches in a multi-switch requirement container are active.
	 * Enables cross-level puzzles (e.g. 4 switches across 4 levels opening a single gate).
	 */
	UFUNCTION(BlueprintPure, Category = "Interaction|Persistence")
	bool CheckMultiSwitchRequirements(
		const FGameplayTagContainer& RequiredSwitchTags,
		const FGameplayTag& RequiredStateTag) const;

	/** Resets all persistent ledgers (for new game / reset test runs) */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Interaction|Persistence")
	void ResetLedger();

	/** Gets total count of WorldShared records */
	UFUNCTION(BlueprintPure, Category = "Interaction|Persistence")
	int32 GetWorldRecordCount() const { return WorldLedger.Num(); }

	/** Gets total count of records for a specific player */
	UFUNCTION(BlueprintPure, Category = "Interaction|Persistence")
	int32 GetPlayerRecordCount(const FString& PlayerId) const;

private:
	/** Global shared world ledger (doors, water levels, bridges, global switches) */
	UPROPERTY(Transient)
	TMap<FGameplayTag, FInteractionPersistentRecord> WorldLedger;

	/** Per-player mission ledgers (personal loot, individual quest dialogue/items) */
	UPROPERTY(Transient)
	TMap<FString, FInteractionPersistentRecordMap> PlayerLedgers;
};
