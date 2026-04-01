#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ReadControlRigKeyLibrary.generated.h"

UCLASS()
class TOUCANCCPICKEREDITOR_API UReadControlRigKeyLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category="Toucan|Sequencer")
    static bool getControlRotationInSequenceAtFrame(
        const FRigElementKey& rigKey,
        ULevelSequence* sequence,
        int32 frameNumber,
        FRotator& outRotation
    );

    UFUNCTION(BlueprintCallable, Category="Toucan|Sequencer")
    static bool isControlModifiedInSequenceAtFrame(
        const FRigElementKey& rigKey,
        ULevelSequence* sequence,
        int32 frameNumber,
        FRotator defaultRotation = FRotator::ZeroRotator,
        float tolerance = 0.1f
    );

    UFUNCTION(BlueprintCallable, Category="Toucan|Sequencer")
    static void getModifiedControlsInSequenceAtFrame(
        const TArray<FRigElementKey>& rigKeys,
        ULevelSequence* sequence,
        int32 frameNumber,
        float tolerance,
        TArray<FRigElementKey>& outModifiedKeys
    );
};