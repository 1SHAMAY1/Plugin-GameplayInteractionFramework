#include "Misc/AutomationTest.h"
#include "Subsystems/InteractionPersistenceSubsystem.h"
#include "Components/InteractableComponent.h"
#include "Types/InteractionNativeTags.h"
#include "Types/GameplayInteractionTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"

// 1. Persistence Subsystem Ledger Creation & Query Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPersistenceSubsystemLedgerTest, "GameplayInteractionFramework.Persistence.SubsystemLedger", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FPersistenceSubsystemLedgerTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance must be created"), GI);

	if (GI)
	{
		UInteractionPersistenceSubsystem* Subsystem = NewObject<UInteractionPersistenceSubsystem>(GI);
		TestNotNull(TEXT("UInteractionPersistenceSubsystem must be created"), Subsystem);

		if (Subsystem)
		{
			const FGameplayTag StateTag = InteractionNativeTags::State_Active.GetTag();
			const FGameplayTag CompletedTag = InteractionNativeTags::State_Completed.GetTag();

			TestFalse(TEXT("Initially state should not exist"), Subsystem->HasCompleted(StateTag));

			Subsystem->SetPersistentState(StateTag, CompletedTag, TEXT("Player_Alpha"), EInteractionPersistenceScope::WorldShared);

			TestTrue(TEXT("State should be reported as completed"), Subsystem->HasCompleted(StateTag));
			TestEqual(TEXT("World record count should be 1"), Subsystem->GetWorldRecordCount(), 1);

			FGameplayTag OutValue;
			const bool bFound = Subsystem->GetPersistentState(StateTag, OutValue, TEXT(""), EInteractionPersistenceScope::WorldShared);
			TestTrue(TEXT("Record should be retrievable"), bFound);
			TestEqual(TEXT("Retrieved state tag must match Completed"), OutValue, CompletedTag);

			Subsystem->ResetLedger();
			TestEqual(TEXT("World record count should be 0 after reset"), Subsystem->GetWorldRecordCount(), 0);
			TestFalse(TEXT("State should not exist after reset"), Subsystem->HasCompleted(StateTag));
		}
	}

	return true;
}

// 2. Stream-In Hydration Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStreamInHydrationTest, "GameplayInteractionFramework.Persistence.StreamInHydration", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FStreamInHydrationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance must be created"), GI);

	if (GI)
	{
		UInteractionPersistenceSubsystem* Subsystem = NewObject<UInteractionPersistenceSubsystem>(GI);
		TestNotNull(TEXT("Persistence subsystem must be created"), Subsystem);

		const FGameplayTag DoorPersistentTag = InteractionNativeTags::State_Completed.GetTag();
		Subsystem->SetPersistentState(DoorPersistentTag, InteractionNativeTags::State_Completed.GetTag(), TEXT(""), EInteractionPersistenceScope::WorldShared);

		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
		TestNotNull(TEXT("World must be created"), World);

		if (World)
		{
			AActor* Prop = World->SpawnActor<AActor>();
			USceneComponent* Root = NewObject<USceneComponent>(Prop);
			Prop->SetRootComponent(Root);
			Root->RegisterComponent();

			UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
			Comp->PersistentStateTag = DoorPersistentTag;
			Comp->PersistenceScope = EInteractionPersistenceScope::WorldShared;
			Comp->bIsSingleUse = true;

			// Verify that before hydration the default is Available
			TestEqual(TEXT("Default state before hydration is Available"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());

			// Simulate stream-in hydration by querying the subsystem
			FGameplayTag SavedState;
			if (Subsystem->GetPersistentState(Comp->PersistentStateTag, SavedState, TEXT(""), Comp->PersistenceScope))
			{
				if (SavedState == InteractionNativeTags::State_Completed.GetTag())
				{
					Comp->SetIsInteractable(false);
				}
			}

			TestFalse(TEXT("Single-use interactable should be disabled after hydration"), Comp->bIsInteractable);
			TestEqual(TEXT("State should be Disabled after hydration"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Disabled.GetTag());

			Prop->Destroy();
			World->DestroyWorld(false);
		}
	}

	return true;
}

// 3. Metroidvania Scoped Persistence Test (4 Players, 4 Missions)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMetroidvaniaScopedPersistenceTest, "GameplayInteractionFramework.Persistence.MetroidvaniaScoped", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMetroidvaniaScopedPersistenceTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance must be created"), GI);

	if (GI)
	{
		UInteractionPersistenceSubsystem* Subsystem = NewObject<UInteractionPersistenceSubsystem>(GI);
		TestNotNull(TEXT("Subsystem must be created"), Subsystem);

		const FGameplayTag GlobalBridgeTag = InteractionNativeTags::State_Completed.GetTag();
		const FGameplayTag ChestMissionATag = InteractionNativeTags::State_Active.GetTag();
		const FGameplayTag ChestMissionBTag = InteractionNativeTags::State_Locked.GetTag();

		// Player 1 in Level 1 activates a WorldShared bridge
		Subsystem->SetPersistentState(GlobalBridgeTag, InteractionNativeTags::State_Completed.GetTag(), TEXT("Player1"), EInteractionPersistenceScope::WorldShared);

		// All 4 players in all 4 levels observe the bridge is extended
		TestTrue(TEXT("Player 1 sees bridge extended"), Subsystem->HasCompleted(GlobalBridgeTag, TEXT("Player1"), EInteractionPersistenceScope::WorldShared));
		TestTrue(TEXT("Player 2 sees bridge extended"), Subsystem->HasCompleted(GlobalBridgeTag, TEXT("Player2"), EInteractionPersistenceScope::WorldShared));
		TestTrue(TEXT("Player 3 sees bridge extended"), Subsystem->HasCompleted(GlobalBridgeTag, TEXT("Player3"), EInteractionPersistenceScope::WorldShared));
		TestTrue(TEXT("Player 4 sees bridge extended"), Subsystem->HasCompleted(GlobalBridgeTag, TEXT("Player4"), EInteractionPersistenceScope::WorldShared));

		// Player 2 in Level 2 completes personal Mission A chest (InstigatorScoped)
		Subsystem->SetPersistentState(ChestMissionATag, InteractionNativeTags::State_Completed.GetTag(), TEXT("Player2"), EInteractionPersistenceScope::InstigatorScoped);

		// Player 2's mission chest is completed, but remains unlooted for Players 1, 3, and 4!
		TestTrue(TEXT("Player 2 completed Mission A chest"), Subsystem->HasCompleted(ChestMissionATag, TEXT("Player2"), EInteractionPersistenceScope::InstigatorScoped));
		TestFalse(TEXT("Player 1 has NOT completed Mission A chest"), Subsystem->HasCompleted(ChestMissionATag, TEXT("Player1"), EInteractionPersistenceScope::InstigatorScoped));
		TestFalse(TEXT("Player 3 has NOT completed Mission A chest"), Subsystem->HasCompleted(ChestMissionATag, TEXT("Player3"), EInteractionPersistenceScope::InstigatorScoped));
		TestFalse(TEXT("Player 4 has NOT completed Mission A chest"), Subsystem->HasCompleted(ChestMissionATag, TEXT("Player4"), EInteractionPersistenceScope::InstigatorScoped));

		// Player 4 in Level 4 completes personal Mission B chest
		Subsystem->SetPersistentState(ChestMissionBTag, InteractionNativeTags::State_Completed.GetTag(), TEXT("Player4"), EInteractionPersistenceScope::InstigatorScoped);

		TestTrue(TEXT("Player 4 completed Mission B chest"), Subsystem->HasCompleted(ChestMissionBTag, TEXT("Player4"), EInteractionPersistenceScope::InstigatorScoped));
		TestFalse(TEXT("Player 2 has NOT completed Mission B chest"), Subsystem->HasCompleted(ChestMissionBTag, TEXT("Player2"), EInteractionPersistenceScope::InstigatorScoped));

		TestEqual(TEXT("Player 2 should have 1 personal mission record"), Subsystem->GetPlayerRecordCount(TEXT("Player2")), 1);
		TestEqual(TEXT("Player 4 should have 1 personal mission record"), Subsystem->GetPlayerRecordCount(TEXT("Player4")), 1);
	}

	return true;
}

// 4. Cross-Level Multi-Switch Coordination Test (4 Switches Across 4 Levels)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FCrossLevelMultiSwitchCoordinationTest, "GameplayInteractionFramework.Persistence.MultiSwitchCoordination", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FCrossLevelMultiSwitchCoordinationTest::RunTest(const FString& Parameters)
{
	UGameInstance* GI = NewObject<UGameInstance>();
	TestNotNull(TEXT("GameInstance must be created"), GI);

	if (GI)
	{
		UInteractionPersistenceSubsystem* Subsystem = NewObject<UInteractionPersistenceSubsystem>(GI);
		TestNotNull(TEXT("Subsystem must be created"), Subsystem);

		const FGameplayTag Switch1 = InteractionNativeTags::Mode_Instant.GetTag();
		const FGameplayTag Switch2 = InteractionNativeTags::Mode_Hold.GetTag();
		const FGameplayTag Switch3 = InteractionNativeTags::Mode_RapidTap.GetTag();
		const FGameplayTag Switch4 = InteractionNativeTags::Mode_Toggle.GetTag();

		FGameplayTagContainer RequiredSwitches;
		RequiredSwitches.AddTag(Switch1);
		RequiredSwitches.AddTag(Switch2);
		RequiredSwitches.AddTag(Switch3);
		RequiredSwitches.AddTag(Switch4);

		const FGameplayTag CompletedTag = InteractionNativeTags::State_Completed.GetTag();

		// Activate 3 out of 4 switches in Levels 1, 2, and 3
		Subsystem->SetPersistentState(Switch1, CompletedTag, TEXT("Player1"), EInteractionPersistenceScope::WorldShared);
		Subsystem->SetPersistentState(Switch2, CompletedTag, TEXT("Player2"), EInteractionPersistenceScope::WorldShared);
		Subsystem->SetPersistentState(Switch3, CompletedTag, TEXT("Player3"), EInteractionPersistenceScope::WorldShared);

		// Condition not met yet
		TestFalse(TEXT("3 out of 4 switches must NOT satisfy cross-level puzzle condition"),
			Subsystem->CheckMultiSwitchRequirements(RequiredSwitches, CompletedTag));

		// Player 4 in Level 4 activates Switch 4
		Subsystem->SetPersistentState(Switch4, CompletedTag, TEXT("Player4"), EInteractionPersistenceScope::WorldShared);

		// All 4 switches active across 4 levels!
		TestTrue(TEXT("4 out of 4 switches active across 4 levels must satisfy puzzle condition"),
			Subsystem->CheckMultiSwitchRequirements(RequiredSwitches, CompletedTag));
	}

	return true;
}
