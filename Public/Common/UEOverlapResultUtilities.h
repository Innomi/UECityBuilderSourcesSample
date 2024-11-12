// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

struct FOverlapResult;

struct UNDEADEMPIRE_API FOverlapResultCompare
{
public:
	FOverlapResultCompare(FOverlapResult const & InOverlapResult);
	bool operator () (FOverlapResult const & InOverlapResult);

private:
	FOverlapResult const & OverlapResult;
};