#include "ComponentFactory.h"
#include <iostream>
#include "Components/Transform.h"
#include "Components/Shape.h"
#include "Components/Material.h"
#include "Components/Move.h"
#include "Components/RotateInCircle.h"

void ComponentFactory::Init()
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    Register("Transform",
        []() { return new Transform(); },
        std::type_index(typeid(Transform)));
    Register("Shape",
        []() { return new Shape(); },
        std::type_index(typeid(Shape)));
    Register("Material",
        []() { return new Material(); },
        std::type_index(typeid(Material)));
    Register("Move",
        []() { return new Move(); },
        std::type_index(typeid(Move)));
    Register("RotateInCircle",
        []() { return new RotateInCircle(); },
        std::type_index(typeid(RotateInCircle)));
}

void ComponentFactory::Register(const std::string& name, CreateFn factory, std::type_index typeInfo)
{
    // Idempotent: skip if already registered
    if (mFactories.find(name) != mFactories.end()) return;
    mFactories[name] = std::move(factory);
    mTypeMap.emplace(name, typeInfo);
}

Component* ComponentFactory::Create(const std::string& name)
{
    auto it = mFactories.find(name);
    if (it == mFactories.end())
    {
		std::cout << "ComponentFactory: Component type '" << name << "' is not registered." << std::endl;
        return nullptr;
    }
    auto typeIt = mTypeMap.find(name);
    Component* comp = it->second();
    // Set type info so Object can look it up by type_index
    if (comp && typeIt != mTypeMap.end())
        comp->SetTypeInfo(typeIt->second);
    return comp;
}

bool ComponentFactory::IsRegistered(const std::string& name) const
{
    return mFactories.find(name) != mFactories.end();
}

std::type_index ComponentFactory::GetTypeInfo(const std::string& name) const
{
    auto it = mTypeMap.find(name);
    return it != mTypeMap.end() ? it->second : std::type_index(typeid(void));
}

size_t ComponentFactory::GetCount() const
{
    return mFactories.size();
}
