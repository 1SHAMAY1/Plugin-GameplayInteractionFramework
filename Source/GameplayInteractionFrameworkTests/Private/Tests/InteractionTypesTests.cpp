#include "Misc/AutomationTest.h"
#include "Types/GameplayInteractionTypes.h"
#include "Types/InteractionNativeTags.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionStateStructTest, "GameplayInteractionFramework.Types.StateStruct", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionStateStructTest::RunTest(const FString& Parameters)
{
	FInteractionReplicatedState State;
	TestFalse(TEXT("Default StateTag should be empty/invalid"), State.StateTag.IsValid());
	TestEqual(TEXT("Default ChangeCounter should be 0"), State.ChangeCounter, static_cast<uint8>(0));
	TestEqual(TEXT("Default ServerStartTime should be 0.0"), State.ServerStartTime, 0.0f);
	TestNull(TEXT("Default Instigator should be null"), State.Instigator.Get());

	// Set state values
	State.StateTag = InteractionNativeTags::State_Active.GetTag();
	State.ChangeCounter = 5;
	State.ServerStartTime = 100.5f;

	TestEqual(TEXT("StateTag should match State_Active"), State.StateTag, InteractionNativeTags::State_Active.GetTag());
	TestEqual(TEXT("ChangeCounter should match 5"), State.ChangeCounter, static_cast<uint8>(5));
	TestNearlyEqual(TEXT("ServerStartTime should match 100.5"), State.ServerStartTime, 100.5f, 0.001f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionCandidateSortingTest, "GameplayInteractionFramework.Types.CandidateSorting", EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FInteractionCandidateSortingTest::RunTest(const FString& Parameters)
{
	TArray<FInteractionCandidate> Candidates;

	FInteractionCandidate LowScore;
	LowScore.Score = 0.25f;
	LowScore.Distance = 200.0f;
	Candidates.Add(LowScore);

	FInteractionCandidate HighScore;
	HighScore.Score = 0.95f;
	HighScore.Distance = 50.0f;
	Candidates.Add(HighScore);

	FInteractionCandidate MidScore;
	MidScore.Score = 0.60f;
	MidScore.Distance = 100.0f;
	Candidates.Add(MidScore);

	// Sort candidates (higher scores sort first)
	Candidates.Sort();

	TestEqual(TEXT("First candidate must be the highest score"), Candidates[0].Score, 0.95f);
	TestEqual(TEXT("Second candidate must be the mid score"), Candidates[1].Score, 0.60f);
	TestEqual(TEXT("Third candidate must be the lowest score"), Candidates[2].Score, 0.25f);

	return true;
}
