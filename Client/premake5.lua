project "Client"
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
        "$(SolutionDir)Core/src",
        "$(SolutionDir)vendor/imgui/src",
        "$(SolutionDir)vendor/GameNetworkingSockets/include",
    }

    links
    {
        "Core",
        "ImGui",
    }

    filter "configurations:Debug"
        kind "ConsoleApp"
		defines "DEBUG_CONFIG"
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
        kind "ConsoleApp"
		defines "RELEASE_CONFIG"
		runtime "Release"
        optimize "on"

    filter "configurations:Distribution"
        kind "WindowedApp"
		defines "DISTRIBUTION_CONFIG"
		runtime "Release"
        optimize "on"