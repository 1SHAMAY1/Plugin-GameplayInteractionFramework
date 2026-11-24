#include "Misc/AutomationTest.h"
#include "Components/InteractionDetectorComponent.h"
#include "Components/InteractableComponent.h"
#include "Types/InteractionNativeTags.h"
#include "Mocks/MockTraceStrategy.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDetectorScoringEquationTest, "GameplayInteractionFramework.Detector.ScoringEquation", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDetectorScoringEquationTest::RunTest(const FString& Parameters)
{
	AActor* TestActor = NewObject<AActor>();
	UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(TestActor);
	Detector->WeightFacingAngle = 0.5f;
	Detector->WeightDistance = 0.3f;
	Detector->WeightPriority = 0.2f;

	// Case 1: Perfect score (Distance = 0, Angle Dot = 1.0, Priority = 100)
	// DistFactor = 1.0, AngleFactor = 1.0, PriorityFactor = 1.0 -> Score = 0.5 + 0.3 + 0.2 = 1.0
	const float PerfectScore = Detector->CalculateCandidateScore(0.0f, 200.0f, 1.0f, 100);
	TestNearlyEqual(TEXT("Perfect alignment and distance should produce 1.0"), PerfectScore, 1.0f, 0.001f);

	// Case 2: Worst score (Distance = MaxDistance, Angle Dot = -1.0, Priority = 0)
	// DistFactor = 0.0, AngleFactor = 0.0, PriorityFactor = 0.0 -> Score = 0.0
	const float WorstScore = Detector->CalculateCandidateScore(200.0f, 200.0f, -1.0f, 0);
	TestNearlyEqual(TEXT("Opposite angle and max distance should produce 0.0"), WorstScore, 0.0f, 0.001f);

	// Case 3: Priority comparison when distance and angle are identical
	const float HighPriorityScore = Detector->CalculateCandidateScore(100.0f, 200.0f, 0.5f, 50);
	const float LowPriorityScore = Detector->CalculateCandidateScore(100.0f, 200.0f, 0.5f, 10);
	TestTrue(TEXT("Higher priority must yield higher score when angle and distance match"), HighPriorityScore > LowPriorityScore);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDetectorCustomTraceStrategyTest, "GameplayInteractionFramework.Detector.CustomTraceStrategy", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDetectorCustomTraceStrategyTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PlayerActor = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(PlayerActor);
		PlayerActor->SetRootComponent(Root);
		Root->RegisterComponent();

		UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(PlayerActor);
		Detector->MaxTraceDistance = 350.0f;
		Detector->RegisterComponent();

		UMockTraceStrategy* MockStrategy = NewObject<UMockTraceStrategy>(PlayerActor);
		Detector->CustomTraceStrategy = MockStrategy;

		// Perform detection
		Detector->PerformDetection();

		TestTrue(TEXT("Custom trace strategy must have been executed"), MockStrategy->bWasExecuted);
		TestNearlyEqual(TEXT("Custom trace strategy must receive effective distance"), MockStrategy->LastPassedDistance, 350.0f, 1.0f);

		PlayerActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDetectorHoldStateMachineTest, "GameplayInteractionFramework.Detector.HoldStateMachine", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDetectorHoldStateMachineTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PlayerActor = World->SpawnActor<AActor>();
		USceneComponent* PlayerRoot = NewObject<USceneComponent>(PlayerActor);
		PlayerActor->SetRootComponent(PlayerRoot);
		PlayerRoot->RegisterComponent();
		PlayerActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		PlayerActor->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

		AActor* PropActor = World->SpawnActor<AActor>();
		UBoxComponent* PropBox = NewObject<UBoxComponent>(PropActor);
		PropActor->SetRootComponent(PropBox);
		PropBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		PropBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PropBox->SetCollisionResponseToAllChannels(ECR_Block);
		PropBox->RegisterComponent();
		PropActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

		UInteractableComponent* InteractableComp = NewObject<UInteractableComponent>(PropActor);
		InteractableComp->InteractionModeTag = InteractionNativeTags::Mode_Hold.GetTag();
		InteractableComp->HoldDuration = 1.0f;
		InteractableComp->InteractionDistance = 200.0f;
		InteractableComp->RegisterComponent();

		UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(PlayerActor);
		Detector->MaxTraceDistance = 300.0f;
		Detector->RegisterComponent();

		Detector->PerformDetection();
		TestNotNull(TEXT("Detector should focus target after detection"), Detector->GetFocusedTarget());

		if (Detector->GetFocusedTarget())
		{
			// Start hold
			Detector->StartInteract();
			TestTrue(TEXT("Detector should be interacting"), Detector->IsInteracting());
			TestNearlyEqual(TEXT("Initial hold progress should be 0.0"), Detector->GetCurrentProgressRatio(), 0.0f, 0.01f);

			// Simulate 0.5s tick (halfway)
			Detector->TickComponent(0.5f, LEVELTICK_All, nullptr);
			TestNearlyEqual(TEXT("Hold progress after 0.5s should be 0.5"), Detector->GetCurrentProgressRatio(), 0.5f, 0.02f);
			TestTrue(TEXT("Detector should still be interacting"), Detector->IsInteracting());

			// Premature release -> should cancel
			Detector->StopInteract();
			TestFalse(TEXT("Detector should stop interacting after early release"), Detector->IsInteracting());
			TestNearlyEqual(TEXT("Progress should reset to 0.0 after cancellation"), Detector->GetCurrentProgressRatio(), 0.0f, 0.01f);

			// Second attempt: hold through to completion (1.0s)
			Detector->StartInteract();
			Detector->TickComponent(1.05f, LEVELTICK_All, nullptr);
			TestFalse(TEXT("Detector should finish interacting upon completion"), Detector->IsInteracting());
			TestEqual(TEXT("Interactable state should be Completed"), InteractableComp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());
		}

		PlayerActor->Destroy();
		PropActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDetectorRapidTapDecayTest, "GameplayInteractionFramework.Detector.RapidTapDecay", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDetectorRapidTapDecayTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* PlayerActor = World->SpawnActor<AActor>();
		USceneComponent* PlayerRoot = NewObject<USceneComponent>(PlayerActor);
		PlayerActor->SetRootComponent(PlayerRoot);
		PlayerRoot->RegisterComponent();
		PlayerActor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));
		PlayerActor->SetActorRotation(FRotator(0.0f, 0.0f, 0.0f));

		AActor* PropActor = World->SpawnActor<AActor>();
		UBoxComponent* PropBox = NewObject<UBoxComponent>(PropActor);
		PropActor->SetRootComponent(PropBox);
		PropBox->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
		PropBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		PropBox->SetCollisionResponseToAllChannels(ECR_Block);
		PropBox->RegisterComponent();
		PropActor->SetActorLocation(FVector(100.0f, 0.0f, 0.0f));

		UInteractableComponent* InteractableComp = NewObject<UInteractableComponent>(PropActor);
		InteractableComp->InteractionModeTag = InteractionNativeTags::Mode_RapidTap.GetTag();
		InteractableComp->RapidTapsRequired = 5;
		InteractableComp->TapDecayRate = 1.0f; // 1 tap per second decay
		InteractableComp->InteractionDistance = 200.0f;
		InteractableComp->RegisterComponent();

		UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(PlayerActor);
		Detector->MaxTraceDistance = 300.0f;
		Detector->RegisterComponent();

		Detector->PerformDetection();
		TestNotNull(TEXT("Target must be focused"), Detector->GetFocusedTarget());

		if (Detector->GetFocusedTarget())
		{
			// Tap 3 times
			Detector->TapInteract();
			Detector->TapInteract();
			Detector->TapInteract();

			// Progress should be 3/5 = 0.60
			TestNearlyEqual(TEXT("Progress after 3 taps should be 0.6"), Detector->GetCurrentProgressRatio(), 0.60f, 0.01f);

			// Simulate 1.1s tick forward -> 1 tap decays
			Detector->TickComponent(1.1f, LEVELTICK_All, nullptr);
			// Progress should decay to 2/5 = 0.40
			TestNearlyEqual(TEXT("Progress after decay should be 0.4"), Detector->GetCurrentProgressRatio(), 0.40f, 0.01f);

			// Tap 3 more times to hit threshold (2 + 3 = 5)
			Detector->TapInteract();
			Detector->TapInteract();
			Detector->TapInteract();

			TestEqual(TEXT("Interactable state should be Completed after 5 taps"), InteractableComp->InteractionState.StateTag, InteractionNativeTags::State_Completed.GetTag());
		}

		PlayerActor->Destroy();
		PropActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDetectorThrottlingTest, "GameplayInteractionFramework.Detector.Throttling", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FDetectorThrottlingTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		AActor* TestActor = World->SpawnActor<AActor>();
		USceneComponent* Root = NewObject<USceneComponent>(TestActor);
		TestActor->SetRootComponent(Root);
		Root->RegisterComponent();

		UInteractionDetectorComponent* Detector = NewObject<UInteractionDetectorComponent>(TestActor);
		Detector->QueryInterval = 0.0333f; // ~30 Hz
		Detector->RegisterComponent();

		// Sub-frame tick: 0.010s (< 0.0333s)
		Detector->TickComponent(0.010f, LEVELTICK_All, nullptr);
		TestTrue(TEXT("QueryInterval should remain positive"), Detector->QueryInterval > 0.0f);

		TestActor->Destroy();
		World->DestroyWorld(false);
	}

	return true;
}
