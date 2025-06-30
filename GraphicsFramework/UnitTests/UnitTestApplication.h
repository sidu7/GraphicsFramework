#pragma once
#include "Core/Application.h"

class UnitTestApplication : public Application
{
public:
	virtual std::string GetSettingsFilePath() override;

	virtual void Init() override;
	virtual void Close() override;

	void UnitTestSelector();

protected:
	int SelectedUnitTest = 0;
};