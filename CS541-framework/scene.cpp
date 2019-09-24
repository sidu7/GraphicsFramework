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

#include "math.h"
#include <fstream>
#include <stdlib.h>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#include <freeglut.h>
#include <glu.h>                // For gluErrorString

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#include "shader.h"
#include "shapes.h"
#include "scene.h"
#include "object.h"
#include "texture.h"
#include "transform.h"
#include "fbo.h"

const float PI = 3.14159f;
const float rad = PI/180.0f;    // Convert degrees to radians

MAT4 Identity;

const float grndSize = 100.0;    // Island radius;  Minimum about 20;  Maximum 1000 or so
const int   grndTiles = int(grndSize);
const float grndOctaves = 4.0;  // Number of levels of detail to compute
const float grndFreq = 0.03;    // Number of hills per (approx) 50m
const float grndPersistence = 0.03; // Terrain roughness: Slight:0.01  rough:0.05
const float grndLow = -3.0;         // Lowest extent below sea level
const float grndHigh = 5.0;        // Highest extent above sea level

////////////////////////////////////////////////////////////////////////
// This macro makes it easy to sprinkle checks for OpenGL errors
// throughout your code.  Most OpenGL calls can record errors, and a
// careful programmer will check the error status *often*, perhaps as
// often as after every OpenGL call.  At the very least, once per
// refresh will tell you if something is going wrong.
#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line scene.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }

// Create an RGB color from human friendly parameters: hue, saturation, value
vec3 HSV2RGB(const float h, const float s, const float v)
{
    if (s == 0.0)
        return vec3(v,v,v);

    int i = (int)(h*6.0) % 6;
    float f = (h*6.0f) - i;
    float p = v*(1.0f - s);
    float q = v*(1.0f - s*f);
    float t = v*(1.0f - s*(1.0f-f));
    if      (i == 0)     return vec3(v,t,p);
    else if (i == 1)  return vec3(q,v,p);
    else if (i == 2)  return vec3(p,v,t);
    else if (i == 3)  return vec3(p,q,v);
    else if (i == 4)  return vec3(t,p,v);
    else   /*i == 5*/ return vec3(v,p,q);
}

////////////////////////////////////////////////////////////////////////
// Constructs a hemisphere of spheres of varying hues
Object* SphereOfSpheres(Shape* SpherePolygons)
{
    Object* ob = new Object(NULL, nullId);
    
    for (float angle=0.0;  angle<360.0;  angle+= 18.0)
        for (float row=0.075;  row<PI/2.0;  row += PI/2.0/6.0) {   
            vec3 hue = HSV2RGB(angle/360.0, 1.0f-2.0f*row/PI, 1.0f);

            Object* sp = new Object(SpherePolygons, spheresId,
                                    hue, vec3(1.0, 1.0, 1.0), 120.0);
            float s = sin(row);
            float c = cos(row);
            ob->add(sp, Rotate(2,angle)*Translate(c,0,s)*Scale(0.075*c,0.075*c,0.075*c));
        }
    return ob;
}

////////////////////////////////////////////////////////////////////////
// Constructs a -1...+1  quad (canvas) framed by four (elongated) boxes
Object* FramedPicture(const MAT4& modelTr, const int objectId, 
                      Shape* BoxPolygons, Shape* QuadPolygons)
{
    // This draws the frame as four (elongated) boxes of size +-1.0
    float w = 0.05;             // Width of frame boards.
    
    Object* frame = new Object(NULL, nullId);
    Object* ob;
    
    vec3 woodColor(87.0/255.0,51.0/255.0,35.0/255.0);
    ob = new Object(BoxPolygons, frameId,
                    woodColor, vec3(0.2, 0.2, 0.2), 10.0);
    frame->add(ob, Translate(0.0, 0.0, 1.0+w)*Scale(1.0, w, w));
    frame->add(ob, Translate(0.0, 0.0, -1.0-w)*Scale(1.0, w, w));
    frame->add(ob, Translate(1.0+w, 0.0, 0.0)*Scale(w, w, 1.0+2*w));
    frame->add(ob, Translate(-1.0-w, 0.0, 0.0)*Scale(w, w, 1.0+2*w));

    ob = new Object(QuadPolygons, objectId,
                    woodColor, vec3(0.0, 0.0, 0.0), 10.0);
    frame->add(ob, Rotate(0,90));

    return frame;
}

////////////////////////////////////////////////////////////////////////
// A callback procedure, called regularly to update the atime global
// variable.
float atime = 0.0;
void animate(int value)
{
    atime = 360.0*glutGet((GLenum)GLUT_ELAPSED_TIME)/36000;
    glutPostRedisplay();

    // Schedule next call to this function
    glutTimerFunc(30, animate, 1);
}

////////////////////////////////////////////////////////////////////////
// InitializeScene is called once during setup to create all the
// textures, shape VAOs, and shader programs as well as setting a
// number of other parameters.
void Scene::InitializeScene()
{
    glEnable(GL_DEPTH_TEST);
    CHECKERROR;

    // FIXME: This is a good place for initializing the transformation
    // values.


    CHECKERROR;
    objectRoot = new Object(NULL, nullId);
    
    // Set the initial light position parammeters
    lightSpin = 150.0;
    lightTilt = -45.0;
    lightDist = 1000.0;

    // Enable OpenGL depth-testing
    glEnable(GL_DEPTH_TEST);

    // FIXME:  Change false to true to randomize the central object position.
    ground =  new ProceduralGround(grndSize, grndTiles, grndOctaves, grndFreq,
                                   grndPersistence, grndLow, grndHigh, false);

    basePoint = ground->highPoint;

    // Create the lighting shader program from source code files.
    lightingProgram = new ShaderProgram();
    lightingProgram->AddShader("lighting.vert", GL_VERTEX_SHADER);
    lightingProgram->AddShader("lighting.frag", GL_FRAGMENT_SHADER);

    glBindAttribLocation(lightingProgram->programId, 0, "vertex");
    glBindAttribLocation(lightingProgram->programId, 1, "vertexNormal");
    glBindAttribLocation(lightingProgram->programId, 2, "vertexTexture");
    glBindAttribLocation(lightingProgram->programId, 3, "vertexTangent");
    lightingProgram->LinkProgram();

	// Create the shadows shader program from source code files.
	shadowProgram = new ShaderProgram();
	shadowProgram->AddShader("shadow.vert", GL_VERTEX_SHADER);
	shadowProgram->AddShader("shadow.frag", GL_FRAGMENT_SHADER);

	glBindAttribLocation(shadowProgram->programId, 0, "vertex");
	shadowProgram->LinkProgram();

    // Create all the Polygon shapes
    Shape* TeapotPolygons =  new Teapot(12);
    Shape* BoxPolygons = new Ply("box.ply");
    Shape* SpherePolygons = new Sphere(32);
    Shape* RoomPolygons = new Ply("room.ply");
    Shape* GroundPolygons = ground;
    Shape* QuadPolygons = new Quad();
    Shape* SeaPolygons = new Plane(2000.0, 50);

    // Various colors used in the subsequent models
    vec3 woodColor(87.0/255.0, 51.0/255.0, 35.0/255.0);
    vec3 brickColor(134.0/255.0, 60.0/255.0, 56.0/255.0);
    vec3 brassColor(0.5, 0.5, 0.1);
    vec3 grassColor(62.0/255.0, 102.0/255.0, 38.0/255.0);
    vec3 waterColor(0.3, 0.3, 1.0);
 
    // Creates all the models from which the scene is composed.  Each
    // is created with a polygon shape (possibly NULL), a
    // transformation, and the surface lighting parameters Kd, Ks, and
    // alpha.
    Object* central    = new Object(NULL, nullId);
    Object* anim       = new Object(NULL, nullId);
    Object* room       = new Object(RoomPolygons, roomId, brickColor, vec3(0.0, 0.0, 0.0), 1);
    Object* teapot     = new Object(TeapotPolygons, teapotId, brassColor, vec3(0.5, 0.5, 0.5), 120);
    Object* podium     = new Object(BoxPolygons, boxId, vec3(woodColor), vec3(0.3, 0.3, 0.3), 10); 
    Object* sky        = new Object(SpherePolygons, skyId, vec3(), vec3(), 0);
    Object* ground     = new Object(GroundPolygons, groundId, grassColor, vec3(0.0, 0.0, 0.0), 1);
    Object* sea        = new Object(SeaPolygons, seaId, waterColor, vec3(1.0, 1.0, 1.0), 120);
    Object* spheres    = SphereOfSpheres(SpherePolygons);
    Object* leftFrame  = FramedPicture(Identity, lPicId, BoxPolygons, QuadPolygons);
    Object* rightFrame = FramedPicture(Identity, rPicId, BoxPolygons, QuadPolygons); 

    // FIXME: This is where you could read in all the textures and
    // associate them with the various objects just created above.

    // Scene is composed of sky, ground, sea, room and some central models
    objectRoot->add(sky, Scale(2000.0, 2000.0, 2000.0));
    objectRoot->add(ground);
    objectRoot->add(room, Translate(basePoint.x, basePoint.y, basePoint.z));
    objectRoot->add(sea);
    objectRoot->add(central);

    // Central model has a rudimentary animation (constant rotation on Z)
    animated.push_back(anim);

    // Central contains a teapot on a podium and an external sphere of spheres
    central->add(podium, Translate(0.0, 0,0));
    central->add(anim, Translate(0.0, 0,0));
    anim->add(teapot, Translate(0.1, 0.0, 1.5)*TeapotPolygons->modelTr);
    anim->add(spheres, Translate(0.0, 0.0, 0.0)*Scale(30.0, 30.0, 30.0));

    // Room contains two framed pictures
    room->add(leftFrame, Translate(-1.5, 9.85, 1.)*Scale(0.8, 0.8, 0.8));
    room->add(rightFrame, Translate( 1.5, 9.85, 1.)*Scale(0.8, 0.8, 0.8));

    // Schedule first timed animation call
    glutTimerFunc(30, animate, 1);

	//Camera initializations
	wtilt = 45.0f;
	wspin = 0.0f;
	zoom = 150.0f;

	tx = 0.0f;
	ty = 0.0f;

	//Eye Init
	eye = vec3(4.0f, 4.0f, 4.0f);
	speed = 0.000005f;
	gspin = -133.0f;
	gtilt = 0.0f;
	time_since_last_refresh = 0.0f;
	GameCam = false;

	//Lighting Init
	BRDF = true; // false = Phong lighting
	printf("\t\t\t\tBRDF Lighting\n");

	//FBO for Shadow Map
	shadowMap = new FBO();

	shadowMap->CreateFBO(1024, 1024);

	//Create G-Buffer
	G_Buffer = new FBO();

	G_Buffer->CreateFBO(width, height, 4);

    CHECKERROR;
}

////////////////////////////////////////////////////////////////////////
// Procedure DrawScene is called whenever the scene needs to be
// drawn. (Which is often: 30 to 60 times per second are the common
// goals.)
void Scene::DrawScene()
{
	time_since_last_refresh = glutGet((GLenum)GLUT_ELAPSED_TIME);	

    // Calculate the light's position.
    float lPos[4] = {
       basePoint.x+lightDist*cos(lightSpin*rad)*sin(lightTilt*rad), 
       basePoint.y+lightDist*sin(lightSpin*rad)*sin(lightTilt*rad), 
       basePoint.z+lightDist*cos(lightTilt*rad), 
       1.0 };

	//Computing movement of eye
	float step = speed * time_since_last_refresh;

	if(move[0])
		eye += step * vec3(sin(gspin*rad), cos(gspin*rad), 0.0f);
	if(move[1])
		eye -= step * vec3(sin(gspin*rad), cos(gspin*rad), 0.0f);
	if(move[2])
		eye -= step * vec3(cos(gspin*rad), -sin(gspin*rad), 0.0f);
	if(move[3])
		eye += step * vec3(cos(gspin*rad), -sin(gspin*rad), 0.0f);

    // Compute Viewing and Perspective transformations.
    MAT4 WorldProj, WorldView, WorldInverse;

    // FIXME: When you are ready to try interactive viewing, replace
    // the following hardcoded values for WorldProj and WorldView with
    // transformation matrices calculated from variables such as spin, 
    // tilt, tr, basePoint, ry, front, and back.

	WorldProj = Perspective(0.2*width/height, 0.2, 0.1, 1000);

	if(!GameCam)
		WorldView = Translate(tx, ty, -zoom) * Rotate(0, wtilt - 90) * Rotate(2, wspin);
	else
		WorldView = Rotate(0, gtilt - 90)*Rotate(2, gspin)*Translate(-eye.x, -eye.y, -ground->HeightAt(eye.x,eye.y)-2.0f);

	invert(&WorldView, &WorldInverse);

	//Pass 1
	
	int programId;
	int loc;

	// Set the viewport, and clear the screen
	/*glViewport(0, 0, shadowMap->width, shadowMap->height);
	shadowMap->Bind();
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	shadowProgram->Use();
	programId = shadowProgram->programId;

	MAT4 LightLookAt, shadowMatrix, LightProj;
	vec3 lightpos = vec3(lPos[0], lPos[1], lPos[2]);
	LightLookAt = LookAt(lightpos, vec3(0.0f, 0.0f, 0.0f), vec3(0.0f, 1.0f, 0.0f));
	float ry = 50.0f/lightDist;
	LightProj = Perspective(ry* shadowMap->width / shadowMap->height, ry, 0.1, 1000);

	loc = glGetUniformLocation(programId, "WorldProj");
	glUniformMatrix4fv(loc, 1, GL_TRUE, LightProj.Pntr());
	loc = glGetUniformLocation(programId, "WorldView");
	glUniformMatrix4fv(loc, 1, GL_TRUE, LightLookAt.Pntr());

	MAT4 B = Translate(0.5f, 0.5f, 0.5f) *  Scale(0.5f, 0.5f, 0.5f);
	shadowMatrix = B * LightProj * LightLookAt;

	glEnable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	CHECKERROR;
	objectRoot->Draw(shadowProgram, Identity);
	CHECKERROR;
	glDisable(GL_CULL_FACE);

	shadowProgram->Unuse();
	shadowMap->Unbind();
*/
	//Pass 2

    // Use the lighting shader
    lightingProgram->Use();

	// Set the viewport, and clear the screen
	glViewport(0, 0, width, height);
	glClearColor(0.5, 0.5, 0.5, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    programId = lightingProgram->programId;

	//glActiveTexture(GL_TEXTURE2);
	//glBindTexture(GL_TEXTURE_2D, shadowMap->textureID[0]);

    // Setup the perspective and viewing matrices for normal viewing.
    loc = glGetUniformLocation(programId, "WorldProj");
    glUniformMatrix4fv(loc, 1, GL_TRUE, WorldProj.Pntr());
    loc = glGetUniformLocation(programId, "WorldView");
    glUniformMatrix4fv(loc, 1, GL_TRUE, WorldView.Pntr());
    loc = glGetUniformLocation(programId, "WorldInverse");
    glUniformMatrix4fv(loc, 1, GL_TRUE, WorldInverse.Pntr());
	loc = glGetUniformLocation(programId, "ShadowMat");
	//glUniformMatrix4fv(loc, 1, GL_TRUE, shadowMatrix.Pntr());
	loc = glGetUniformLocation(programId, "shadowMap");
	//glUniform1i(loc, 2);
    loc = glGetUniformLocation(programId, "lightPos");
    glUniform3fv(loc, 1, &(lPos[0]));  
    loc = glGetUniformLocation(programId, "mode");
    glUniform1i(loc, mode);  

	vec3 Light;
	if (BRDF)
		Light = vec3(3.2, 3.2, 3.2);
	else
		Light = vec3(1, 1, 1);

	vec3 Ambient(0.2, 0.2, 0.2);

	loc = glGetUniformLocation(programId, "Light");
	glUniform3fv(loc, 1, &(Light[0]));
	loc = glGetUniformLocation(programId, "Ambient");
	glUniform3fv(loc, 1, &(Ambient[0]));
	loc = glGetUniformLocation(programId, "UseBRDF");
	glUniform1i(loc, BRDF);


    CHECKERROR;

    // Compute any continuously animating objects
    for (std::vector<Object*>::iterator m=animated.begin();  m<animated.end();  m++)
        (*m)->animTr = Rotate(2, atime);

    // Draw all objects (hierarchically down through each object's child list).
	
    CHECKERROR; 
    objectRoot->Draw(lightingProgram, Identity);
    CHECKERROR; 
	
    lightingProgram->Unuse();

}
