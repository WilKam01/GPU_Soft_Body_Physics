workspace "thesis"
    architecture "x86_64"

    configurations {
        "Debug",
        "Release",
        "Distribution"
    }

outputdir = "build/%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"
Vulkan_SDK = os.getenv("VULKAN_SDK")

-- external includes
includeDir = {
    glfw = "%{wks.location}/external/glfw/include",
    glm = "%{wks.location}/external/glm",
    imgui = "%{wks.location}/external/imgui",
    vulkanSDK = "%{Vulkan_SDK}/Include",
}

-- external libraries
libDir = {
    vulkanSDK = "%{Vulkan_SDK}/Lib/vulkan-1.lib"
}

include "project"

group "deps"
	include "external"
group ""