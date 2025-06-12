#include "Light.h"
#include "Inputs.h"
#include "Time.h"

void Light::HandleInputs()
{
	if (Inputs::Instance()->IsPressed(SDL_SCANCODE_I))
	{
		position.z -= 2.0f;
	}
	if (Inputs::Instance()->IsPressed(SDL_SCANCODE_K))
	{
		position.z += 2.0f;
	}
	if (Inputs::Instance()->IsPressed(SDL_SCANCODE_J))
	{
		position.x -= 2.0f;
	}
	if (Inputs::Instance()->IsPressed(SDL_SCANCODE_L))
	{
		position.x += 2.0f;
	}
}
