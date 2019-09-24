////////////////////////////////////////////////////////////////////////
// A lightweight class representing an instance of an object that can
// be drawn onscreen.  An Object consists of a shape (batch of
// triangles), and various transformation, color and texture
// parameters.  It also contains a list of child objects to be drawn
// in a hierarchical fashion under the control of parent's
// transformations.
//
// Methods consist of a constructor, and a Draw procedure, and an
// append for building hierarchies of objects.

#include "math.h"
#include <fstream>
#include <stdlib.h>

#include <glbinding/gl/gl.h>
#include <glbinding/Binding.h>
using namespace gl;

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#include "shader.h"
#include "shapes.h"
#include "scene.h"
#include "transform.h"

#include <glu.h>                // For gluErrorString
#define CHECKERROR {GLenum err = glGetError(); if (err != GL_NO_ERROR) { fprintf(stderr, "OpenGL error (at line object.cpp:%d): %s\n", __LINE__, gluErrorString(err)); exit(-1);} }


Object::Object(Shape* _shape, const int _objectId,
               const vec3 _diffuseColor, const vec3 _specularColor, const float _shininess)
    : diffuseColor(_diffuseColor), specularColor(_specularColor), shininess(_shininess),
      shape(_shape), objectId(_objectId)
     
{}


void Object::Draw(ShaderProgram* program, MAT4& objectTr)
{
    // Inform the shader of the surface values Kd, Ks, and alpha.
    int loc = glGetUniformLocation(program->programId, "diffuse");
    glUniform3fv(loc, 1, &diffuseColor[0]);

    loc = glGetUniformLocation(program->programId, "specular");
    glUniform3fv(loc, 1, &specularColor[0]);

    loc = glGetUniformLocation(program->programId, "shininess");
    glUniform1f(loc, shininess);

    // Inform the shader of which object is being drawn so it can make
    // object specific decisions.
    loc = glGetUniformLocation(program->programId, "objectId");
    glUniform1i(loc, objectId);

    // Inform the shader of this objects model transformation.  Oddly,
    // the inverse of the model transformation is needed transforming
    // normals.
    loc = glGetUniformLocation(program->programId, "ModelTr");
    glUniformMatrix4fv(loc, 1, GL_TRUE, objectTr.Pntr());
    
    MAT4 inv;
    invert(&objectTr, &inv);
    loc = glGetUniformLocation(program->programId, "NormalTr");
    glUniformMatrix4fv(loc, 1, GL_TRUE, inv.Pntr());

    // If this oject has an associated texture, this is the place to
    // load the texture into a texture-unit of your choice and inform
    // the shader program of the texture-unit number.  See
    // Texture::Bind for the 4 lines of code to do exactly that.
    

    // Draw this object
    CHECKERROR;
    if (shape) shape->DrawVAO();
    CHECKERROR;


    // Recursively draw each sub-objects, each with its own transformation.
    for (int i=0;  i<instances.size();  i++) {
        MAT4 itr = objectTr*instances[i].second*animTr;
        instances[i].first->Draw(program, itr); }
    
    CHECKERROR;
}
