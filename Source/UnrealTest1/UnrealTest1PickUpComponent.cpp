// Copyright Epic Games, Inc. All Rights Reserved.

#include "UnrealTest1PickUpComponent.h"

UUnrealTest1PickUpComponent::UUnrealTest1PickUpComponent()
{
	// Setup the Sphere Collision
	SphereRadius = 32.f;
}

void UUnrealTest1PickUpComponent::BeginPlay()
{
	Super::BeginPlay();

	// Register our Overlap Event
	OnComponentBeginOverlap.AddDynamic(this, &UUnrealTest1PickUpComponent::OnSphereBeginOverlap);
}

void UUnrealTest1PickUpComponent::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Checking if it is a First Person Character overlapping
	AUnrealTest1Character* Character = Cast<AUnrealTest1Character>(OtherActor);
	if(Character != nullptr)
	{
		// Notify that the actor is being picked up
		OnPickUp.Broadcast(Character);

		// Unregister from the Overlap Event so it is no longer triggered
		OnComponentBeginOverlap.RemoveAll(this);
	}
}
