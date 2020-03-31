// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Type.h"
#include "MechSurvivalCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class AMechSurvivalCharacter : public ACharacter
{
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	UPROPERTY(VisibleDefaultsOnly, Category = Laser)
	class UParticleSystemComponent* LaserParticle1;

	UPROPERTY(VisibleDefaultsOnly, Category = Laser)
	class UParticleSystemComponent* LaserParticle2;

	UPROPERTY(VisibleDefaultsOnly, Category = Laser)
	class UParticleSystemComponent* LaserParticle3;

	UPROPERTY(VisibleDefaultsOnly, Category = Laser)
	class UParticleSystemComponent* LaserParticle4;

	UPROPERTY(VisibleDefaultsOnly, Category = Laser)
	class UParticleSystemComponent* LaserSparks;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	float health = 100;

	float baseMovement = 0;
	float sprintMultiplier = 1.5f;

public:
	AMechSurvivalCharacter();

	UFUNCTION(BlueprintCallable)
		void damagePlayer(float damage);

protected:
	virtual void BeginPlay();

public:
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category=Projectile)
	TSubclassOf<class AMechSurvivalProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	float range = 200.0f;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	float minerRange = 500.0f;

	UPROPERTY(VisibleAnywhere, Category = Gameplay)
	int scrapAmount = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Gameplay)
	TEnumAsByte<TYPE> UpgradeType = NONE;

	UPROPERTY(BlueprintReadOnly, Category = Anim)
	bool armed = false;

	UPROPERTY(BlueprintReadOnly, Category = Anim)
	bool firing = false;

	UPROPERTY(BlueprintReadWrite, Category = Anim)
	bool canShoot = false;

	UPROPERTY(EditAnywhere, Category = Gameplay)
	FName restartLevel = "";

protected:
	
	void OnFire();
	void OnFireStop();

	void OnInteract();

	void SwitchEquip();

	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	/**
	 * Called via input to turn at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate.
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);
	
protected:
	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

public:
	virtual void Tick(float DeltaTime) override;

	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }
	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

