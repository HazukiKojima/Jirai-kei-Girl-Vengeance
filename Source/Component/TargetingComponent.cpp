
#include "Battle/Components/TargetingComponent.h"

#include "Battle/Characters/BtlEnemyCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

UTargetingComponent::UTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UTargetingComponent::BeginPlay()
{
    Super::BeginPlay();
}

AActor* UTargetingComponent::FindBestTarget(const TArray<AActor*>& IgnoredActors, float MaxSearchDistance, AActor* CurrentTarget, int32 ComboIndex)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return nullptr;

    ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
    if (!OwnerCharacter) return nullptr;

    // 入力方向を使い、入力がなければキャラクターの正面を基準にする
    FVector SearchDirection = OwnerCharacter->GetLastMovementInputVector();
    const bool bHasInput = !SearchDirection.IsNearlyZero();

    if (!bHasInput)
    {
        SearchDirection = OwnerCharacter->GetActorForwardVector();
    }
    SearchDirection.Normalize();

    // 検索範囲内のPawnを取得する
    TArray<FOverlapResult> OverlapResults;
    FCollisionQueryParams QueryParams;
    QueryParams.AddIgnoredActor(OwnerActor);
    for (AActor* Ignored : IgnoredActors)
    {
        QueryParams.AddIgnoredActor(Ignored);
    }

    bool bHit = GetWorld()->OverlapMultiByChannel(
        OverlapResults,
        OwnerCharacter->GetActorLocation(),
        FQuat::Identity,
        ECC_Pawn,
        FCollisionShape::MakeSphere(MaxSearchDistance),
        QueryParams
    );

    if (!bHit) return nullptr;

    AActor* BestTarget = nullptr;
    float BestScore = -FLT_MAX;
    const FVector PlayerLocation = OwnerCharacter->GetActorLocation();

    for (const FOverlapResult& Result : OverlapResults)
    {
        AActor* HitActor = Result.GetActor();
        if (!HitActor || HitActor == OwnerActor) continue;

        // 有効な敵キャラクターだけを候補にする
        if (ABtlEnemyCharacter* EnemyChar = Cast<ABtlEnemyCharacter>(HitActor))
        {
            if (EnemyChar->IsDisabled()) continue;
        }
        else
        {
            continue;
        }

        FVector ToTarget = HitActor->GetActorLocation() - PlayerLocation;
        float Distance = ToTarget.Size();

        if (Distance > MaxSearchDistance || Distance < KINDA_SMALL_NUMBER) continue;

        FVector ToTargetNormalized = ToTarget / Distance;

        // 向きと距離をスコア化して候補を比較する
        float Dot = FVector::DotProduct(SearchDirection, ToTargetNormalized);
        float DirectionScore = FMath::Clamp(Dot, 0.0f, 1.0f);
        float DistanceScore = 1.0f - (Distance / MaxSearchDistance);

        float TotalScore = (DirectionScore * DirectionWeight) + (DistanceScore * DistanceWeight);

        // コンボ中は現在の対象を優先し、照準のぶれを抑える
        if (ComboIndex > 0 && CurrentTarget && HitActor == CurrentTarget)
        {
            if (!bHasInput)
            {
                TotalScore += 10.0f;
            }
            else
            {
                TotalScore += 0.2f;
            }
        }

        if (TotalScore > BestScore)
        {
            BestScore = TotalScore;
            BestTarget = HitActor;
        }
    }

    return BestTarget;
}

AActor* UTargetingComponent::FindBestGroundTakedownTarget(float MaxSearchDistance)
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return nullptr;

    ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor);
    if (!OwnerCharacter) return nullptr;

    // 入力方向を取得する
    FVector LastInputDirection = OwnerCharacter->GetLastMovementInputVector();

    // 入力がなければキャラクターの正面を基準にする
    if (LastInputDirection.IsNearlyZero())
    {
        LastInputDirection = OwnerCharacter->GetActorForwardVector();
    }
    LastInputDirection.Normalize();

    // ダウン中の敵から候補を探す
    TArray<AActor*> EnemyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ABtlEnemyCharacter::StaticClass(), EnemyActors);

    ABtlEnemyCharacter* BestTarget = nullptr;
    float BestScore = -FLT_MAX;

    const FVector PlayerLocation = OwnerCharacter->GetActorLocation();
    
    for (AActor* Actor : EnemyActors)
    {
        ABtlEnemyCharacter* Enemy = Cast<ABtlEnemyCharacter>(Actor);
        if (!Enemy) continue;

        if (Enemy->IsDown())
        {
            FVector ToEnemy = Enemy->GetActorLocation() - PlayerLocation;
            float Distance = ToEnemy.Size();

            // 検索範囲外と距離ゼロの対象は除外する
            if (Distance > MaxSearchDistance) continue;
            if (Distance < KINDA_SMALL_NUMBER) continue;

            FVector ToEnemyNormalized = ToEnemy / Distance;

            // 入力方向と対象方向の一致度を評価する
            float Dot = FVector::DotProduct(LastInputDirection, ToEnemyNormalized);
            float DirectionScore = FMath::Clamp(Dot, 0.0f, 1.0f);

            float DistanceScore = 1.0f - (Distance / MaxSearchDistance);

            float Score = (DirectionWeight * DirectionScore) + (DistanceWeight * DistanceScore);

            if (Score > BestScore)
            {
                BestScore = Score;
                BestTarget = Enemy;
            }
        }
    }

    return BestTarget;
}

AActor* UTargetingComponent::FindBestCounterTarget(const TSet<TWeakObjectPtr<AActor>>& ActiveCounterWindows)
{
    if (ActiveCounterWindows.Num() == 0)
        return nullptr;

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor) return nullptr;

    AActor* BestTarget = nullptr;
    float BestScore = TNumericLimits<float>::Max();

    const FVector PlayerLoc = OwnerActor->GetActorLocation();

    for (const TWeakObjectPtr<AActor>& WeakActor : ActiveCounterWindows)
    {
        AActor* Actor = WeakActor.Get();
        if (!Actor) continue;

        const float Dist = FVector::DistSquared(PlayerLoc, Actor->GetActorLocation());

        if (Dist < BestScore)
        {
            BestScore = Dist;
            BestTarget = Actor;
        }
    }

    return BestTarget;
}