#include "GameplayInteractionFrameworkModule.h"

#if WITH_EDITOR
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Brushes/SlateImageBrush.h"
#include "Misc/Paths.h"

TSharedPtr<FSlateStyleSet> FGameplayInteractionFrameworkModule::StyleSetInstance = nullptr;

TSharedPtr<FSlateStyleSet> FGameplayInteractionFrameworkModule::GetStyleSet()
{
	return StyleSetInstance;
}
#endif

#define LOCTEXT_NAMESPACE "FGameplayInteractionFrameworkModule"

void FGameplayInteractionFrameworkModule::StartupModule()
{
#if WITH_EDITOR
	// Register custom Slate styles for component icons and thumbnails
	StyleSetInstance = MakeShared<FSlateStyleSet>("GameplayInteractionFrameworkStyle");

	FString ResourcesDir = FPaths::ProjectPluginsDir() / TEXT("GameplayInteractionFramework/Resources");
	if (!FPaths::DirectoryExists(ResourcesDir))
	{
		ResourcesDir = FPaths::EnginePluginsDir() / TEXT("Marketplace/GameplayInteractionFramework/Resources");
	}
	if (!FPaths::DirectoryExists(ResourcesDir))
	{
		ResourcesDir = FPaths::EnginePluginsDir() / TEXT("GameplayInteractionFramework/Resources");
	}

	if (FPaths::DirectoryExists(ResourcesDir))
	{
		StyleSetInstance->SetContentRoot(ResourcesDir);

		const FVector2D Icon16(16.0f, 16.0f);
		const FVector2D Icon64(64.0f, 64.0f);

		// 1. UInteractableComponent
		StyleSetInstance->Set("ClassIcon.InteractableComponent",
			new FSlateImageBrush(StyleSetInstance->RootToContentDir(TEXT("Icons/InteractableComponent_16"), TEXT(".png")), Icon16));
		StyleSetInstance->Set("ClassThumbnail.InteractableComponent",
			new FSlateImageBrush(StyleSetInstance->RootToContentDir(TEXT("Icons/InteractableComponent_64"), TEXT(".png")), Icon64));

		// 2. UInteractionDetectorComponent
		StyleSetInstance->Set("ClassIcon.InteractionDetectorComponent",
			new FSlateImageBrush(StyleSetInstance->RootToContentDir(TEXT("Icons/InteractionDetectorComponent_16"), TEXT(".png")), Icon16));
		StyleSetInstance->Set("ClassThumbnail.InteractionDetectorComponent",
			new FSlateImageBrush(StyleSetInstance->RootToContentDir(TEXT("Icons/InteractionDetectorComponent_64"), TEXT(".png")), Icon64));

		FSlateStyleRegistry::RegisterSlateStyle(*StyleSetInstance.Get());
	}
#endif
}

void FGameplayInteractionFrameworkModule::ShutdownModule()
{
#if WITH_EDITOR
	if (StyleSetInstance.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSetInstance.Get());
		StyleSetInstance.Reset();
	}
#endif
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FGameplayInteractionFrameworkModule, GameplayInteractionFramework)
