#include "Misc/AutomationTest.h"
#include "Components/InteractableComponent.h"
#include "Types/InteractionNativeTags.h"
#include "Settings/GameplayInteractionSettings.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractableComponentLifecycleTest, "GameplayInteractionFramework.Interactable.Lifecycle", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractableComponentLifecycleTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PropActor = World->SpawnActor<AActor>();
		TestNotNull(TEXT("PropActor must be spawned"), PropActor);

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		TestNotNull(TEXT("InteractableComponent must be created"), Comp);

		Comp->RegisterComponent();

		// Verify initial state
		TestTrue(TEXT("bIsInteractable should default to true"), Comp->bIsInteractable);
		TestEqual(TEXT("Initial state should be Available"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());

		// Disable component
		Comp->SetIsInteractable(false);
		TestFalse(TEXT("bIsInteractable should now be false"), Comp->bIsInteractable);
		TestEqual(TEXT("State should transition to Disabled"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Disabled.GetTag());

		// Re-enable component
		Comp->SetIsInteractable(true);
		TestTrue(TEXT("bIsInteractable should be true again"), Comp->bIsInteractable);
		TestEqual(TEXT("State should revert to Available"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Available.GetTag());

		PropActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractableComponentSmartPointsTest, "GameplayInteractionFramework.Interactable.SmartPoints", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractableComponentSmartPointsTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PropActor = World->SpawnActor<AActor>();
		PropActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		Comp->RegisterComponent();

		// Add two smart interaction points with local relative offsets
		FSmartInteractionPoint FrontDoorPoint;
		FrontDoorPoint.RelativeOffset = FVector(200.0f, 0.0f, 0.0f);
		FrontDoorPoint.PointTag = FGameplayTag::RequestGameplayTag(FName("Interaction.Point.DriverDoor"), false);
		Comp->SmartPoints.Add(FrontDoorPoint);

		FSmartInteractionPoint TrunkPoint;
		TrunkPoint.RelativeOffset = FVector(-200.0f, 0.0f, 0.0f);
		TrunkPoint.PointTag = FGameplayTag::RequestGameplayTag(FName("Interaction.Point.Trunk"), false);
		Comp->SmartPoints.Add(TrunkPoint);

		TestEqual(TEXT("Smart point count must be 2"), Comp->GetSmartPointCount(), 2);

		// Test world transform computation
		const FTransform FrontDoorTransform = Comp->GetSmartPointWorldTransform(0);
		TestNearlyEqual(TEXT("Front door X should be 200"), static_cast<float>(FrontDoorTransform.GetLocation().X), 200.0f, 1.0f);

		const FTransform TrunkTransform = Comp->GetSmartPointWorldTransform(1);
		TestNearlyEqual(TEXT("Trunk X should be -200"), static_cast<float>(TrunkTransform.GetLocation().X), -200.0f, 1.0f);

		// Test closest smart point query
		const int32 ClosestToFront = Comp->FindClosestSmartPointIndex(FVector(250.0f, 50.0f, 0.0f));
		TestEqual(TEXT("Closest to front should be index 0"), ClosestToFront, 0);

		const int32 ClosestToBack = Comp->FindClosestSmartPointIndex(FVector(-250.0f, 0.0f, 0.0f));
		TestEqual(TEXT("Closest to back should be index 1"), ClosestToBack, 1);

		PropActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractableComponentTagGatingTest, "GameplayInteractionFramework.Interactable.TagGating", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractableComponentTagGatingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PropActor = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(PropActor);
		PropActor->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();
		PropActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

		AActor* InstigatorActor = World->SpawnActor<AActor>();
		USceneComponent* InstigatorRoot = NewObject<USceneComponent>(InstigatorActor);
		InstigatorActor->SetRootComponent(InstigatorRoot);
		InstigatorRoot->RegisterComponent();
		InstigatorActor->SetActorLocation(FVector(50.0f, 0.0f, 0.0f));

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		Comp->InteractionDistance = 250.0f;
		Comp->RegisterComponent();

		FInteractionFrameworkContext Context;
		Context.InstigatorActor = InstigatorActor;
		Context.TargetObject = Comp;
		Context.SelectedPointIndex = 0;

		FGameplayTagContainer FailureTags;

		// Scenario 1: Clean target should be valid
		EInteractionValidationResult Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Clean target should return Valid"), Result, EInteractionValidationResult::Valid);

		// Scenario 2: Target locked
		Comp->BlockedTargetTags.AddTag(InteractionNativeTags::State_Locked.GetTag());
		Comp->TargetTags.AddTag(InteractionNativeTags::State_Locked.GetTag());
		FailureTags.Reset();
		Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Locked target should return BlockedByTags"), Result, EInteractionValidationResult::BlockedByTags);
		TestTrue(TEXT("FailureTags should include State_Locked"), FailureTags.HasTag(InteractionNativeTags::State_Locked.GetTag()));

		// Scenario 3: Target disabled
		Comp->TargetTags.Reset();
		Comp->BlockedTargetTags.Reset();
		Comp->SetIsInteractable(false);
		FailureTags.Reset();
		Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Disabled target should return BlockedByTags"), Result, EInteractionValidationResult::BlockedByTags);

		PropActor->Destroy();
		InstigatorActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractableComponentDistanceCheckTest, "GameplayInteractionFramework.Interactable.DistanceCheck", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractableComponentDistanceCheckTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PropActor = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(PropActor);
		PropActor->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();
		PropActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

		AActor* InstigatorActor = World->SpawnActor<AActor>();
		USceneComponent* InstigatorRoot = NewObject<USceneComponent>(InstigatorActor);
		InstigatorActor->SetRootComponent(InstigatorRoot);
		InstigatorRoot->RegisterComponent();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		Comp->InteractionDistance = 200.0f;
		Comp->RegisterComponent();

		FInteractionFrameworkContext Context;
		Context.InstigatorActor = InstigatorActor;
		Context.TargetObject = Comp;
		Context.SelectedPointIndex = 0;

		FGameplayTagContainer FailureTags;

		// Within range (100cm < 200cm)
		InstigatorActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));
		EInteractionValidationResult Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Instigator within range should be Valid"), Result, EInteractionValidationResult::Valid);

		// Out of range (350cm > 200cm + tolerance)
		InstigatorActor->SetActorLocation(FVector(350.0f, 0.0f, 0.0f));
		FailureTags.Reset();
		Result = Comp->EvaluateEligibility(Context, FailureTags);
		TestEqual(TEXT("Instigator far away should return OutOfRange"), Result, EInteractionValidationResult::OutOfRange);

		PropActor->Destroy();
		InstigatorActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractableComponentRepNotifyStateTest, "GameplayInteractionFramework.Interactable.RepNotifyState", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractableComponentRepNotifyStateTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PropActor = World->SpawnActor<AActor>();
		AActor* InstigatorActor = World->SpawnActor<AActor>();

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		Comp->RegisterComponent();

		const uint8 InitialCounter = Comp->InteractionState.ChangeCounter;

		// Transition to Active
		Comp->SetInteractionState(InteractionNativeTags::State_Active.GetTag(), InstigatorActor);
		TestEqual(TEXT("State should be Active"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Active.GetTag());
		TestEqual(TEXT("Instigator should be set"), Comp->InteractionState.Instigator.Get(), InstigatorActor);
		TestEqual(TEXT("ChangeCounter should increment by 1"), Comp->InteractionState.ChangeCounter, static_cast<uint8>(InitialCounter + 1));

		// Transition to Completed
		Comp->SetInteractionState(InteractionNativeTags::State_Completed.GetTag(), InstigatorActor);
		TestEqual(TEXT("State should be Completed"), Comp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());
		TestEqual(TEXT("ChangeCounter should increment by 2"), Comp->InteractionState.ChangeCounter, static_cast<uint8>(InitialCounter + 2));

		PropActor->Destroy();
		InstigatorActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractableComponentSingleUseContentionTest, "GameplayInteractionFramework.Interactable.SingleUseContention", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractableComponentSingleUseContentionTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PropActor = World->SpawnActor<AActor>();
		USceneComponent* PropRoot = NewObject<USceneComponent>(PropActor);
		PropActor->SetRootComponent(PropRoot);
		PropRoot->RegisterComponent();
		PropActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

		AActor* PlayerA = World->SpawnActor<AActor>();
		USceneComponent* PlayerARoot = NewObject<USceneComponent>(PlayerA);
		PlayerA->SetRootComponent(PlayerARoot);
		PlayerARoot->RegisterComponent();
		PlayerA->SetActorLocation(FVector(50.0f, 0.0f, 0.0f));

		AActor* PlayerB = World->SpawnActor<AActor>();
		USceneComponent* PlayerBRoot = NewObject<USceneComponent>(PlayerB);
		PlayerB->SetRootComponent(PlayerBRoot);
		PlayerBRoot->RegisterComponent();
		PlayerB->SetActorLocation(FVector(60.0f, 0.0f, 0.0f));

		UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
		Comp->InteractionDistance = 200.0f;
		Comp->bIsSingleUse = true;
		Comp->RegisterComponent();

		// Player A interacts first
		FInteractionFrameworkContext ContextA;
		ContextA.InstigatorActor = PlayerA;
		ContextA.TargetObject = Comp;

		FGameplayTagContainer FailureTagsA;
		const EInteractionValidationResult ResultA = Comp->EvaluateEligibility(ContextA, FailureTagsA);
		TestEqual(TEXT("Player A should be Valid"), ResultA, EInteractionValidationResult::Valid);

		// Player A completes interaction
		IInteractableTarget::Execute_OnInteractionCompleted(Comp, ContextA);

		// Player B attempts to interact with the single-use item after Player A completed it
		FInteractionFrameworkContext ContextB;
		ContextB.InstigatorActor = PlayerB;
		ContextB.TargetObject = Comp;

		FGameplayTagContainer FailureTagsB;
		const EInteractionValidationResult ResultB = Comp->EvaluateEligibility(ContextB, FailureTagsB);
		TestEqual(TEXT("Player B should be rejected since target is now disabled"), ResultB, EInteractionValidationResult::BlockedByTags);

		PropActor->Destroy();
		PlayerA->Destroy();
		PlayerB->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}
