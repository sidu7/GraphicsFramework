#pragma once

#include "Core/Application.h"

class AdvancedRenderingTechniquesApplication : public Application
{
public:

	virtual Settings GetSettings() override;

	virtual void Init() override;
	virtual void Close() override;

	void ProjectSelector();

protected:
	int SelectedProject = 0;
};