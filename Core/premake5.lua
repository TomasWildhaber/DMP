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
		"$(SolutionDir)vendor/imgui/src",
    }

	links
	{
		"opengl32.lib",
		"GLFW",
		"ImGui",
	}

	defines { "CORE" }

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