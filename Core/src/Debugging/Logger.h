#pragma once
#if !defined(DISTRIBUTION_CONFIG) || defined(SYSTEM_CONSOLE)

#include <iostream>
#include <any>
#include <vector>
#include <string>
#include <ctime>
#include "Networking/Message.h"

enum class Level
{
	Trace = 0, Debug, Info, Warn, Error
};

class Logger
{
	Logger() = delete;
public:
	static inline Level level = Level::Debug;
	static inline const char* Title = "App";

	template<typename... Args>
	static void Trace(const char* message, Args&&... _args)
	{
		if (level == Level::Trace)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[94m[" << timebuffer << "] " << Title << " TRACE: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void Debug(const char* message, Args&&... _args)
	{
		if (level <= Level::Debug)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[35m[" << timebuffer << "] " << Title << " DEBUG: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void Info(const char* message, Args&&... _args)
	{
		if (level <= Level::Info)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[32m[" << timebuffer << "] " << Title << " INFO: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void Warn(const char* message, Args&&... _args)
	{
		if (level <= Level::Warn)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[33m[" << timebuffer << "] " << Title << " WARNING: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void Error(const char* message, Args&&... _args)
	{
		if (level <= Level::Error)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[31m[" << timebuffer << "] " << Title << " ERROR: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void TraceCore(const char* message, Args&&... _args)
	{
		if (level == Level::Trace)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[94m[" << timebuffer << "] CORE TRACE: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void DebugCore(const char* message, Args&&... _args)
	{
		if (level <= Level::Debug)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[35m[" << timebuffer << "] CORE DEBUG: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void InfoCore(const char* message, Args&&... _args)
	{
		if (level <= Level::Info)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[32m[" << timebuffer << "] CORE INFO: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void WarnCore(const char* message, Args&&... _args)
	{
		if (level <= Level::Warn)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[33m[" << timebuffer << "] CORE WARNING: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}

	template<typename... Args>
	static void ErrorCore(const char* message, Args&&... _args)
	{
		if (level <= Level::Error)
		{
			std::vector<std::any> args = { _args... };

			GetLoggerTime();
			std::cout << "\033[31m[" << timebuffer << "] CORE ERROR: ";
			while (*message)
			{
				if (*message == '{' && *(message + 2) == '}')
				{
					int i = *(message + 1) - 48;
					printArg(args, i);
					message += 3;
				}
				else
				{
					std::cout << *message;
					message++;
				}
			}

			std::cout << std::endl;
		}
	}
private:
	static inline void printArg(std::vector<std::any>& args, int& index)
	{
		if (args[index].type() == typeid(const char*))
			std::cout << any_cast<const char*>(args[index]);
		else if (args[index].type() == typeid(char*))
			std::cout << any_cast<char*>(args[index]);
		else if (args[index].type() == typeid(int))
			std::cout << any_cast<int>(args[index]);
		else if (args[index].type() == typeid(uint32_t))
			std::cout << any_cast<uint32_t>(args[index]);
		else if (args[index].type() == typeid(size_t))
			std::cout << any_cast<size_t>(args[index]);
		else if (args[index].type() == typeid(float))
			std::cout << any_cast<float>(args[index]);
		else if (args[index].type() == typeid(double))
			std::cout << any_cast<double>(args[index]);
		else if (args[index].type() == typeid(bool))
			std::cout << any_cast<bool>(args[index]);
		else if (args[index].type() == typeid(std::string))
			std::cout << any_cast<std::string>(args[index]);
		else if (args[index].type() == typeid(uint_least16_t))
			std::cout << any_cast<uint_least16_t>(args[index]);
		else
			std::cout << "Argument at position: " << index << " not supported!" << std::endl;
	}

	static inline void GetLoggerTime()
	{
		std::time_t current_time = std::time(0);
		std::tm* timestamp = std::localtime(&current_time);
		strftime(timebuffer, 80, "%T", timestamp);
	}

	static inline char timebuffer[80];
};

#endif