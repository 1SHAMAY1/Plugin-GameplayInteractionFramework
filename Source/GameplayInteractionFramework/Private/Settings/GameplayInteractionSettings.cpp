#include "Settings/GameplayInteractionSettings.h"
#include "Types/InteractionNativeTags.h"

UGameplayInteractionSettings::UGameplayInteractionSettings()
{
	DefaultInteractionModeTag = InteractionNativeTags::Mode_Instant;
	DefaultTraceShapeTag = InteractionNativeTags::TraceShape_CameraLine;
}

const UGameplayInteractionSettings* UGameplayInteractionSettings::Get()
{
	return GetDefault<UGameplayInteractionSettings>();
}
