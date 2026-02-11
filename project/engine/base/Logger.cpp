#include "Logger.h"
#include <Windows.h>


namespace Logger 
{
	void Log(const std::string& message) {
		OutputDebugStringA(message.c_str());
	}

	// ログファイルを書き出す
	void Log(std::ostream& os, const std::string& message) {
		os << message << std::endl;
		OutputDebugStringA(message.c_str());
	}
}