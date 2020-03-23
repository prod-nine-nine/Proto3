// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MechBase.generated.h"

UCLASS()
class MECHSURVIVAL_API AMechBase : public ACharacter
{
private:
	GENERATED_BODY()

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		class USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
		class UCameraComponent* FirstPersonCameraComponent;

	/** Pawn mesh: outside view only seen when not them */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
		USkeletalMeshComponent* Mesh3P;

	class AMechSurvivalCharacter* pilot = 0;

	UMaterialInstanceDynamic* MI;

	float mechScale = 2.5f;

	bool chargingJump = false;
	float jumpChargeTime = 0;
	UPROPERTY(EditDefaultsOnly, Category = jump)
	float maxJumpChargeTime = 0.5f;
	float jumpMin = 0;
	UPROPERTY(EditDefaultsOnly, Category = jump)
	float jumpDiff = 1000;
	float basePlayerMovement = 0;
	bool moveChangeOnce = false;

	bool boost = false;
	float boostTimer = 0;
	UPROPERTY(EditDefaultsOnly, Category = boost)
	float maxBoostTime = 3.0f;
	UPROPERTY(EditDefaultsOnly, Category = boost)
	float boostAmount = 100;

	UPROPERTY(EditAnywhere, Category = durability)
	float maxDurability = 100;
	UPROPERTY(EditAnywhere, Category = durability)
	float currentDurability = 0;

public:

	UPROPERTY(EditAnywhere, Category = gameplay)
	bool jumpEnabled = false;
	UPROPERTY(EditAnywhere, Category = gameplay)
	bool gunEnabled = false;
	UPROPERTY(EditAnywhere, Category = gameplay)
	bool boostEnabled = false;

	bool mechEnabled = true;
	bool jumping = false;
	bool canBoostJump = false;
public:
	// Sets default values for this character's properties
	AMechBase();

	void setPilot(AMechSurvivalCharacter* newPilot) { pilot = newPilot; }

	UFUNCTION(BlueprintCallable)
		void damageMech(float damage) { currentDurability = (currentDurability - damage <= 0) ? 0 : currentDurability - damage; }

	bool healMech(float healing) {
		if (currentDurability == maxDurability) { return false; }
		currentDurability = (currentDurability + healing >= maxDurability) ? maxDurability : currentDurability + healing;
		return true;
	}

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	/** Fires a projectile. */
	void OnFire();

	void OnInteract();

	void chargeJump();

	void Jump() override;
	void StopJumping() override;

	void BoostOn() { if (!boostEnabled) { return; } boost = true; boostTimer = 0; }
	void BoostOff() { boost = false; }

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

public:	

	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera)
		float BaseLookUpRate;

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		FVector GunOffset;

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AMechSurvivalProjectile> ProjectileClass;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
		class UAnimMontage* FireAnimation;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
