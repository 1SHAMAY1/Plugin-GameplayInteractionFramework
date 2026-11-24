#include "Misc/AutomationTest.h"
#include "Subsystems/InteractionWorldSubsystem.h"
#include "Subsystems/InteractionLocalPlayerSubsystem.h"
#include "FunctionLibraries/InteractionFunctionLibrary.h"
#include "Components/InteractableComponent.h"
#include "Types/InteractionNativeTags.h"
#include "GameFramework/Actor.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSubsystemRegistryAndBroadphaseTest, "GameplayInteractionFramework.Subsystems.WorldRegistryAndBroadphase", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FSubsystemRegistryAndBroadphaseTest::RunTest(const FString& Parameters)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	TestNotNull(TEXT("Test world must be created"), World);

	if (World)
	{
		UInteractionWorldSubsystem* Subsystem = World->GetSubsystem<UInteractionWorldSubsystem>();
		TestNotNull(TEXT("UInteractionWorldSubsystem must exist"), Subsystem);

		if (Subsystem)
		{
			TArray<AActor*> SpawnedActors;

			// Spawn 5 actors at varying distances: 0cm, 100cm, 250cm, 600cm, 1200cm
			const TArray<float> Distances = { 0.0f, 100.0f, 250.0f, 600.0f, 1200.0f };
			for (int32 i = 0; i < Distances.Num(); ++i)
			{
				AActor* PropActor = World->SpawnActor<AActor>();
				USceneComponent* Root = NewObject<USceneComponent>(PropActor);
				PropActor->SetRootComponent(Root);
				Root->RegisterComponent();
				PropActor->SetActorLocation(FVector(Distances[i], 0.0f, 0.0f));

				UInteractableComponent* Comp = NewObject<UInteractableComponent>(PropActor);
				Comp->RegisterComponent();
				Subsystem->RegisterInteractable(Comp);

				SpawnedActors.Add(PropActor);
			}

			// Verify all 5 are registered
			TestEqual(TEXT("Subsystem must have 5 registered interactables"), Subsystem->GetRegisteredCount(), 5);

			// Broad-phase query with radius 300cm centered at (0,0,0)
			// Should capture only actors at 0cm, 100cm, and 250cm (3 items)
			TArray<UInteractableComponent*> InRangeComps;
			const int32 Count = Subsystem->GetInteractablesInRadius(FVector::ZeroVector, 300.0f, InRangeComps);

			TestEqual(TEXT("Query should find exactly 3 interactables within 300cm"), Count, 3);
			TestEqual(TEXT("Array size matches returned count"), InRangeComps.Num(), 3);

			// Unregister one manually and verify count updates
			Subsystem->UnregisterInteractable(SpawnedActors[0]->FindComponentByClass<UInteractableComponent>());
			TestEqual(TEXT("Count should be 4 after unregistering"), Subsystem->GetRegisteredCount(), 4);

			for (AActor* Actor : SpawnedActors)
			{
				Actor->Destroy();
			}
		}

		World->DestroyWorld(false);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLocalPlayerSubsystemEventBusTest, "GameplayInteractionFramework.Subsystems.LocalPlayerEventBus", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FLocalPlayerSubsystemEventBusTest::RunTest(const FString& Parameters)
{
	ULocalPlayer* LocalPlayer = NewObject<ULocalPlayer>(GEngine);
	UInteractionLocalPlayerSubsystem* Subsystem = NewObject<UInteractionLocalPlayerSubsystem>(LocalPlayer);
	TestNotNull(TEXT("Local player subsystem must be instantiated"), Subsystem);

	if (Subsystem)
	{
		// Initial state
		TestNull(TEXT("Initial focused target should be null"), Subsystem->GetCurrentFocusedTarget());
		TestNearlyEqual(TEXT("Initial progress ratio should be 0.0"), Subsystem->GetCurrentProgressRatio(), 0.0f);

		// Test Focus Changed
		FInteractionPromptData Prompt;
		Prompt.ActionVerb = FText::FromString(TEXT("Hack"));
		Prompt.DisplayTitle = FText::FromString(TEXT("Control Terminal"));

		UObject* DummyTarget = NewObject<USceneComponent>();
		Subsystem->NotifyFocusedTargetChanged(DummyTarget, Prompt);

		TestEqual(TEXT("Focused target updated"), Subsystem->GetCurrentFocusedTarget(), DummyTarget);
		TestEqual(TEXT("Prompt ActionVerb matches"), Subsystem->GetCurrentPromptData().ActionVerb.ToString(), TEXT("Hack"));
		TestEqual(TEXT("Prompt DisplayTitle matches"), Subsystem->GetCurrentPromptData().DisplayTitle.ToString(), TEXT("Control Terminal"));

		// Test Progress Updated
		Subsystem->NotifyInteractionProgress(0.75f, InteractionNativeTags::Mode_Hold.GetTag(), DummyTarget);
		TestNearlyEqual(TEXT("Progress ratio updated to 0.75"), Subsystem->GetCurrentProgressRatio(), 0.75f, 0.01f);

		// Test Cancellation
		Subsystem->NotifyInteractionCanceled(InteractionNativeTags::CancelReason_KeyReleased.GetTag(), DummyTarget);
		TestNearlyEqual(TEXT("Progress reset on cancellation"), Subsystem->GetCurrentProgressRatio(), 0.0f, 0.01f);

		// Test Completion
		Subsystem->NotifyInteractionCompleted(DummyTarget);
		TestNearlyEqual(TEXT("Progress set to 1.0 on completion"), Subsystem->GetCurrentProgressRatio(), 1.0f, 0.01f);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFunctionLibraryMathTest, "GameplayInteractionFramework.FunctionLibrary.MathAndUtilities", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FFunctionLibraryMathTest::RunTest(const FString& Parameters)
{
	// 1. Scoring equation math test
	// Dot = 1.0 (directly facing -> NormalizedDot = 1.0), Dist = 0.0 (NormalizedDist = 1.0), Priority = 0
	// Score = (1.0 * 0.5) + (1.0 * 0.3) + 0.0 = 0.80
	const float ScoreA = UInteractionFunctionLibrary::CalculateDirectionalScore(0.0f, 500.0f, 1.0f, 0, 0.5f, 0.3f, 0.2f);
	TestNearlyEqual(TEXT("Perfect alignment score should be 0.80"), ScoreA, 0.80f, 0.01f);

	// Max distance, Dot = -1.0 (looking opposite -> NormalizedDot = 0.0), Dist = MaxDist (NormalizedDist = 0.0)
	// Score = 0.0
	const float ScoreB = UInteractionFunctionLibrary::CalculateDirectionalScore(500.0f, 500.0f, -1.0f, 0, 0.5f, 0.3f, 0.2f);
	TestNearlyEqual(TEXT("Opposite alignment score should be 0.0"), ScoreB, 0.0f, 0.01f);

	// 2. Closest point search
	TArray<FVector> Points = {
		FVector(100.0f, 0.0f, 0.0f),
		FVector(500.0f, 0.0f, 0.0f),
		FVector(25.0f, 0.0f, 0.0f),
		FVector(1000.0f, 0.0f, 0.0f)
	};

	const int32 ClosestIdx = UInteractionFunctionLibrary::FindClosestPointIndex(Points, FVector::ZeroVector);
	TestEqual(TEXT("Closest point index to origin should be Index 2 (25cm)"), ClosestIdx, 2);

	// Empty array boundary condition
	TArray<FVector> EmptyPoints;
	TestEqual(TEXT("Empty array returns INDEX_NONE"), UInteractionFunctionLibrary::FindClosestPointIndex(EmptyPoints, FVector::ZeroVector), INDEX_NONE);

	// 3. Validation descriptions
	const FText ValidDesc = UInteractionFunctionLibrary::GetValidationResultDescription(EInteractionValidationResult::Valid);
	TestFalse(TEXT("Valid description should not be empty"), ValidDesc.IsEmpty());

	const FText LoSDesc = UInteractionFunctionLibrary::GetValidationResultDescription(EInteractionValidationResult::ObstructedLineOfSight);
	TestFalse(TEXT("LoS description should not be empty"), LoSDesc.IsEmpty());

	const FText BusyDesc = UInteractionFunctionLibrary::GetValidationResultDescription(EInteractionValidationResult::TargetBusy);
	TestFalse(TEXT("TargetBusy description should not be empty"), BusyDesc.IsEmpty());

	return true;
}
