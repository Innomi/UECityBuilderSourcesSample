// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * TUEConcurrentSpatialGridIndex
 */
template <typename DataType>
class TUEConcurrentSpatialGridIndex
{
public:
	using FIndexEntry = TPair<FIntRect, DataType>;

	explicit TUEConcurrentSpatialGridIndex(FIntPoint const InSize, FIntPoint const InIndexCellSize, FIntPoint const InLockCellSize);

	FIntPoint GetSize() const;
	FIntPoint GetIndexCellSize() const;
	FIntPoint GetLockCellSize() const;
	FIntPoint GetIndexCellsNum() const;
	FIntPoint GetLockCellsNum() const;

	bool TryInsert(FIntRect const & Rect, DataType const & Data);
	bool TryInsert(FIntRect const & Rect, DataType && Data);
	void InsertUnchecked(FIntRect const & Rect, DataType const & Data);
	void InsertUnchecked(FIntRect const & Rect, DataType && Data);
	bool CheckIfFree(FIntRect const & Rect) const;
	void Erase(FIntRect const & Rect);

	template <typename PredicateType>
	void EraseByPredicate(FIntRect const & Rect, PredicateType Predicate);

	TMap<FIntRect, DataType> GetOverlapping(FIntRect const & Rect) const;

private:
	enum class ERWLockType : uint8
	{
		ReadOnly,
		Write
	};

	class FRWRectScopeLock
	{
	public:
		UE_NODISCARD_CTOR explicit FRWRectScopeLock(TUEConcurrentSpatialGridIndex<DataType> const & InSpatialGridIndex, FIntRect const & InRect, ERWLockType const InLockType);
		~FRWRectScopeLock();
		UE_NONCOPYABLE(FRWRectScopeLock)

	private:
		TUEConcurrentSpatialGridIndex const & SpatialGridIndex;
		FIntRect const & Rect;
		ERWLockType const LockType;
	};

	friend FRWRectScopeLock;

	using FCellInfo = TArray<FIndexEntry>;

	void CheckRange(FIntPoint const Coords) const;
	void CheckRange(FIntRect const & Rect) const;
	template <typename ArgType>
	bool TryInsertImpl(FIntRect const & Rect, ArgType && Data);
	template <typename ArgType>
	void InsertUncheckedImpl(FIntRect const & Rect, ArgType && Data);
	void GetLocks(FIntRect const & Rect, ERWLockType const LockType) const;
	void FreeLocks(FIntRect const & Rect, ERWLockType const LockType) const;
	template <typename ArgType>
	void InsertUncheckedNoLock(FIntRect const & Rect, ArgType && Data);
	bool CheckIfFreeNoLock(FIntRect const & Rect) const;

	TArray<FCellInfo> SpatialGridData;
	mutable TArray<FRWLock> Locks;
	FIntPoint const Size;
	FIntPoint const IndexCellSize;
	FIntPoint const IndexCellsNum;
	FIntPoint const LockCellSize;
	FIntPoint const LockCellsNum;
};

template <typename DataType>
TUEConcurrentSpatialGridIndex<DataType>::TUEConcurrentSpatialGridIndex(FIntPoint const InSize, FIntPoint const InIndexCellSize, FIntPoint const InLockCellSize)
	: Size(InSize)
	, IndexCellSize(InIndexCellSize)
	, IndexCellsNum(InIndexCellSize.GetMin() > 0 ? InSize / InIndexCellSize : FIntPoint::ZeroValue)
	, LockCellSize(InLockCellSize)
	, LockCellsNum(InLockCellSize.GetMin() > 0 ? InSize / InLockCellSize : FIntPoint::ZeroValue)
{
	check(InSize.X >= 0 && InSize.Y >= 0);

	check(InIndexCellSize.X > 0 && InIndexCellSize.Y > 0);
	check(InSize.X % InIndexCellSize.X == 0 && InSize.Y % InIndexCellSize.Y == 0);

	check(0 < InLockCellSize.X && 0 < InLockCellSize.Y);
	check(InSize.X % InLockCellSize.X == 0 && InSize.Y % InLockCellSize.Y == 0);
	check(InLockCellSize.X % InIndexCellSize.X == 0 && InLockCellSize.Y % InIndexCellSize.Y == 0);

	SpatialGridData.SetNum(IndexCellsNum.X * IndexCellsNum.Y);
	Locks.SetNum(LockCellsNum.X * LockCellsNum.Y);
}

template <typename DataType>
FIntPoint TUEConcurrentSpatialGridIndex<DataType>::GetSize() const
{
	return Size;
}

template <typename DataType>
FIntPoint TUEConcurrentSpatialGridIndex<DataType>::GetIndexCellSize() const
{
	return IndexCellSize;
}

template <typename DataType>
FIntPoint TUEConcurrentSpatialGridIndex<DataType>::GetLockCellSize() const
{
	return LockCellSize;
}

template <typename DataType>
FIntPoint TUEConcurrentSpatialGridIndex<DataType>::GetIndexCellsNum() const
{
	return IndexCellsNum;
}

template <typename DataType>
FIntPoint TUEConcurrentSpatialGridIndex<DataType>::GetLockCellsNum() const
{
	return LockCellsNum;
}

template <typename DataType>
bool TUEConcurrentSpatialGridIndex<DataType>::TryInsert(FIntRect const & Rect, DataType const & Data)
{
	return TryInsertImpl(Rect, Data);
}

template <typename DataType>
bool TUEConcurrentSpatialGridIndex<DataType>::TryInsert(FIntRect const & Rect, DataType && Data)
{
	return TryInsertImpl(Rect, MoveTempIfPossible(Data));
}

template <typename DataType>
template <typename ArgType>
bool TUEConcurrentSpatialGridIndex<DataType>::TryInsertImpl(FIntRect const & Rect, ArgType && Data)
{
	FRWRectScopeLock WRectScopeLock(*this, Rect, ERWLockType::Write);
	if (!CheckIfFreeNoLock(Rect))
	{
		return false;
	}
	InsertUncheckedNoLock(Rect, Forward<ArgType>(Data));
	return true;
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::InsertUnchecked(FIntRect const & Rect, DataType const & Data)
{
	InsertUncheckedImpl(Rect, Data);
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::InsertUnchecked(FIntRect const & Rect, DataType && Data)
{
	InsertUncheckedImpl(Rect, MoveTempIfPossible(Data));
}

template <typename DataType>
template <typename ArgType>
void TUEConcurrentSpatialGridIndex<DataType>::InsertUncheckedImpl(FIntRect const & Rect, ArgType && Data)
{
	FRWRectScopeLock WRectScopeLock(*this, Rect, ERWLockType::Write);
	InsertUncheckedNoLock(Rect, Forward<ArgType>(Data));
}

template <typename DataType>
bool TUEConcurrentSpatialGridIndex<DataType>::CheckIfFree(FIntRect const & Rect) const
{
	FRWRectScopeLock RRectScopeLock(*this, Rect, ERWLockType::ReadOnly);
	return CheckIfFreeNoLock(Rect);
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::Erase(FIntRect const & Rect)
{
	EraseByPredicate(Rect, [](FIndexEntry const & IndexEntry) -> bool { return true; });
}

template <typename DataType>
template <typename PredicateType>
void TUEConcurrentSpatialGridIndex<DataType>::EraseByPredicate(FIntRect const & Rect, PredicateType Predicate)
{
	CheckRange(Rect);
	FRWRectScopeLock WRectScopeLock(*this, Rect, ERWLockType::Write);
	FIntRect const IndexCellsRect = FIntRect::DivideAndRoundUp(Rect, IndexCellSize);
	for (int64 Y = IndexCellsRect.Min.Y; Y < IndexCellsRect.Max.Y; ++Y)
	{
		int64 const IndexOffset = Y * IndexCellsNum.X;
		for (int64 X = IndexCellsRect.Min.X; X < IndexCellsRect.Max.X; ++X)
		{
			int64 const Index = IndexOffset + X;
			SpatialGridData[Index].RemoveAllSwap([&Rect, &Predicate](FIndexEntry const & IndexEntry) -> bool { return Rect.Intersect(IndexEntry.Key) && Predicate(IndexEntry); });
		}
	}
}

template <typename DataType>
TMap<FIntRect, DataType> TUEConcurrentSpatialGridIndex<DataType>::GetOverlapping(FIntRect const & Rect) const
{
	CheckRange(Rect);
	TMap<FIntRect, DataType> OverlappingRectsInfo;
	FIntRect const IndexCellsRect = FIntRect::DivideAndRoundUp(Rect, IndexCellSize);
	
	FRWRectScopeLock RRectScopeLock(*this, Rect, ERWLockType::ReadOnly);

	for (int64 Y = IndexCellsRect.Min.Y; Y < IndexCellsRect.Max.Y; ++Y)
	{
		int64 const IndexOffset = Y * IndexCellsNum.X;
		for (int64 X = IndexCellsRect.Min.X; X < IndexCellsRect.Max.X; ++X)
		{
			int64 const Index = IndexOffset + X;
			for (FIndexEntry const & IndexEntry : SpatialGridData[Index])
			{
				if (Rect.Intersect(IndexEntry.Key))
				{
					OverlappingRectsInfo.Emplace(IndexEntry.Key, IndexEntry.Value);
				}
			}
		}
	}
	return OverlappingRectsInfo;
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::GetLocks(FIntRect const & Rect, ERWLockType const LockType) const
{
	CheckRange(Rect);
	FIntRect const LockCellsRect = Rect / LockCellSize;
	for (int64 Y = LockCellsRect.Min.Y; Y <= LockCellsRect.Max.Y; ++Y)
	{
		int64 const IndexOffset = Y * LockCellsNum.X;
		for (int64 X = LockCellsRect.Min.X; X <= LockCellsRect.Max.X; ++X)
		{
			int64 const Index = IndexOffset + X;
			if (LockType == ERWLockType::ReadOnly)
			{
				Locks[Index].ReadLock();
			}
			else
			{
				Locks[Index].WriteLock();
			}
		}
	}
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::FreeLocks(FIntRect const & Rect, ERWLockType const LockType) const
{
	CheckRange(Rect);
	FIntRect const LockCellsRect = Rect / LockCellSize;
	for (int64 Y = LockCellsRect.Max.Y; Y >= LockCellsRect.Min.Y; --Y)
	{
		int64 const IndexOffset = Y * LockCellsNum.X;
		for (int64 X = LockCellsRect.Max.X; X >= LockCellsRect.Min.X; --X)
		{
			int64 const Index = IndexOffset + X;
			if (LockType == ERWLockType::ReadOnly)
			{
				Locks[Index].ReadUnlock();
			}
			else
			{
				Locks[Index].WriteUnlock();
			}
		}
	}
}

template <typename DataType>
template <typename ArgType>
void TUEConcurrentSpatialGridIndex<DataType>::InsertUncheckedNoLock(FIntRect const & Rect, ArgType && Data)
{
	CheckRange(Rect);
	FIntRect const IndexCellsRect = FIntRect::DivideAndRoundUp(Rect, IndexCellSize);
	for (int64 Y = IndexCellsRect.Min.Y; Y < IndexCellsRect.Max.Y; ++Y)
	{
		int64 const IndexOffset = Y * IndexCellsNum.X;
		for (int64 X = IndexCellsRect.Min.X; X < IndexCellsRect.Max.X; ++X)
		{
			int64 const Index = IndexOffset + X;
			SpatialGridData[Index].Push({ Rect, Forward<ArgType>(Data) });
		}
	}
}

template <typename DataType>
bool TUEConcurrentSpatialGridIndex<DataType>::CheckIfFreeNoLock(FIntRect const & Rect) const
{
	CheckRange(Rect);
	FIntRect const IndexCellsRect = FIntRect::DivideAndRoundUp(Rect, IndexCellSize);
	bool IsFree = true;
	for (int64 Y = IndexCellsRect.Min.Y; Y < IndexCellsRect.Max.Y; ++Y)
	{
		int64 const IndexOffset = Y * IndexCellsNum.X;
		for (int64 X = IndexCellsRect.Min.X; X < IndexCellsRect.Max.X; ++X)
		{
			int64 const Index = IndexOffset + X;
			for (FIndexEntry const & IndexEntry : SpatialGridData[Index])
			{
				if (Rect.Intersect(IndexEntry.Key))
				{
					IsFree = false;
					goto EndLoop;
				}
			}
		}
	}
EndLoop:
	return IsFree;
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::CheckRange(FIntPoint const Coords) const
{
	check((Coords.X >= 0) && (Coords.X < Size.X) && (Coords.Y >= 0) && (Coords.Y < Size.Y));
}

template <typename DataType>
void TUEConcurrentSpatialGridIndex<DataType>::CheckRange(FIntRect const & Rect) const
{
	CheckRange(Rect.Min);
	CheckRange(Rect.Max);
}

template <typename DataType>
TUEConcurrentSpatialGridIndex<DataType>::FRWRectScopeLock::FRWRectScopeLock(TUEConcurrentSpatialGridIndex<DataType> const & InSpatialGridIndex, FIntRect const & InRect, ERWLockType const InLockType)
	: SpatialGridIndex(InSpatialGridIndex)
	, Rect(InRect)
	, LockType(InLockType)
{
	SpatialGridIndex.GetLocks(Rect, LockType);
}

template <typename DataType>
TUEConcurrentSpatialGridIndex<DataType>::FRWRectScopeLock::~FRWRectScopeLock()
{
	SpatialGridIndex.FreeLocks(Rect, LockType);
}