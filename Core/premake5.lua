project "Core"
	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	staticruntime "off"

	targetdir (outputdir .. "$(Configuration)/$(ProjectName)")
	objdir (intoutputdir .. "$(Configuration)/$(ProjectName)")

	pchheader "pch.h"
	pchsource "src/pch.cpp"

	files
	{
		"src/**.h",
		"src/**.cpp",
	}

	includedirs
	{
		"src",
		"$(SolutionDir)vendor/glfw/include",
		"$(SolutionDir)vendor/asio/asio/include",
		"$(SolutionDir)vendor/mysql connector/include",
		"$(SolutionDir)vendor/imgui/src",
	}

	links
	{
		"opengl32.lib",
		"Ws2_32.lib",
		"Crypt32.lib",
		"GLFW",
		"ImGui",
	}

	defines { "CORE", "STATIC_CONCPP" }

	filter { "system:windows", "configurations:Debug" }
		links "$(SolutionDir)vendor/mysql connector/bin/debug/lib64/debug/vs14/mysqlcppconn-static.lib"

	filter { "system:windows", "configurations:Release or Distribution" }
		links "$(SolutionDir)vendor/mysql connector/bin/release/bin/lib64/vs14/mysqlcppconn-static.lib"

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