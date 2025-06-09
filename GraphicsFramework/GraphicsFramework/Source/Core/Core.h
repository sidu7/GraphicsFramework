#pragma once

#include <iostream>
#include <fstream>
#include <stack>

// RapidJson
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

// GLM
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

// SDL
#include <SDL/include/SDL.h>

// Imgui
#include <Imgui/imgui.h>

// Graphics Framework
#include "GlobalDefines.h"
#include "Singleton.h"
#include "Inputs.h"
#include "Object.h"
#include "Time.h"
#include "Engine.h"
#include "Scene.h"