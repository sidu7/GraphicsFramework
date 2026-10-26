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
