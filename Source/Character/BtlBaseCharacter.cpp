// Fill out your copyright notice in the Description page of Project Settings.


#include "Battle/Characters/BtlBaseCharacter.h"

// Sets default values
ABtlBaseCharacter::ABtlBaseCharacter()
{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void ABtlBaseCharacter::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void ABtlBaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}
