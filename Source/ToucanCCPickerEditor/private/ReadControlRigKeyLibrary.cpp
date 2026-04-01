#include "ReadControlRigKeyLibrary.h"

#include "LevelSequence.h"
#include "ControlRig.h"
#include "Rigs/RigHierarchy.h"
#include "ControlRigSequencerEditorLibrary.h"
#include "MovieSceneTimeHelpers.h"

namespace
{
	static void clearCacheData(FControlRigKeyCache& cache)
	{
		cache.sequence = nullptr;
		cache.sourceKeys.Reset();
		cache.matchedRigCount = 0;
		cache.matchedKeyCount = 0;
		cache.keysByRig.Reset();
	}

	static void buildRigKeyMap(
		ULevelSequence* sequence,
		const TArray<FRigElementKey>& rigKeys,
		TMap<TObjectPtr<UControlRig>, TArray<FRigElementKey>>& outKeysByRig
	)
	{
		outKeysByRig.Reset();

		if (!sequence || rigKeys.IsEmpty())
		{
			return;
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

			TArray<FRigElementKey> matchingKeys;
			matchingKeys.Reserve(rigKeys.Num());

			for (const FRigElementKey& rigKey : rigKeys)
			{
				if (hierarchy->Contains(rigKey))
				{
					matchingKeys.Add(rigKey);
				}
			}

			if (!matchingKeys.IsEmpty())
			{
				outKeysByRig.Add(controlRig, MoveTemp(matchingKeys));
			}
		}
	}

	static bool isRotatorModified(
		const FRotator& currentRotation,
		const FRotator& defaultRotation,
		float tolerance
	)
	{
		return
			!FMath::IsNearlyEqual(currentRotation.Roll, defaultRotation.Roll, tolerance) ||
			!FMath::IsNearlyEqual(currentRotation.Pitch, defaultRotation.Pitch, tolerance) ||
			!FMath::IsNearlyEqual(currentRotation.Yaw, defaultRotation.Yaw, tolerance);
	}

	static bool findRigInCache(
		const FControlRigKeyCache& cache,
		const FRigElementKey& rigKey,
		UControlRig*& outControlRig
	)
	{
		outControlRig = nullptr;

		if (!cache.sequence)
		{
			return false;
		}

		for (const TPair<TObjectPtr<UControlRig>, TArray<FRigElementKey>>& pair : cache.keysByRig)
		{
			UControlRig* controlRig = pair.Key;
			if (!controlRig)
			{
				continue;
			}

			if (pair.Value.Contains(rigKey))
			{
				outControlRig = controlRig;
				return true;
			}
		}

		return false;
	}

	static void fillCacheStats(FControlRigKeyCache& cache)
	{
		cache.matchedRigCount = cache.keysByRig.Num();
		cache.matchedKeyCount = 0;

		for (const TPair<TObjectPtr<UControlRig>, TArray<FRigElementKey>>& pair : cache.keysByRig)
		{
			cache.matchedKeyCount += pair.Value.Num();
		}
	}
}

bool UReadControlRigKeyLibrary::buildControlRigKeyCache(
	ULevelSequence* sequence,
	const TArray<FRigElementKey>& rigKeys,
	FControlRigKeyCache& outCache
)
{
	clearCacheData(outCache);

	if (!sequence || rigKeys.IsEmpty())
	{
		return false;
	}

	outCache.sequence = sequence;
	outCache.sourceKeys = rigKeys;
	buildRigKeyMap(sequence, rigKeys, outCache.keysByRig);
	fillCacheStats(outCache);

	return outCache.matchedRigCount > 0;
}

bool UReadControlRigKeyLibrary::updateControlRigKeyCache(
	FControlRigKeyCache& cache,
	ULevelSequence* sequence,
	const TArray<FRigElementKey>& rigKeys
)
{
	return buildControlRigKeyCache(sequence, rigKeys, cache);
}

void UReadControlRigKeyLibrary::clearControlRigKeyCache(FControlRigKeyCache& cache)
{
	clearCacheData(cache);
}

bool UReadControlRigKeyLibrary::isControlRigKeyCacheUsable(const FControlRigKeyCache& cache)
{
	return cache.sequence != nullptr && cache.keysByRig.Num() > 0;
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

	const TArray<FControlRigSequencerBindingProxy> rigBindings =
		UControlRigSequencerEditorLibrary::GetControlRigs(sequence);

	UControlRig* matchedRig = nullptr;

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
			matchedRig = controlRig;
			break;
		}
	}

	if (!matchedRig)
	{
		return false;
	}

	outRotation = UControlRigSequencerEditorLibrary::GetLocalControlRigRotator(
		sequence,
		matchedRig,
		rigKey.Name,
		FFrameNumber(frameNumber),
		EMovieSceneTimeUnit::DisplayRate
	);

	return true;
}

bool UReadControlRigKeyLibrary::getControlRotationFromCacheAtFrame(
	const FRigElementKey& rigKey,
	const FControlRigKeyCache& cache,
	int32 frameNumber,
	FRotator& outRotation
)
{
	outRotation = FRotator::ZeroRotator;

	UControlRig* matchedRig = nullptr;
	if (!findRigInCache(cache, rigKey, matchedRig) || !matchedRig || !cache.sequence)
	{
		return false;
	}

	outRotation = UControlRigSequencerEditorLibrary::GetLocalControlRigRotator(
		cache.sequence,
		matchedRig,
		rigKey.Name,
		FFrameNumber(frameNumber),
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

bool UReadControlRigKeyLibrary::isControlModifiedFromCacheAtFrame(
	const FRigElementKey& rigKey,
	const FControlRigKeyCache& cache,
	int32 frameNumber,
	FRotator defaultRotation,
	float tolerance
)
{
	FRotator currentRotation;
	const bool gotRotation = getControlRotationFromCacheAtFrame(
		rigKey,
		cache,
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

	if (!sequence || rigKeys.IsEmpty())
	{
		return;
	}

	TMap<TObjectPtr<UControlRig>, TArray<FRigElementKey>> keysByRig;
	buildRigKeyMap(sequence, rigKeys, keysByRig);

	const FFrameNumber discreteFrame(frameNumber);

	for (const TPair<TObjectPtr<UControlRig>, TArray<FRigElementKey>>& pair : keysByRig)
	{
		UControlRig* controlRig = pair.Key;
		if (!controlRig)
		{
			continue;
		}

		for (const FRigElementKey& rigKey : pair.Value)
		{
			const FRotator currentRotation =
				UControlRigSequencerEditorLibrary::GetLocalControlRigRotator(
					sequence,
					controlRig,
					rigKey.Name,
					discreteFrame,
					EMovieSceneTimeUnit::DisplayRate
				);

			if (isRotatorModified(currentRotation, FRotator::ZeroRotator, tolerance))
			{
				outModifiedKeys.Add(rigKey);
			}
		}
	}
}

void UReadControlRigKeyLibrary::getModifiedControlsFromCacheAtFrame(
	const FControlRigKeyCache& cache,
	int32 frameNumber,
	float tolerance,
	TArray<FRigElementKey>& outModifiedKeys
)
{
	outModifiedKeys.Reset();

	if (!cache.sequence || cache.keysByRig.IsEmpty())
	{
		return;
	}

	const FFrameNumber discreteFrame(frameNumber);

	for (const TPair<TObjectPtr<UControlRig>, TArray<FRigElementKey>>& pair : cache.keysByRig)
	{
		UControlRig* controlRig = pair.Key;
		if (!controlRig)
		{
			continue;
		}

		for (const FRigElementKey& rigKey : pair.Value)
		{
			const FRotator currentRotation =
				UControlRigSequencerEditorLibrary::GetLocalControlRigRotator(
					cache.sequence,
					controlRig,
					rigKey.Name,
					discreteFrame,
					EMovieSceneTimeUnit::DisplayRate
				);

			if (isRotatorModified(currentRotation, FRotator::ZeroRotator, tolerance))
			{
				outModifiedKeys.Add(rigKey);
			}
		}
	}
}