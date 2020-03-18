// Fill out your copyright notice in the Description page of Project Settings.

#include "UpgradeBase.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

// Sets default values
AUpgradeBase::AUpgradeBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	//PrimaryActorTick.bCanEverTick = true;

	Coll = CreateDefaultSubobject<UBoxComponent>(TEXT("Coll"));
	//Coll->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block);
	Coll->SetCollisionProfileName(FName("PhysicsActor"));
	Coll->SetSimulatePhysics(true);
	SetRootComponent(Coll);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::Type::NoCollision);
}

// Called when the game starts or when spawned
void AUpgradeBase::BeginPlay()
{
	Super::BeginPlay();
	
}

TEnumAsByte<TYPE> AUpgradeBase::mine(float deltaSeconds)
{
	timeToGather -= deltaSeconds;

	if (timeToGather <= 0)
	{
		return type;
	}

	return NONE;
}

// Called every frame
void AUpgradeBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

