#pragma once
#include <SDL.h>
#include "Singleton.h"

class Inputs
{
	SINGLETON(Inputs)
public:	
	void Init();
	void Update();
	void Close();
private:
};

