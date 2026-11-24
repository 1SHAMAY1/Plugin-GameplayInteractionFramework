#include "Misc/AutomationTest.h"
#include "Components/InteractableComponent.h"
#include "Components/InteractionDetectorComponent.h"
#include "Types/InteractionNativeTags.h"
#include "Settings/GameplayInteractionSettings.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServerAuthorityLoSOcclusionTest, "GameplayInteractionFramework.Authority.LoSOcclusion", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerAuthorityLoSOcclusionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		// Spawn Instigator at X = 0
		AActor* InstigatorActor = World->SpawnActor<AActor>();
		USceneComponent* InstigatorRoot = NewObject<USceneComponent>(InstigatorActor);
		InstigatorActor->SetRootComponent(InstigatorRoot);
		InstigatorRoot->RegisterComponent();
		InstigatorActor->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));

		// Spawn Target at X = 200
		AActor* TargetActor = World->SpawnActor<AActor>();
		USceneComponent* TargetRoot = NewObject<USceneComponent>(TargetActor);
		TargetActor->SetRootComponent(TargetRoot);
		TargetRoot->RegisterComponent();
		TargetActor->SetActorLocation(FVector(200.0f, 0.0f, 100.0f));

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(TargetActor);
		Comp->InteractionDistance = 500.0f;
		Comp->bRequireLineOfSight = true;
		Comp->RegisterComponent();

		// Case 1: Clear line of sight (no obstacles)
		FInteractionFrameworkContext Context;
		Context.InstigatorActor = InstigatorActor;
		Context.TargetObject = Comp;
		Context.SelectedPointIndex = 0;

		FGameplayTagContainer FailureTags;
		EInteractionValidationResult Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Clear LoS should evaluate to Valid"), Result, EInteractionValidationResult::Valid);

		// Case 2: Obstructed line of sight (spawn blocking wall at X = 100)
		AActor* WallActor = World->SpawnActor<AActor>();
		UBoxComponent* BoxComp = NewObject<UBoxComponent>(WallActor);
		BoxComp->InitBoxExtent(FVector(20.0f, 100.0f, 100.0f));
		BoxComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		BoxComp->SetCollisionResponseToAllChannels(ECR_Block);
		WallActor->SetRootComponent(BoxComp);
		BoxComp->RegisterComponent();
		WallActor->SetActorLocation(FVector(100.0f, 0.0f, 100.0f));

		FailureTags.Reset();
		Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Obstructed LoS should return ObstructedLineOfSight"), Result, EInteractionValidationResult::ObstructedLineOfSight);
		TestTrue(TEXT("FailureTags should include CancelReason_LineOfSightLost"), FailureTags.HasTag(InteractionNativeTags::CancelReason_LineOfSightLost.GetTag()));

		WallActor->Destroy();
		TargetActor->Destroy();
		InstigatorActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServerHoldAntiSpeedhackTest, "GameplayInteractionFramework.Authority.HoldAntiSpeedhack", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerHoldAntiSpeedhackTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* InstigatorActor = World->SpawnActor<AActor>();
		AActor* TargetActor = World->SpawnActor<AActor>();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(TargetActor);
		Comp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
		Comp->HoldDuration = 2.0f;
		Comp->RegisterComponent();

		// Record hold start on server at current world time
		Comp->RecordServerHoldStart(InstigatorActor);

		// Immediate completion attempt with no elapsed time (speedhack / packet injection simulation)
		float ElapsedTime = 0.0f;
		const bool bPassed = Comp->ValidateServerHoldTiming(InstigatorActor, Comp->HoldDuration, ElapsedTime);

		TestFalse(TEXT("Premature hold completion must fail server anti-speedhack check"), bPassed);
		TestNearlyEqual(TEXT("Elapsed time must be ~0.0s"), ElapsedTime, 0.0f, 0.05f);

		InstigatorActor->Destroy();
		TargetActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServerHoldLagCompensationTest, "GameplayInteractionFramework.Authority.HoldLagCompensation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerHoldLagCompensationTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* InstigatorActor = World->SpawnActor<AActor>();
		AActor* TargetActor = World->SpawnActor<AActor>();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(TargetActor);
		Comp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
		Comp->HoldDuration = 1.0f;
		Comp->RegisterComponent();

		const double CurrentWorldTime = World->GetTimeSeconds();

		// Case 1: Legitimate client with acceptable ping latency
		// Hold duration = 1.0s, default NetworkHoldTimeTolerance = 0.15s -> Min allowed = 0.85s
		// Client completed after 0.90s of server elapsed time (within tolerance window)
		Comp->RecordServerHoldStart(InstigatorActor, CurrentWorldTime - 0.90);
		float ElapsedTime1 = 0.0f;
		const bool bLegitimatePassed = Comp->ValidateServerHoldTiming(InstigatorActor, Comp->HoldDuration, ElapsedTime1);

		TestTrue(TEXT("Legitimate hold within latency tolerance (0.90s >= 0.85s) must be accepted"), bLegitimatePassed);
		TestNearlyEqual(TEXT("Calculated elapsed time should match simulated delta"), ElapsedTime1, 0.90f, 0.05f);

		// Case 2: Blatant cheat / premature release outside tolerance window (0.50s < 0.85s)
		Comp->RecordServerHoldStart(InstigatorActor, CurrentWorldTime - 0.50);
		float ElapsedTime2 = 0.0f;
		const bool bCheatPassed = Comp->ValidateServerHoldTiming(InstigatorActor, Comp->HoldDuration, ElapsedTime2);

		TestFalse(TEXT("Premature hold outside tolerance window (0.50s < 0.85s) must be rejected"), bCheatPassed);
		TestNearlyEqual(TEXT("Calculated elapsed time should match simulated delta"), ElapsedTime2, 0.50f, 0.05f);

		InstigatorActor->Destroy();
		TargetActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServerSimultaneousContentionRaceTest, "GameplayInteractionFramework.Authority.SimultaneousContentionRace", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerSimultaneousContentionRaceTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* Player1 = World->SpawnActor<AActor>();
		USceneComponent* P1Root = NewObject<USceneComponent>(Player1);
		Player1->SetRootComponent(P1Root);
		P1Root->RegisterComponent();
		Player1->SetActorLocation(FVector(0.0f, 0.0f, 100.0f));

		AActor* Player2 = World->SpawnActor<AActor>();
		USceneComponent* P2Root = NewObject<USceneComponent>(Player2);
		Player2->SetRootComponent(P2Root);
		P2Root->RegisterComponent();
		Player2->SetActorLocation(FVector(0.0f, 50.0f, 100.0f));

		AActor* PropActor = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(PropActor);
		PropActor->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();
		PropActor->SetActorLocation(FVector(100.0f, 0.0f, 100.0f));

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		Comp->bAllowConcurrentInteractions = false; // Mutually exclusive contention lock
		Comp->InteractionDistance = 500.0f;
		Comp->bRequireLineOfSight = false;
		Comp->RegisterComponent();

		// Player 1 claims interaction first (Server transitions state to Active)
		Comp->SetInteractionState(InteractionNativeTags::State_Active.GetTag(), Player1);

		TestEqual(TEXT("State must be Active"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Active.GetTag());
		TestEqual(TEXT("Active instigator must be Player 1"), Comp->InteractionState.Instigator.Get(), Player1);

		// Player 2 simultaneously attempts to interact with the active target on the same frame
		FInteractionFrameworkContext P2Context;
		P2Context.InstigatorActor = Player2;
		P2Context.TargetObject = Comp;
		P2Context.SelectedPointIndex = 0;

		FGameplayTagContainer FailureTags;
		const EInteractionValidationResult P2Result = Comp->EvaluateEligibility(P2Context, FailureTags);

		TestEqual(TEXT("Player 2 should be rejected due to contention (TargetBusy)"), P2Result, EInteractionValidationResult::TargetBusy);
		TestTrue(TEXT("FailureTags must contain CancelReason_TargetBusy"), FailureTags.HasTag(InteractionNativeTags::CancelReason_TargetBusy.GetTag()));

		// Confirm Player 1 ownership and active state were preserved without interruption
		TestEqual(TEXT("Active instigator remains Player 1"), Comp->InteractionState.Instigator.Get(), Player1);
		TestEqual(TEXT("State remains Active"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Active.GetTag());

		Player1->Destroy();
		Player2->Destroy();
		PropActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FServerRepNotifyLateJoinerTest, "GameplayInteractionFramework.Authority.RepNotifyLateJoiner", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FServerRepNotifyLateJoinerTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* ServerProp = World->SpawnActor<AActor>();
		UInteractableComponent* ServerComp = NewObject<UInteractableComponent>(ServerProp);
		ServerComp->RegisterComponent();

		AActor* Player1 = World->SpawnActor<AActor>();

		// Server authoritative transition: object was used and completed by Player 1
		ServerComp->SetInteractionState(InteractionNativeTags::State_Completed.GetTag(), Player1);

		const FInteractionReplicatedState ServerState = ServerComp->InteractionState;
		TestEqual(TEXT("Server state tag must be Completed"), ServerState.StateTag, InteractionNativeTags::State_Completed.GetTag());
		TestEqual(TEXT("Server instigator must be Player 1"), ServerState.Instigator.Get(), Player1);
		TestTrue(TEXT("ChangeCounter must be incremented"), ServerState.ChangeCounter > 0);

		// Simulate client joining late: receives initial replicated property bundle
		AActor* ClientProp = World->SpawnActor<AActor>();
		UInteractableComponent* ClientComp = NewObject<UInteractableComponent>(ClientProp);
		ClientComp->RegisterComponent();

		// Initial state on client before replication arrives
		TestEqual(TEXT("Client default initial state is Available"), ClientComp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());

		// Replicated state arrives from server
		const FInteractionReplicatedState PreviousClientState = ClientComp->InteractionState;
		ClientComp->InteractionState = ServerState;
		ClientComp->OnRep_InteractionState(PreviousClientState);

		// Verify client has fully synchronized to authoritative server state via RepNotify
		TestEqual(TEXT("Client state must match server Completed state"), ClientComp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());
		TestEqual(TEXT("Client instigator must match server Player 1"), ClientComp->InteractionState.Instigator.Get(), Player1);
		TestEqual(TEXT("Client ChangeCounter must match server ChangeCounter"), ClientComp->InteractionState.ChangeCounter, ServerState.ChangeCounter);

		ServerProp->Destroy();
		ClientProp->Destroy();
		Player1->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}
