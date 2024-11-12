// Fill out your copyright notice in the Description page of Project Settings.

#include "Common/UEOverlapResultUtilities.h"
#include "Engine/OverlapResult.h"

FOverlapResultCompare::FOverlapResultCompare(FOverlapResult const & InOverlapResult)
	: OverlapResult(InOverlapResult)
{
}

bool FOverlapResultCompare::operator () (FOverlapResult const & InOverlapResult)
{
	return OverlapResult.Component.HasSameIndexAndSerialNumber(InOverlapResult.Component) && OverlapResult.ItemIndex == InOverlapResult.ItemIndex;
}