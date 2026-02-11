#pragma once
#include <cstdint>
#include <string>
#include <format>
#include <istream>

namespace Logger
{
	void Log(const std::string& message);

	// ログファイルを書き出す
	void Log(std::ostream& os, const std::string& message);
};

