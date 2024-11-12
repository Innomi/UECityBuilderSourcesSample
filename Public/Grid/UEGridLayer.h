// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class UNDEADEMPIRE_API FUEGridLayer
{
public:
	/**
	 * Sets size of layer.
	 * @param InSize - X and Y will be padded to be multiple of NumWordsPerTile and NumBitsPerDWORD accordingly.
	 */
	explicit FUEGridLayer(FUintPoint const InSize);

	FBitReference operator [](FUintPoint const Coords);
	FConstBitReference const operator [](FUintPoint const Coords) const;

	/** Returns state of a grid cell. */
	bool GetCell(FUintPoint const Coords) const;

	bool Contains(FUintRect const & Rect, bool const bValue) const;
	void SetCells(FUintRect const & Rect, bool const bValue);

	/** Size getters. */
	FUintPoint GetSize() const;
	uint32 GetXSize() const;
	uint32 GetYSize() const;

private:
	using WordType = uint32;
	static constexpr uint32 NumWordsPerTile = 16;
	static constexpr uint32 NumBitsPerWord = sizeof(WordType) * 8;
	static constexpr WordType FullWordMask = ~0u;

	/** Tile of grid. Contains info about 16 x 32 grid cells. 64 bytes to fit in one cache line of most modern CPUs. */
	struct alignas(64) FGridTile
	{
	public:
		/** Necessary for TArray specialization. */
		FGridTile(FGridTile const & GridTile) noexcept;

		FBitReference operator [](FUintPoint const Coords);
		FConstBitReference const operator [](FUintPoint const Coords) const;
		
		bool Contains(uint32 const FromWordIndex, uint32 const ToWordIndex, WordType const Mask, bool const bValue) const;
		void SetCells(uint32 const FromWordIndex, uint32 const ToWordIndex, WordType const Mask, bool const bValue);

		static FUintPoint GetSize();
		static uint32 GetXSize();
		static uint32 GetYSize();

	private:
		void CheckRange(FUintPoint const Coords) const;

		WordType GridCells[NumWordsPerTile];
	};

	FUintPoint GetCoordsInTile(FUintPoint const Coords) const;
	FGridTile const & GetTile(FUintPoint const Coords) const;
	FGridTile & GetTile(FUintPoint const Coords);
	FGridTile const & GetTileByCellCoords(FUintPoint const Coords) const;
	FGridTile & GetTileByCellCoords(FUintPoint const Coords);
	uint32 GetTileIndex(FUintPoint const Coords) const;
	FUintPoint GetTileNum() const;
	uint32 GetXTileNum() const;
	uint32 GetYTileNum() const;
	void CheckRange(FUintPoint const Coords) const;
	void CheckRange(FUintRect const & Rect) const;
	void SetSize(FUintPoint const NewSize);

	TArray<FGridTile> GridLayerData;
	FUintPoint Size;
};
