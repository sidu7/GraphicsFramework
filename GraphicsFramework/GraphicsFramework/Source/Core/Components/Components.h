#pragma once
#include <rapidjson/document.h>
#include <glm/glm.hpp>
#include <typeindex>
#include <string>

class Object;
class ComponentFactory;

class Component
{
    friend class ComponentFactory;
    friend class Object;

public:
    Component() = default;
    virtual ~Component() = default;

    // Lifecycle — called after engine is ready
    virtual void Init(Object* owner) {}
    virtual void Destroy() {}
    virtual void Update() {}

    // Serialization (reads JSON into component data)
    virtual void Serialize(rapidjson::Value::Object data) = 0;

    // Accessors
    Object* GetOwner() const { return mOwner; }
    std::type_index GetTypeInfo() const { return mTypeIndex; }
    std::type_index GetIndex() { return mTypeIndex; }

protected:
    void SetTypeInfo(std::type_index typeInfo) { mTypeIndex = typeInfo; }

    Object*  mOwner = nullptr;
    std::type_index mTypeIndex{ std::type_index(typeid(void)) };
};
