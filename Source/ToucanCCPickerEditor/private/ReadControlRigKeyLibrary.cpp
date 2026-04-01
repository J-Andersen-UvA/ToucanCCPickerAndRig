#include "ReadControlRigKeyLibrary.h"

#include "LevelSequence.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "ControlRigSequencerEditorLibrary.h"
#include "MovieSceneTimeHelpers.h"

namespace
{
	static UControlRig* findControlRigForKey(
		ULevelSequence* sequence,
		const FRigElementKey& rigKey
	)
	{
		if (!sequence)
		{
			return nullptr;
		}

		const TArray<FControlRigSequencerBindingProxy> rigBindings =
			UControlRigSequencerEditorLibrary::GetControlRigs(sequence);

		for (const FControlRigSequencerBindingProxy& binding : rigBindings)
		{
			UControlRig* controlRig = binding.ControlRig;
			if (!controlRig)
			{
				continue;
			}

			const URigHierarchy* hierarchy = controlRig->GetHierarchy();
			if (!hierarchy)
			{
				continue;
			}

			if (hierarchy->Contains(rigKey))
			{
				return controlRig;
			}
		}

		return nullptr;
	}

	static bool isRotatorModified(
		const FRotator& currentRotation,
		const FRotator& defaultRotation,
		const float tolerance
	)
	{
		return
			!FMath::IsNearlyEqual(currentRotation.Roll, defaultRotation.Roll, tolerance) ||
			!FMath::IsNearlyEqual(currentRotation.Pitch, defaultRotation.Pitch, tolerance) ||
			!FMath::IsNearlyEqual(currentRotation.Yaw, defaultRotation.Yaw, tolerance);
	}
}

bool UReadControlRigKeyLibrary::getControlRotationInSequenceAtFrame(
	const FRigElementKey& rigKey,
	ULevelSequence* sequence,
	int32 frameNumber,
	FRotator& outRotation
)
{
	outRotation = FRotator::ZeroRotator;

	if (!sequence)
	{
		return false;
	}

	UControlRig* controlRig = findControlRigForKey(sequence, rigKey);
	if (!controlRig)
	{
		return false;
	}

	const FFrameNumber discreteFrame = FFrameNumber(frameNumber);

	outRotation = UControlRigSequencerEditorLibrary::GetLocalControlRigRotator(
		sequence,
		controlRig,
		rigKey.Name,
		discreteFrame,
		EMovieSceneTimeUnit::DisplayRate
	);

	return true;
}

bool UReadControlRigKeyLibrary::isControlModifiedInSequenceAtFrame(
	const FRigElementKey& rigKey,
	ULevelSequence* sequence,
	int32 frameNumber,
	FRotator defaultRotation,
	float tolerance
)
{
	FRotator currentRotation;
	const bool gotRotation = getControlRotationInSequenceAtFrame(
		rigKey,
		sequence,
		frameNumber,
		currentRotation
	);

	if (!gotRotation)
	{
		return false;
	}

	return isRotatorModified(currentRotation, defaultRotation, tolerance);
}

void UReadControlRigKeyLibrary::getModifiedControlsInSequenceAtFrame(
	const TArray<FRigElementKey>& rigKeys,
	ULevelSequence* sequence,
	int32 frameNumber,
	float tolerance,
	TArray<FRigElementKey>& outModifiedKeys
)
{
	outModifiedKeys.Reset();

	if (!sequence)
	{
		return;
	}

	for (const FRigElementKey& rigKey : rigKeys)
	{
		FRotator currentRotation;
		if (!getControlRotationInSequenceAtFrame(rigKey, sequence, frameNumber, currentRotation))
		{
			continue;
		}

		if (isRotatorModified(currentRotation, FRotator::ZeroRotator, tolerance))
		{
			outModifiedKeys.Add(rigKey);
		}
	}
}