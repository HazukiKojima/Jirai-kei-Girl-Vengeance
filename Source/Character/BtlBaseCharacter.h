// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "BtlBaseCharacter.generated.h"

UCLASS()
class PROJECTJ_API ABtlBaseCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	ABtlBaseCharacter();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
};
