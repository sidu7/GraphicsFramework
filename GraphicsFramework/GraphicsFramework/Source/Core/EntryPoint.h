#pragma once

#include "Core/CommandLine.h"
#include "Core/Engine.h"
#include "Core/Application.h"

// Main declaration
extern Application* CreateApplication();

int main(int argc, char** argv)
{
	Application* App = CreateApplication();
	
	CommandLine::Instance()->Init(argc, argv);
	Engine::Instance()->Start(App->GetSettingsFilePath());	
	App->Init();
	Engine::Instance()->Run();
	App->Close();
	Engine::Instance()->Stop();	

	delete App;
	return 0;
}