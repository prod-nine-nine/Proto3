// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Type.h"
#include "UpgradeBase.generated.h"

UCLASS()
class MECHSURVIVAL_API AUpgradeBase : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleInstanceOnly, Category = Mesh)
		class UBoxComponent* Coll;

	UPROPERTY(VisibleInstanceOnly, Category = Mesh)
		class UStaticMeshComponent* Mesh;

	UPROPERTY(EditInstanceOnly, Category = Type)
		TEnumAsByte<TYPE> type = SCRAP;

	UPROPERTY(EditInstanceOnly, Category = Gather)
		float timeToGather = 2.0f;

	float gatherTimeLeft = 0;

	UMaterialInstanceDynamic* MI;

public:	
	// Sets default values for this actor's properties
	AUpgradeBase();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	
	TEnumAsByte<TYPE> mine(float deltaSeconds);

	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
