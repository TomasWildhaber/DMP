project "Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
	staticruntime "on"

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
		"$(SolutionDir)vendor/GameNetworkingSockets/include",
		"$(SolutionDir)vendor/imgui/src",
    }

	links
	{
		"opengl32.lib",
		"GLFW",
		"ImGui",
	}

	defines { "CORE" }

	filter { "platforms:windows", "configurations:Debug" }
		links "vendor/GameNetworkingSockets/bin/Windows/Debug/GameNetworkingSockets_s.lib"

	filter { "platforms:windows", "configurations:Release" }
		links "vendor/GameNetworkingSockets/bin/Windows/Release/GameNetworkingSockets_s.lib"

	filter { "platforms:windows", "configurations:Distribution" }
		links "vendor/GameNetworkingSockets/bin/Windows/Distribution/GameNetworkingSockets_s.lib"

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