#include "Object.h"
#include "JSONHelper.h"
#include "ShapeManager.h"
#include "../opengl/Shader.h"
#include "Components/Components.h"
#include "../opengl/Renderer.h"
#include "../opengl/Texture.h"
#include "Components/Transform.h"
#include "Components/Material.h"
#include "Components/RotateInCircle.h"
#include "Components/Move.h"
#include "Components/Shape.h"

Object::Object()
{
}

Object::~Object()
{
	for(auto index : mIndices)
	{
		delete mComponents[index];
	}
}

void Object::Update(Shader* shader)
{
	for(auto index : mIndices)
	{
		mComponents[index]->Update(shader);
	}

	// Draw calls
	Shape* shape = GetComponent<Shape>();
	if (shape && glIsEnabled(GL_CULL_FACE))
	{
		if (shape->mShape == Shapes::QUAD)
		{
			glCullFace(GL_BACK);
		}
		else
		{
			glCullFace(GL_FRONT);
		}
	}
	Renderer::Instance().Draw(*shape->mShapeData.first, *shape->mShapeData.second, *shader);
	Material* material = GetComponent<Material>();
	if(material && material->pTexture)
	{
		material->pTexture->Unbind(8);
	}
}

void Object::Serialize(const rapidjson::Value::Object& data)
{
	rapidjson::Value::Array components = data["Components"].GetArray();
	for (unsigned int i = 0; i < components.Size(); ++i)
	{
		ComponentType type = (ComponentType)components[i]["Type"].GetUint();
		Component* component = nullptr;
		switch (type)
		{
		case TRANSFORM:
			component = Component::New<Transform>();
			component->Serialize(components[i].GetObject());
			component->mOwner = this;
			AddComponent<Transform>(component);
			break;
		case MATERIAL:
			component = Component::New<Material>();
			component->Serialize(components[i].GetObject());
			component->mOwner = this;
			AddComponent<Material>(component);
			break;
		case SHAPE:
			component = Component::New<Shape>();
			component->Serialize(components[i].GetObject());
			component->mOwner = this;
			AddComponent<Shape>(component);
			break;
		case ROTATEINCIRCLE:
			component = Component::New<RotateInCircle>();
			component->Serialize(components[i].GetObject());
			component->mOwner = this;
			AddComponent<RotateInCircle>(component);
			break;
		case MOVE:
			component = Component::New<Move>();
			component->Serialize(components[i].GetObject());
			component->mOwner = this;
			AddComponent<Move>(component);
			break;
		}
	}
}
