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
        "**.cpp",
        "shaders/**"
    }

    defines {
	"_CRT_SECURE_NO_WARNINGS",
        "GLFW_INCLUDE_NONE"
    }

    includedirs {
        "src",
	    "%{includeDir.glfw}",
	    "%{includeDir.glm}",
	    "%{includeDir.imgui}",
	    "%{includeDir.stb}",
	    "%{includeDir.fastobj}",
	    "%{includeDir.vulkanSDK}"
    }

    links {
        "glfw",
        "imgui",
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

    local function compile_shaders()
        commands = {}
        for _, file in ipairs(os.matchfiles("shaders/**")) do
            table.insert(commands, "glslangValidator -V -o assets/spv/" .. string.sub(file, string.find(file, "/[^/]*$") + 1) .. ".spv " .. file)
        end
        return commands
    end

    buildmessage "Compiling shaders..."
    buildcommands { compile_shaders() }
    buildoutputs { "assets/spv/**.spv" }
