#include "ToucanPickerViewportLibrary.h"

#include "Editor.h"
#include "LevelEditorViewport.h"
#include "EditorViewportClient.h"

namespace
{
    FVector getAlignedAxis(const FTransform& controlTransform, EToucanControlAxis alignAxis)
    {
        switch (alignAxis)
        {
            case EToucanControlAxis::PositiveX:
                return controlTransform.GetUnitAxis(EAxis::X);

            case EToucanControlAxis::NegativeX:
                return -controlTransform.GetUnitAxis(EAxis::X);

            case EToucanControlAxis::PositiveY:
                return controlTransform.GetUnitAxis(EAxis::Y);

            case EToucanControlAxis::NegativeY:
                return -controlTransform.GetUnitAxis(EAxis::Y);

            case EToucanControlAxis::PositiveZ:
                return controlTransform.GetUnitAxis(EAxis::Z);

            case EToucanControlAxis::NegativeZ:
                return -controlTransform.GetUnitAxis(EAxis::Z);

            default:
                return controlTransform.GetUnitAxis(EAxis::X);
        }
    }
}

void UToucanPickerViewportLibrary::focusViewportOnControl(
    const FTransform& controlTransform,
    EToucanControlAxis alignAxis,
    float cameraDistance
)
{
#if WITH_EDITOR
    if (!GEditor)
    {
        return;
    }

    const FVector controlLocation = controlTransform.GetLocation();
    const FVector cameraForward = getAlignedAxis(controlTransform, alignAxis).GetSafeNormal();

    if (cameraForward.IsNearlyZero())
    {
        return;
    }

    const FVector worldUp = FVector::UpVector;
    FVector right = FVector::CrossProduct(worldUp, cameraForward).GetSafeNormal();

    if (right.IsNearlyZero())
    {
        right = FVector::CrossProduct(FVector::ForwardVector, cameraForward).GetSafeNormal();
    }

    const FVector cameraUp = FVector::CrossProduct(cameraForward, right).GetSafeNormal();
    const FVector cameraLocation = controlLocation - (cameraForward * cameraDistance);
    const FRotator cameraRotation = FRotationMatrix::MakeFromXZ(cameraForward, cameraUp).Rotator();

    for (FLevelEditorViewportClient* viewportClient : GEditor->GetLevelViewportClients())
    {
        if (!viewportClient || !viewportClient->IsPerspective())
        {
            continue;
        }

        viewportClient->SetViewLocation(cameraLocation);
        viewportClient->SetViewRotation(cameraRotation);
        viewportClient->Invalidate();
    }
#endif
}