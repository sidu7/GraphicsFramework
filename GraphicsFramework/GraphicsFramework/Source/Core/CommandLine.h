#pragma once

#include "Core/Singleton.h"
#include <string>
#include <vector>

class CommandLine : public Singleton<CommandLine>
{
public:

	void Init(const std::string& InCmdArgs);
	void Init(int ArgsCount, char** Args);

	inline const std::string& Get() { return CmdStr; }
	inline std::vector<std::string>& GetArgs() { return CmdArgs; }

	bool HasParam(const std::string& param);

	bool GetValue(const std::string& param, float& outValue);
	bool GetValue(const std::string& param, int32_t& outValue);
	bool GetValue(const std::string& param, std::string& outValue);

protected:
	std::string SanitizeParam(const std::string& param);
	std::string SanitizeValueParam(const std::string& param);

	std::string CmdStr;
	std::vector<std::string> CmdArgs;
};