////////////////////////////////////////////////////////////////////////
// The scene class contains all the parameters needed to define and
// draw a simple scene, including:
//   * Geometry
//   * Light parameters
//   * Material properties
//   * Viewport size parameters
//   * Viewing transformation values
//   * others ...
//
// Some of these parameters are set when the scene is built, and
// others are set by the framework in response to user mouse/keyboard
// interactions.  All of them can be used to draw the scene.

#include "shapes.h"
#include "object.h"
#include "texture.h"

class FBO;

enum ObjectIds {
    nullId	= 0,
    skyId	= 1,
    seaId	= 2,
    groundId	= 3,
    roomId	= 4,
    boxId	= 5,
    frameId	= 6,
    lPicId	= 7,
    rPicId	= 8,
    teapotId	= 9,
    spheresId	= 10,
};

class Shader;

class Scene
{
public:
    // Viewing transformation parameters (suggested) FIXME: This is a
    // good place for the transformation values which are set by the
    // user mouse/keyboard actions and used in DrawScene to create the
    // transformation matrices.

    ProceduralGround* ground;

    // Light position parameters
    float lightSpin, lightTilt, lightDist;

	//Camera parameters
	float wtilt, wspin, zoom;
	float tx, ty;

	//Eye parameters
	vec3 eye;
	float speed;
	float gspin, gtilt;
	float time_since_last_refresh;
	bool GameCam;
	bool move[4]; // 0: forward, 1: backward, 2:left, 3: right

	//Lighting 
	bool BRDF;

    vec3 basePoint;  // Records where the scene building is centered.
    int mode; // Extra mode indicator hooked up to number keys and sent to shader
    
    // Viewport
    int width, height;

    // All objects in the scene are children of this single root object.
    Object* objectRoot;
    std::vector<Object*> animated;

    // Shader programs
    ShaderProgram* lightingProgram;
	ShaderProgram* shadowProgram;

	//FBO for ShadowMap
	FBO* shadowMap;
	FBO* G_Buffer;

    //void append(Object* m) { objects.push_back(m); }

    void InitializeScene();
    void DrawScene();

};
