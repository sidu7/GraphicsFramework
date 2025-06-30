#include "CommandLine.h"

#include "Utils/FileHelper.h"

void CommandLine::Init(const std::string& InCmdArgs)
{
	CmdStr = InCmdArgs;

	CmdArgs = FileHelper::SplitString(CmdStr, ' ');
}

void CommandLine::Init(int ArgsCount, char** Args)
{
	for (int32_t i = 1; i < ArgsCount; ++i)
	{
		CmdStr += Args[i] + std::string(" ");
		CmdArgs.push_back(Args[i]);
	}
}

bool CommandLine::HasParam(const std::string& param)
{
	std::string Sparam = SanitizeParam(param);
	return std::find(CmdArgs.begin(), CmdArgs.end(), Sparam) != CmdArgs.end();
}

bool CommandLine::GetValue(const std::string& param, float& outValue)
{
	std::string Sparam = SanitizeValueParam(param);
	size_t FoundIdx = CmdStr.find(Sparam);
	size_t Start = FoundIdx + Sparam.size();
	size_t End = CmdStr.find_first_of(' ', FoundIdx);
	outValue = std::atof(CmdStr.substr(Start, End - Start).c_str());

	return FoundIdx != CmdStr.size();
}

bool CommandLine::GetValue(const std::string& param, int32_t& outValue)
{
	std::string Sparam = SanitizeValueParam(param);
	size_t FoundIdx = CmdStr.find(Sparam);
	size_t Start = FoundIdx + Sparam.size();
	size_t End = CmdStr.find_first_of(' ', FoundIdx);
	outValue = std::atoi(CmdStr.substr(Start, End - Start).c_str());

	return FoundIdx != CmdStr.size();
}

bool CommandLine::GetValue(const std::string& param, std::string& outValue)
{
	std::string Sparam = SanitizeValueParam(param);
	size_t FoundIdx = CmdStr.find(Sparam);
	size_t Start = FoundIdx + Sparam.size();
	size_t End = CmdStr.find_first_of(' ', FoundIdx);
	outValue = CmdStr.substr(Start, End - Start);

	return FoundIdx != CmdStr.size();
}

std::string CommandLine::SanitizeParam(const std::string& param)
{
	std::string Sparam = param;
	if (!Sparam.empty() && Sparam[0] != '-')
	{
		Sparam = '-' + param;
	}

	return Sparam;
}

std::string CommandLine::SanitizeValueParam(const std::string& param)
{
	std::string Sparam = param;
	if (!Sparam.empty())
	{
		if (Sparam[0] != '-')
		{
			Sparam = '-' + Sparam;
		}
		if (Sparam[Sparam.size() - 1] != '=')
		{
			Sparam = Sparam + '=';
		}
	}

	return Sparam;
}