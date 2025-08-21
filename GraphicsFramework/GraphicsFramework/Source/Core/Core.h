#pragma once

#include <iostream>
#include <fstream>
#include <stack>
#include <algorithm>
#include <sstream>
#include <random>
#include <chrono>
#include <functional>

// STL
#include <vector>
#include <set>

// RapidJson
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/filewritestream.h>

// GLM
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext.hpp>
#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/norm.hpp"
#include <glm/gtc/random.hpp>

// SDL
#include <SDL/include/SDL.h>

// Imgui
#include <Imgui/imgui.h>

// Stbi
#include <stbi_image\stbi_image.h>

// Graphics Framework
#include "GlobalDefines.h"
#include "Singleton.h"
#include "Inputs.h"
#include "Object.h"
#include "Time.h"
#include "Scene.h"