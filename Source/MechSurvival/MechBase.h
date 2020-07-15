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
		UStaticMeshComponent* Mesh3P;

	class AMechSurvivalCharacter* pilot = 0;

	UMaterialInstanceDynamic* MI;

	float mechScale = 2.5f;

	//bool chargingJump = false;
	float jumpChargeTime = 0;
	UPROPERTY(EditAnywhere, Category = "gameplay | jump")
	float jumpStrength = 1000;
	//float jumpMin = 0;
	UPROPERTY(EditAnywhere, Category = "gameplay | jump")
	float maxJumpChargeTime = 0.5f;
	//float jumpDiff = 1000;
	float basePlayerMovement = 0;
	bool moveChangeOnce = false;

	bool boost = false;
	float boostTimer = 0;
	UPROPERTY(EditAnywhere, Category = "gameplay | boost")
	float maxBoostTime = 3.0f;
	UPROPERTY(EditAnywhere, Category = "gameplay | boost")
	float boostAmount = 10000;

	UPROPERTY(EditAnywhere, Category = "gameplay | durability")
	float maxDurability = 100;

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "gameplay | durability")
	float currentDurability = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | attachments")
	bool jumpEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | attachments")
	bool gunEnabled = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | attachments")
	bool boostEnabled = false;

	bool mechEnabled = true;
	bool jumping = false;
	bool canBoostJump = false;
public:
	// Sets default values for this character's properties
	AMechBase();

	void setPilot(AMechSurvivalCharacter* newPilot) { pilot = newPilot; }

	UFUNCTION(BlueprintCallable)
		void damageMech(float damage);

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

	//void chargeJump();

	void Jump() override;
	void StopJumping() override;

	void BoostOn();
	void BoostOff();

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

	/** Projectile class to spawn */
	UPROPERTY(EditDefaultsOnly, Category = Projectile)
		TSubclassOf<class AMechSurvivalProjectile> ProjectileClass;

	/** Sounds to play */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* FireSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* MechBoost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* MechHit;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* MechDanger;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* MechJump;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* MechJumpBoost;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "gameplay | sound")
		class USoundBase* MechLeave;

	class UAudioComponent* ActiveMechDanger = 0;

	class UAudioComponent* ActiveMechBoost = 0;

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

};
