project "Core"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
	staticruntime "on"

    targetdir (outputdir .. "$(Configuration)/$(ProjectName)")
	objdir (intoutputdir .. "$(Configuration)/$(ProjectName)")

    files
	{
		"src/**.h",
		"src/**.cpp",
	}

    includedirs
    {
        "src",
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