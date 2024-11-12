// Fill out your copyright notice in the Description page of Project Settings.

#include "Grid/UEGridLayer.h"

FUEGridLayer::FUEGridLayer(FUintPoint const InSize)
{
	SetSize(InSize);
}

FBitReference FUEGridLayer::operator [](FUintPoint const Coords)
{
	CheckRange(Coords);
	return GetTileByCellCoords(Coords)[GetCoordsInTile(Coords)];
}

FConstBitReference const FUEGridLayer::operator [](FUintPoint const Coords) const
{
	CheckRange(Coords);
	return GetTileByCellCoords(Coords)[GetCoordsInTile(Coords)];
}

bool FUEGridLayer::GetCell(FUintPoint const Coords) const
{
	return operator [](Coords);
}

bool FUEGridLayer::Contains(FUintRect const & Rect, bool const bValue) const
{
	// TODO: it's ugly as fuck, rewrite it. Have no time right now.
	if (Rect.IsEmpty())
	{
		return false;
	}
	CheckRange(Rect);

	FUintRect const TilesToCheck{ Rect.Min / FGridTile::GetSize(), (Rect.Max - FUintPoint{ 1, 1 }) / FGridTile::GetSize() };
	uint32 const StartWordIndex = Rect.Min.X % NumWordsPerTile;
	WordType const StartMask = FullWordMask << (Rect.Min.Y % NumBitsPerWord);
	uint32 const EndWordIndex = ((Rect.Max.X - 1) % NumWordsPerTile) + 1;
	WordType const EndMask = FullWordMask >> ((NumBitsPerWord - (Rect.Max.Y % NumBitsPerWord)) % NumBitsPerWord);

	auto CheckColumn = [this, StartWordIndex, EndWordIndex, bValue] (uint32 const FirstTileIndex, uint32 const LastTileIndex, WordType const Mask) -> bool
		{
			if (GridLayerData[FirstTileIndex].Contains(StartWordIndex, NumWordsPerTile, Mask, bValue))
			{
				return true;
			}
			for (uint32 TileIndex = FirstTileIndex + 1; TileIndex < LastTileIndex; ++TileIndex)
			{
				if (GridLayerData[TileIndex].Contains(0, NumWordsPerTile, Mask, bValue))
				{
					return true;
				}
			}
			return GridLayerData[LastTileIndex].Contains(0, EndWordIndex, Mask, bValue);
		};

	if (TilesToCheck.Width() == 0)
	{
		if (TilesToCheck.Height() == 0)
		{
			return GetTile(TilesToCheck.Min).Contains(StartWordIndex, EndWordIndex, StartMask & EndMask, bValue);
		}
		else
		{
			uint32 const FirstTileIndex = GetTileIndex(TilesToCheck.Min);
			uint32 const LastTileIndex = GetTileIndex(TilesToCheck.Max);
			if (GridLayerData[FirstTileIndex].Contains(StartWordIndex, EndWordIndex, StartMask, bValue))
			{
				return true;
			}
			for (uint32 TileIndex = FirstTileIndex + GetXTileNum(); TileIndex < LastTileIndex; TileIndex += GetXTileNum())
			{
				if (GridLayerData[TileIndex].Contains(StartWordIndex, EndWordIndex, FullWordMask, bValue))
				{
					return true;
				}
			}
			return GridLayerData[LastTileIndex].Contains(StartWordIndex, EndWordIndex, EndMask, bValue);
		}
	}
	else
	{
		if (TilesToCheck.Height() == 0)
		{
			uint32 const FirstTileIndex = GetTileIndex(TilesToCheck.Min);
			uint32 const LastTileIndex = GetTileIndex(TilesToCheck.Max);
			return CheckColumn(FirstTileIndex, LastTileIndex, StartMask & EndMask);
		}
		else
		{
			// Checking first column.
			{
				uint32 const FirstTileIndex = GetTileIndex(TilesToCheck.Min);
				uint32 const LastTileIndex = GetTileIndex(FUintPoint{ TilesToCheck.Max.X, TilesToCheck.Min.Y });
				if (CheckColumn(FirstTileIndex, LastTileIndex, StartMask))
				{
					return true;
				}
			}

			// Checking internal columns.
			{
				uint32 const InnerXTilesNum = TilesToCheck.Width() - 1;
				uint32 const FirstColumnTileIndex = GetTileIndex(FUintPoint{ TilesToCheck.Min.X, TilesToCheck.Min.Y + 1 });
				uint32 const LastColumnTileIndex = GetTileIndex(FUintPoint{ TilesToCheck.Min.X, TilesToCheck.Max.Y });
				BYTE const Test = bValue ? 0u : ~0u;
				for (uint32 ColumnTileIndex = FirstColumnTileIndex; ColumnTileIndex < LastColumnTileIndex; ColumnTileIndex += GetXTileNum())
				{
					if (GridLayerData[ColumnTileIndex].Contains(StartWordIndex, NumWordsPerTile, FullWordMask, bValue))
					{
						return true;
					}
					void const * const Start = static_cast<void const *>(GridLayerData.GetData() + ColumnTileIndex + 1);
					void const * const End = static_cast<void const *>(GridLayerData.GetData() + ColumnTileIndex + 1 + InnerXTilesNum);
					for (BYTE const * Byte = static_cast<BYTE const *>(Start); Byte < End; ++Byte)
					{
						if (*Byte != Test)
						{
							return true;
						}
					}
					if (GridLayerData[ColumnTileIndex + 1 + InnerXTilesNum].Contains(0, EndWordIndex, FullWordMask, bValue))
					{
						return true;
					}
				}
			}

			// Checking last column.
			{
				uint32 const FirstTileIndex = GetTileIndex(FUintPoint{ TilesToCheck.Min.X, TilesToCheck.Max.Y });
				uint32 const LastTileIndex = GetTileIndex(TilesToCheck.Max);
				return CheckColumn(FirstTileIndex, LastTileIndex, EndMask);
			}
		}
	}
}

void FUEGridLayer::SetCells(FUintRect const & Rect, bool const bValue)
{
	if (Rect.IsEmpty())
	{
		return;
	}
	CheckRange(Rect);

	FUintRect const TilesToSet{ Rect.Min / FGridTile::GetSize(), (Rect.Max - FUintPoint{ 1, 1 }) / FGridTile::GetSize() };
	uint32 const StartWordIndex = Rect.Min.X % NumWordsPerTile;
	WordType const StartMask = FullWordMask << (Rect.Min.Y % NumBitsPerWord);
	uint32 const EndWordIndex = ((Rect.Max.X - 1) % NumWordsPerTile) + 1;
	WordType const EndMask = FullWordMask >> ((NumBitsPerWord - (Rect.Max.Y % NumBitsPerWord)) % NumBitsPerWord);

	auto SetColumn = [this, StartWordIndex, EndWordIndex, bValue] (uint32 const FirstTileIndex, uint32 const LastTileIndex, WordType const Mask)
		{
			GridLayerData[FirstTileIndex].SetCells(StartWordIndex, NumWordsPerTile, Mask, bValue);
			for (uint32 TileIndex = FirstTileIndex + 1; TileIndex < LastTileIndex; ++TileIndex)
			{
				GridLayerData[TileIndex].SetCells(0, NumWordsPerTile, Mask, bValue);
			}
			GridLayerData[LastTileIndex].SetCells(0, EndWordIndex, Mask, bValue);
		};

	if (TilesToSet.Width() == 0)
	{
		if (TilesToSet.Height() == 0)
		{
			GetTile(TilesToSet.Min).SetCells(StartWordIndex, EndWordIndex, StartMask & EndMask, bValue);
		}
		else
		{
			uint32 const FirstTileIndex = GetTileIndex(TilesToSet.Min);
			uint32 const LastTileIndex = GetTileIndex(TilesToSet.Max);
			GridLayerData[FirstTileIndex].SetCells(StartWordIndex, EndWordIndex, StartMask, bValue);
			for (uint32 TileIndex = FirstTileIndex + GetXTileNum(); TileIndex < LastTileIndex; TileIndex += GetXTileNum())
			{
				GridLayerData[TileIndex].SetCells(StartWordIndex, EndWordIndex, FullWordMask, bValue);
			}
			GridLayerData[LastTileIndex].SetCells(StartWordIndex, EndWordIndex, EndMask, bValue);
		}
	}
	else
	{
		if (TilesToSet.Height() == 0)
		{
			uint32 const FirstTileIndex = GetTileIndex(TilesToSet.Min);
			uint32 const LastTileIndex = GetTileIndex(TilesToSet.Max);
			SetColumn(FirstTileIndex, LastTileIndex, StartMask & EndMask);
		}
		else
		{
			// Setting first column.
			{
				uint32 const FirstTileIndex = GetTileIndex(TilesToSet.Min);
				uint32 const LastTileIndex = GetTileIndex(FUintPoint{ TilesToSet.Max.X, TilesToSet.Min.Y });
				SetColumn(FirstTileIndex, LastTileIndex, StartMask);
			}

			// Setting internal columns.
			{
				uint32 const InnerXTilesNum = TilesToSet.Width() - 1;
				uint32 const FirstColumnTileIndex = GetTileIndex(FUintPoint{ TilesToSet.Min.X, TilesToSet.Min.Y + 1 });
				uint32 const LastColumnTileIndex = GetTileIndex(FUintPoint{ TilesToSet.Min.X, TilesToSet.Max.Y });
				for (uint32 ColumnTileIndex = FirstColumnTileIndex; ColumnTileIndex < LastColumnTileIndex; ColumnTileIndex += GetXTileNum())
				{
					GridLayerData[ColumnTileIndex].SetCells(StartWordIndex, NumWordsPerTile, FullWordMask, bValue);
					// Set fully covered tiles.
					FMemory::Memset(GridLayerData.GetData() + ColumnTileIndex + 1, (bValue ? 0xff : 0), InnerXTilesNum * sizeof(FGridTile));
					GridLayerData[ColumnTileIndex + 1 + InnerXTilesNum].SetCells(0, EndWordIndex, FullWordMask, bValue);
				}
			}

			// Setting last column.
			{
				uint32 const FirstTileIndex = GetTileIndex(FUintPoint{ TilesToSet.Min.X, TilesToSet.Max.Y });
				uint32 const LastTileIndex = GetTileIndex(TilesToSet.Max);
				SetColumn(FirstTileIndex, LastTileIndex, EndMask);
			}
		}
	}
}

FUintPoint FUEGridLayer::GetSize() const
{
	return Size;
}

uint32 FUEGridLayer::GetXSize() const
{
	return Size.X;
}

uint32 FUEGridLayer::GetYSize() const
{
	return Size.Y;
}

FUintPoint FUEGridLayer::GetCoordsInTile(FUintPoint const Coords) const
{
	return FUintPoint{ Coords.X % NumWordsPerTile, Coords.Y % NumBitsPerWord };
}

FUEGridLayer::FGridTile const & FUEGridLayer::GetTile(FUintPoint const Coords) const
{
	return GridLayerData[GetTileIndex(Coords)];
}

FUEGridLayer::FGridTile & FUEGridLayer::GetTile(FUintPoint const Coords)
{
	return const_cast<FGridTile &>(static_cast<FUEGridLayer const &>(*this).GetTile(Coords)); // Hate Scott Meyers, btw.
}

FUEGridLayer::FGridTile const & FUEGridLayer::GetTileByCellCoords(FUintPoint const Coords) const
{
	CheckRange(Coords);
	uint32 const TileX = Coords.X / NumWordsPerTile;
	uint32 const TileY = Coords.Y / NumBitsPerWord;
	return GetTile(FUintPoint{ TileX, TileY });
}

FUEGridLayer::FGridTile & FUEGridLayer::GetTileByCellCoords(FUintPoint const Coords)
{
	return const_cast<FGridTile &>(static_cast<FUEGridLayer const &>(*this).GetTileByCellCoords(Coords)); // Hate Scott Meyers, btw.
}

uint32 FUEGridLayer::GetTileIndex(FUintPoint const Coords) const
{
	return Coords.Y * GetXTileNum() + Coords.X;
}

FUintPoint FUEGridLayer::GetTileNum() const
{
	return FUintPoint{ GetXTileNum(), GetYTileNum() };
}

uint32 FUEGridLayer::GetXTileNum() const
{
	return GetXSize() / NumWordsPerTile;
}

uint32 FUEGridLayer::GetYTileNum() const
{
	return GetYSize() / NumBitsPerWord;
}

void FUEGridLayer::CheckRange(FUintPoint const Coords) const
{
	check((Coords.X < GetXSize()) && (Coords.Y < GetYSize()));
}

void FUEGridLayer::CheckRange(FUintRect const & Rect) const
{
	CheckRange(Rect.Min);
	CheckRange(Rect.IsEmpty() ? Rect.Max : Rect.Max - FUintPoint{ 1, 1 });
	check(Rect.Min.X <= Rect.Max.X && Rect.Min.Y <= Rect.Max.Y);
}

void FUEGridLayer::SetSize(FUintPoint const NewSize)
{
	check((NewSize.X <= MAX_uint32 - (NumWordsPerTile - 1)) && (NewSize.Y <= MAX_uint32 - (NumBitsPerWord - 1)));
	Size = FUintPoint{ ((NewSize.X + (NumWordsPerTile - 1)) / NumWordsPerTile) * NumWordsPerTile, ((NewSize.Y + (NumBitsPerWord - 1)) / NumBitsPerWord) * NumBitsPerWord };
	GridLayerData.Empty();
	GridLayerData.SetNumZeroed(GetXTileNum() * GetYTileNum());
}

FUEGridLayer::FGridTile::FGridTile(FGridTile const & GridTile) noexcept
{
	for (uint32 WordIndex = 0; WordIndex < NumWordsPerTile; ++WordIndex)
	{
		GridCells[WordIndex] = GridTile.GridCells[WordIndex];
	}
}

FBitReference FUEGridLayer::FGridTile::operator [](FUintPoint const Coords)
{
	CheckRange(Coords);
	return FBitReference(GridCells[Coords.X], 1 << Coords.Y);
}

FConstBitReference const FUEGridLayer::FGridTile::operator [](FUintPoint const Coords) const
{
	CheckRange(Coords);
	return FConstBitReference(GridCells[Coords.X], 1 << Coords.Y);
}

bool FUEGridLayer::FGridTile::Contains(uint32 const FromWordIndex, uint32 const ToWordIndex, WordType const Mask, bool const bValue) const
{
	check((FromWordIndex <= ToWordIndex) && (ToWordIndex <= NumWordsPerTile));
	WordType const Test = bValue ? 0u : ~0u;
	for (uint32 WordIndex = FromWordIndex; WordIndex < ToWordIndex; ++WordIndex)
	{
		if ((GridCells[WordIndex] & Mask) != (Test & Mask))
		{
			return true;
		}
	}
	return false;
}

void FUEGridLayer::FGridTile::SetCells(uint32 const FromWordIndex, uint32 const ToWordIndex, WordType const Mask, bool const bValue)
{
	check((FromWordIndex <= ToWordIndex) && (ToWordIndex <= NumWordsPerTile));
	if (bValue)
	{
		for (uint32 WordIndex = FromWordIndex; WordIndex < ToWordIndex; ++WordIndex)
		{
			GridCells[WordIndex] |= Mask;
		}
	}
	else
	{
		for (uint32 WordIndex = FromWordIndex; WordIndex < ToWordIndex; ++WordIndex)
		{
			GridCells[WordIndex] &= ~Mask;
		}
	}
}

FUintPoint FUEGridLayer::FGridTile::GetSize()
{
	return FUintPoint{ GetXSize(), GetYSize() };
}

uint32 FUEGridLayer::FGridTile::GetXSize()
{
	return NumWordsPerTile;
}

uint32 FUEGridLayer::FGridTile::GetYSize()
{
	return NumBitsPerWord;
}

void FUEGridLayer::FGridTile::CheckRange(FUintPoint const Coords) const
{
	check((Coords.X < GetXSize()) && (Coords.Y < GetYSize()));
}
