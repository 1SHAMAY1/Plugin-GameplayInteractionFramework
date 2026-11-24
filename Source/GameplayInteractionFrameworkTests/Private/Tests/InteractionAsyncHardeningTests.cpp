#include "Misc/AutomationTest.h"
#include "Components/InteractableComponent.h"
#include "Components/InteractionDetectorComponent.h"
#include "Async/AsyncAction_PerformInteraction.h"
#include "Types/InteractionNativeTags.h"
#include "Types/GameplayInteractionTypes.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

// 1. Lease Timeout Test (TTL Anti-Starvation)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionLeaseTimeoutTest, "GameplayInteractionFramework.Hardening.LeaseTimeout", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionLeaseTimeoutTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World must be created"), World);

	if (World)
	{
		AActor* Prop = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(Prop);
		Prop->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
		Comp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
		Comp->HoldDuration = 2.0f;
		Comp->InteractionLeaseDuration = 0.5f; // Fast lease duration for test
		Comp->RegisterComponent();

		AActor* Player = World->SpawnActor<AActor>();
		USceneComponent* PlayerRoot = NewObject<USceneComponent>(Player);
		Player->SetRootComponent(PlayerRoot);
		PlayerRoot->RegisterComponent();

		FInteractionFrameworkContext Context;
		Context.InstigatorActor = Player;
		Context.TargetObject = Comp;

		// Start interaction
		IInteractableTarget::Execute_OnInteractionStarted(Comp, Context);
		TestEqual(TEXT("State should transition to Active on start"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Active.GetTag());

		// Trigger lease expiration directly
		Comp->HandleLeaseExpired();

		TestEqual(TEXT("State should auto-revert to Available after lease expiry"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());

		Player->Destroy();
		Prop->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

// 2. Instigator Disconnect / Crash Cleanup Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionInstigatorDisconnectCrashTest, "GameplayInteractionFramework.Hardening.InstigatorDisconnectCrash", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionInstigatorDisconnectCrashTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World must be created"), World);

	if (World)
	{
		AActor* Prop = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(Prop);
		Prop->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
		Comp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
		Comp->HoldDuration = 3.0f;
		Comp->bAllowConcurrentInteractions = false;
		Comp->RegisterComponent();

		AActor* Player1 = World->SpawnActor<AActor>();
		USceneComponent* P1Root = NewObject<USceneComponent>(Player1);
		Player1->SetRootComponent(P1Root);
		P1Root->RegisterComponent();

		FInteractionFrameworkContext Context;
		Context.InstigatorActor = Player1;
		Context.TargetObject = Comp;

		IInteractableTarget::Execute_OnInteractionStarted(Comp, Context);
		TestEqual(TEXT("Target should be Active with Player 1"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Active.GetTag());

		// Verify Player 2 is locked out (TargetBusy)
		AActor* Player2 = World->SpawnActor<AActor>();
		USceneComponent* P2Root = NewObject<USceneComponent>(Player2);
		Player2->SetRootComponent(P2Root);
		P2Root->RegisterComponent();

		FInteractionFrameworkContext P2Context;
		P2Context.InstigatorActor = Player2;
		P2Context.TargetObject = Comp;
		FGameplayTagContainer FailureTags;
		EInteractionValidationResult ValResult = Comp->EvaluateEligibility(P2Context, FailureTags);
		TestEqual(TEXT("Player 2 should be rejected with TargetBusy while Player 1 holds"), ValResult, EInteractionValidationResult::TargetBusy);

		// Now simulate Player 1 client crashing or Alt+F4ing (Pawn destruction hook)
		Comp->HandleInstigatorEndPlay(Player1, EEndPlayReason::Destroyed);

		// Component should immediately handle OnEndPlay and release the lock
		TestEqual(TEXT("State should auto-reset to Available after Player 1 crash/destruction"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());

		// Now Player 2 should be able to interact without waiting!
		FailureTags.Reset();
		ValResult = Comp->EvaluateEligibility(P2Context, FailureTags);
		TestEqual(TEXT("Player 2 should now be Valid after Player 1 lock is freed"), ValResult, EInteractionValidationResult::Valid);

		Player2->Destroy();
		Prop->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

// 3. 3-Phase Async Commit Success Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionAsyncCommitSuccessTest, "GameplayInteractionFramework.Hardening.AsyncCommitSuccess", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionAsyncCommitSuccessTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World must be created"), World);

	if (World)
	{
		AActor* Prop = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(Prop);
		Prop->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
		Comp->bAllowConcurrentInteractions = false;
		Comp->RegisterComponent();

		AActor* Player1 = World->SpawnActor<AActor>();
		USceneComponent* P1Root = NewObject<USceneComponent>(Player1);
		Player1->SetRootComponent(P1Root);
		P1Root->RegisterComponent();

		const FGuid TxId = FGuid::NewGuid();

		// Phase 1 -> Phase 2: Begin async interaction
		const bool bBeginOk = Comp->BeginAsyncInteraction(TxId, Player1);
		TestTrue(TEXT("BeginAsyncInteraction should succeed"), bBeginOk);
		TestEqual(TEXT("State should be PendingCommit"), Comp->InteractionState.StateTag, InteractionNativeTags::State_PendingCommit.GetTag());
		TestEqual(TEXT("ActiveTransactionId should match"), Comp->GetActiveTransactionId(), TxId);

		// Verify contention: Player 2 trying to interact is rejected with TargetBusy
		AActor* Player2 = World->SpawnActor<AActor>();
		USceneComponent* P2Root = NewObject<USceneComponent>(Player2);
		Player2->SetRootComponent(P2Root);
		P2Root->RegisterComponent();

		FInteractionFrameworkContext P2Context;
		P2Context.InstigatorActor = Player2;
		P2Context.TargetObject = Comp;
		FGameplayTagContainer FailureTags;
		const EInteractionValidationResult ValResult = Comp->EvaluateEligibility(P2Context, FailureTags);
		TestEqual(TEXT("Comp in PendingCommit must reject other players with TargetBusy"), ValResult, EInteractionValidationResult::TargetBusy);

		// Phase 3A: Commit async interaction on success
		const bool bCommitOk = Comp->CommitAsyncInteraction(TxId, Player1);
		TestTrue(TEXT("CommitAsyncInteraction should succeed"), bCommitOk);
		TestEqual(TEXT("State should now be Completed"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());
		TestFalse(TEXT("ActiveTransactionId should be invalidated after commit"), Comp->GetActiveTransactionId().IsValid());

		Player1->Destroy();
		Player2->Destroy();
		Prop->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

// 4. 3-Phase Async Rollback Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionAsyncRollbackTest, "GameplayInteractionFramework.Hardening.AsyncRollback", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionAsyncRollbackTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World must be created"), World);

	if (World)
	{
		AActor* Prop = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(Prop);
		Prop->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
		Comp->RegisterComponent();

		AActor* Player = World->SpawnActor<AActor>();
		USceneComponent* PRoot = NewObject<USceneComponent>(Player);
		Player->SetRootComponent(PRoot);
		PRoot->RegisterComponent();

		const FGuid TxId = FGuid::NewGuid();

		Comp->BeginAsyncInteraction(TxId, Player);
		TestEqual(TEXT("State should be PendingCommit"), Comp->InteractionState.StateTag, InteractionNativeTags::State_PendingCommit.GetTag());

		// Simulate async DB / server failure: Rollback
		const bool bRollbackOk = Comp->RollbackAsyncInteraction(TxId, InteractionNativeTags::CancelReason_TransactionFailed.GetTag(), Player);
		TestTrue(TEXT("RollbackAsyncInteraction should succeed"), bRollbackOk);
		TestEqual(TEXT("State should cleanly reset to Available with zero data corruption"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());
		TestFalse(TEXT("ActiveTransactionId should be cleared after rollback"), Comp->GetActiveTransactionId().IsValid());

		Player->Destroy();
		Prop->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

// 5. Idempotent Transaction Identity Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionIdempotentTransactionTest, "GameplayInteractionFramework.Hardening.IdempotentTransaction", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionIdempotentTransactionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World must be created"), World);

	if (World)
	{
		AActor* Prop = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(Prop);
		Prop->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(Prop);
		Comp->RegisterComponent();

		AActor* Player = World->SpawnActor<AActor>();
		USceneComponent* PRoot = NewObject<USceneComponent>(Player);
		Player->SetRootComponent(PRoot);
		PRoot->RegisterComponent();

		const FGuid LegitimateTxId = FGuid::NewGuid();
		const FGuid StaleOrForgedTxId = FGuid::NewGuid();

		Comp->BeginAsyncInteraction(LegitimateTxId, Player);

		// Attempt to commit with wrong / stale transaction GUID
		const bool bForgedCommit = Comp->CommitAsyncInteraction(StaleOrForgedTxId, Player);
		TestFalse(TEXT("Commit with mismatched TransactionId must be rejected"), bForgedCommit);
		TestEqual(TEXT("Target must remain in PendingCommit state"), Comp->InteractionState.StateTag, InteractionNativeTags::State_PendingCommit.GetTag());

		// Now commit with legitimate TransactionId
		const bool bLegitCommit = Comp->CommitAsyncInteraction(LegitimateTxId, Player);
		TestTrue(TEXT("Commit with matching TransactionId must succeed"), bLegitCommit);
		TestEqual(TEXT("Target must transition to Completed"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());

		Player->Destroy();
		Prop->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

// 6. Blueprint Async Action Node Lifecycle Test
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionAsyncActionNodeTest, "GameplayInteractionFramework.Hardening.AsyncActionNode", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionAsyncActionNodeTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("World must be created"), World);

	if (World)
	{
		AActor* PlayerActor = World->SpawnActor<AActor>();
		USceneComponent* PRoot = NewObject<USceneComponent>(PlayerActor);
		PlayerActor->SetRootComponent(PRoot);
		PRoot->RegisterComponent();

		UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(PlayerActor);
		Detector->RegisterComponent();

		AActor* PropActor = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(PropActor);
		PropActor->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();
		PropActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

		UInteractableComponent* Interactable = NewObject<UInteractableComponent>(PropActor);
		Interactable->InteractionModeTag = InteractionNativeTags::Mode_Instant.GetTag();
		Interactable->InteractionDistance = 300.0f;
		Interactable->RegisterComponent();

		// Detector discovers target
		Detector->PerformDetection();

		// Create Async Action node
		UAsyncAction_PerformInteraction* Action = UAsyncAction_PerformInteraction::PerformInteractionAsync(World, Detector, Interactable, 5.0f);
		TestNotNull(TEXT("Async Action node must be created"), Action);

		if (Action)
		{
			Action->Activate();

			// For instant mode, detector immediately completes and fires OnCompleted
			Action->SetReadyToDestroy();
		}

		PropActor->Destroy();
		PlayerActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}
