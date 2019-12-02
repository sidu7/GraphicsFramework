#include "Object.h"
#include "JSONHelper.h"
#include "ShapeManager.h"
#include "../opengl/Shader.h"
#include "Components.h"
#include "../opengl/Renderer.h"

Object::Object() : pTransform(nullptr), pMaterial(nullptr), pShape(nullptr), pRotate(nullptr), pMove(nullptr)
{
}

Object::~Object()
{
	delete pTransform;
	delete pMaterial;
	delete pShape;
	delete pRotate;
	delete pMove;
}

void Object::Update(Shader* shader)
{
	if (pRotate) pRotate->Update();
	if (pMove) pMove->Update();
	if (pMaterial) {
		pMaterial->Update(shader);
		if (pMaterial->pTexture)
		{
			pMaterial->pTexture->Bind(8);
			shader->SetUniform1i("texDiff", 8);
		}
	}
	if (pTransform) pTransform->Update(shader);
	if (glIsEnabled(GL_CULL_FACE))
	{
		if (pShape->mShape == Shapes::QUAD)
		{
			glCullFace(GL_BACK);
		}
		else
		{
			glCullFace(GL_FRONT);
		}
	}
	Renderer::Instance().Draw(*pShape->mShapeData.first, *pShape->mShapeData.second, *shader);
	if (pMaterial && pMaterial->pTexture)
	{
		pMaterial->pTexture->Unbind(8);
	}
}

void Object::Serialize(const rapidjson::Value::Object& data)
{
	rapidjson::Value::Array components = data["Components"].GetArray();
	for (unsigned int i = 0; i < components.Size(); ++i)
	{
		ComponentType type = (ComponentType)components[i]["Type"].GetUint();
		switch (type)
		{
		case TRANSFORM:
			pTransform = new Transform();
			pTransform->Serialize(components[i].GetObject());
			pTransform->mOwner = this;
			break;
		case MATERIAL:
			pMaterial = new Material();
			pMaterial->Serialize(components[i].GetObject());
			pMaterial->mOwner = this;
			break;
		case SHAPE:
			pShape = new Shape();
			pShape->Serialize(components[i].GetObject());
			pShape->mOwner = this;
			break;
		case ROTATEINCIRCLE:
			pRotate = new RotateInCircle();
			pRotate->Serialize(components[i].GetObject());
			pRotate->mOwner = this;
			break;
		case MOVE:
			pMove = new Move();
			pMove->Serialize(components[i].GetObject());
			pMove->mOwner = this;
			break;
		}
	}
}
