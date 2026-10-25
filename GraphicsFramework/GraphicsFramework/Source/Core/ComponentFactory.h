#pragma once
#include "Core/Singleton.h"
#include <functional>
#include <unordered_map>
#include <string>
#include <typeindex>

class Component;

class ComponentFactory : public Singleton<ComponentFactory>
{
public:
    using CreateFn = std::function<Component* ()>;

    // Initialize all framework component types
    void Init();

    // Register a component type by string name (idempotent)
    void Register(const std::string& name, CreateFn factory, std::type_index typeInfo);

    // Create a new component instance by string name
    Component* Create(const std::string& name);

    // Check if a type is already registered
    bool IsRegistered(const std::string& name) const;

    // Get type index by name
    std::type_index GetTypeInfo(const std::string& name) const;

    // Get registered component count
    size_t GetCount() const;

private:
    std::unordered_map<std::string, CreateFn> mFactories;
    std::unordered_map<std::string, std::type_index> mTypeMap;
};
