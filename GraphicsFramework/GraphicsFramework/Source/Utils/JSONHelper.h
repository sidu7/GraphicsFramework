#pragma once

#include "Core/Core.h"

#ifdef _MSC_VER   
#undef GetObject
#endif


class JSONHelper
{
public:

	static auto ParseFile(const std::string& path)
	{
		std::ifstream file(path); 
		std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>()); 
		file.close(); 
		rapidjson::Document root; 
		root.Parse(contents.c_str()); 
		if (!root.IsObject()) { std::cout << "Error reading JSON file" << path << std::endl; }
		return root;
	}
	
	//JSON parsing
	 static glm::vec3 GetVec3F(const rapidjson::Value::Array& arr)
	{
		return glm::vec3(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat());
	}

	 static glm::vec2 GetVec2F(const rapidjson::Value::Array& arr)
	{
		return glm::vec2(arr[0].GetFloat(), arr[1].GetFloat());
	}

	 static glm::vec4 GetVec4F(const rapidjson::Value::Array& arr)
	{
		return glm::vec4(arr[0].GetFloat(), arr[1].GetFloat(), arr[2].GetFloat(), arr[3].GetFloat());
	}

	 static glm::ivec2 GetIVec2(const rapidjson::Value::Array& arr)
	{
		return glm::ivec2(arr[0].GetInt(), arr[1].GetInt());
	}

	template<typename T>
	static std::vector<T> GetArray(const rapidjson::Value::Array& arr)
	{
		std::vector<T> data;
		data.reserve(arr.Size());
		for (unsigned int i = 0; i < arr.Size(); ++i)
		{
			data.emplace_back(static_cast<T>(arr[i].GetUint()));
		}
		return data;
	}

	template<>
	 static std::vector<std::string> GetArray(const rapidjson::Value::Array& arr)
	{
		std::vector<std::string> data;
		data.reserve(arr.Size());
		for (unsigned int i = 0; i < arr.Size(); ++i)
		{
			data.emplace_back(arr[i].GetString());
		}
		return data;
	}

	template<>
	 static std::vector<unsigned int> GetArray(const rapidjson::Value::Array& arr)
	{
		std::vector<unsigned int> data;
		data.reserve(arr.Size());
		for (unsigned int i = 0; i < arr.Size(); ++i)
		{
			data.emplace_back(arr[i].GetUint());
		}
		return data;
	}

	//JSON deserialization
	template<typename T>
	static void Write(const char* key, const T& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		std::cout << "No specialized template for this value type" << std::endl;
	}

	template<>
	 static void Write(const char* key, const int& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.Int(value);
	}

	template<>
	 static void Write(const char* key, const unsigned int& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.Uint(value);
	}

	template<>
	 static void Write(const char* key, const bool& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.Bool(value);
	}

	template<>
	 static void Write(const char* key, const float& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.Double(value);
	}

	template<>
	 static void Write(const char* key, const double& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.Double(value);
	}

	template<>
	 static void Write(const char* key, const std::string& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.String(value.c_str());
	}

	template<>
	 static void Write(const char* key, const glm::vec4& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.Double(value.z);
		writer.Double(value.w);
		writer.EndArray();
	}

	template<>
	 static void Write(const char* key, const glm::vec3& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.Double(value.z);
		writer.EndArray();
	}

	template<>
	 static void Write(const char* key, const glm::vec2& value, rapidjson::Writer<rapidjson::StringBuffer>& writer)
	{
		writer.Key(key);
		writer.StartArray();
		writer.Double(value.x);
		writer.Double(value.y);
		writer.EndArray();
	}
};