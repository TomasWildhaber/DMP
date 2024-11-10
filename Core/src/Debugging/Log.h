#pragma once
#include "Logger.h"

#ifndef PLATFORM_WINDOWS
	#error No other systems supported yet
#endif

#if !defined(DISTRIBUTION_CONFIG) || defined(SYSTEM_CONSOLE)
	#define SetLoggerTitle(title) { Logger::Title = title; }

	#ifdef CORE
		#define TRACE(...)	Logger::TraceCore(__VA_ARGS__)
		#define DEBUG(...)	Logger::DebugCore(__VA_ARGS__)
		#define INFO(...)	Logger::InfoCore(__VA_ARGS__)
		#define WARN(...)	Logger::WarnCore(__VA_ARGS__)
		#define ERROR(...)	Logger::ErrorCore(__VA_ARGS__)
	#else
		#define TRACE(...)	Logger::Trace(__VA_ARGS__)
		#define DEBUG(...)	Logger::Debug(__VA_ARGS__)
		#define INFO(...)	Logger::Info(__VA_ARGS__)
		#define WARN(...)	Logger::Warn(__VA_ARGS__)
		#define ERROR(...)	Logger::Error(__VA_ARGS__)
	#endif
#else
	#define SetLoggerTitle(title)

	#define TRACE(...)
	#define DEBUG(...)
	#define INFO(...)
	#define WARN(...)
	#define ERROR(...)
#endif

#ifndef DISTRIBUTION_CONFIG
	#ifdef PLATFORM_WINDOWS
		#define DEBUGBREAK() __debugbreak();
	#endif

	#define ASSERT(con, msg, ...) { if (con) { ERROR(msg, __VA_ARGS__); DEBUGBREAK(); } }
	#define ASSERT(con, msg) { if (con) { ERROR(msg); DEBUGBREAK(); } }
#else
	#define ASSERT(con, msg, ...) con
	#define ASSERT(con, msg) con
	#define DEBUGBREAK()
#endif