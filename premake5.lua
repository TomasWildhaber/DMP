workspace "DMP"
	architecture "x64"
	platforms { "Win64" }

	startproject "Client"

	configurations
	{
		"Debug",
		"Release",
		"Distribution"
	}

	flags
	{
		"MultiProcessorCompile"
	}

	outputdir = "$(SolutionDir)bin/"
	intoutputdir = "$(SolutionDir)bin-int/"

	defines { "_CRT_SECURE_NO_WARNINGS" }

	filter "system:windows"
		defines { "PLATFORM_WINDOWS" }

	filter "configurations:Debug"
			defines "DEBUG_CONFIG"
			runtime "Debug"
			symbols "on"

	filter "configurations:Release"
		defines "RELEASE_CONFIG"
		runtime "Release"
		optimize "on"

	filter "configurations:Distribution"
		defines "DISTRIBUTION_CONFIG"
		runtime "Release"
		optimize "on"

group "vendor"
	include "vendor/glfw"
	include "vendor/imgui"
group ""

include "Core"
include "Server"
include "Client"