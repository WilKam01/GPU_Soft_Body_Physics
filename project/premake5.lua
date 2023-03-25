project "project"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir("%{wks.location}/" .. outputdir .. "/bin")
    objdir("%{wks.location}/" .. outputdir .. "/int")

    pchheader "pch.h"
    pchsource "src/pch.cpp"

    files {
        "**.h",
        "**.cpp"
    }

    defines {
        "GLFW_INCLUDE_NONE"
    }

    includedirs {
        "src",
	    "%{includeDir.glfw}",
	    "%{includeDir.vulkanSDK}"
    }

    links {
        "glfw",
        "%{libDir.vulkanSDK}"
    }

    filter "configurations:Debug"
        defines "DEBUG"
        symbols "on"

    filter "configurations:Release"
        defines "RELEASE"
        optimize "on"

    filter "configurations:Distribution"
        defines "DIST"
        optimize "on"

    filter "system:windows"
        systemversion "latest"
        defines "WIN32"
