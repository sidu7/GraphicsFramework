#pragma once

template<typename Class>
class Singleton
{
public:
	static Class* Instance()
	{
		static Class* instance = nullptr;
		if (!instance)
		{
			instance = new Class();
		}

		return instance;
	}

	~Singleton()
	{
		delete Instance();
	}
};

template<typename Class, typename Factory>
class SingletonByFactory
{
public:
	static Class* Instance()
	{
		static Class* instance = nullptr;
		if (!instance)
		{
			instance = Factory::Create();
		}

		return instance;
	}

	~SingletonByFactory()
	{
		delete Instance();
	}
};