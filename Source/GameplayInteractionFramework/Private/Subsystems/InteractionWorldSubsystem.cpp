#include "Subsystems/InteractionWorldSubsystem.h"
#include "Components/InteractableComponent.h"
#include "GameFramework/Actor.h"

void UInteractionWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RegisteredInteractables.Empty();
}

void UInteractionWorldSubsystem::Deinitialize()
{
	RegisteredInteractables.Empty();
	Super::Deinitialize();
}

void UInteractionWorldSubsystem::RegisterInteractable(UInteractableComponent* Component)
{
	if (!Component)
	{
		return;
	}

	RegisteredInteractables.AddUnique(Component);
}

void UInteractionWorldSubsystem::UnregisterInteractable(UInteractableComponent* Component)
{
	if (!Component)
	{
		return;
	}

	RegisteredInteractables.Remove(Component);
}

int32 UInteractionWorldSubsystem::GetInteractablesInRadius(const FVector& Location, float Radius, TArray<UInteractableComponent*>& OutInteractables) const
{
	OutInteractables.Reset();
	const float RadiusSq = FMath::Square(Radius);

	for (const TWeakObjectPtr<UInteractableComponent>& WeakComp : RegisteredInteractables)
	{
		if (UInteractableComponent* Comp = WeakComp.Get())
		{
			if (AActor* OwnerActor = Comp->GetOwner())
			{
				const float DistSq = FVector::DistSquared(Location, OwnerActor->GetActorLocation());
				if (DistSq <= RadiusSq)
				{
					OutInteractables.Add(Comp);
				}
			}
		}
	}

	return OutInteractables.Num();
}

int32 UInteractionWorldSubsystem::GetRegisteredCount() const
{
	int32 ValidCount = 0;
	for (const TWeakObjectPtr<UInteractableComponent>& WeakComp : RegisteredInteractables)
	{
		if (WeakComp.IsValid())
		{
			ValidCount++;
		}
	}
	return ValidCount;
}
