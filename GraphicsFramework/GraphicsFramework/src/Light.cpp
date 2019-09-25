#include "Light.h"
#include "Inputs.h"
#include "Time.h"

void Light::HandleInputs()
{
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_I))
	{
		position.x += 0.5f;
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_K))
	{
		position.x -= 0.5f;
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_J))
	{
		position.z += 0.5f;
	}
	if (Inputs::Instance().IsPressed(SDL_SCANCODE_L))
	{
		position.z -= 0.5f;
	}
}
