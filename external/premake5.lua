project "glfw"
	location "glfw"
	kind "StaticLib"
	language "C"

	targetdir("%{wks.location}/" .. outputdir .. "/bin")
    objdir("%{wks.location}/" .. outputdir .. "/int")

	files
	{
		"glfw/include/GLFW/glfw3.h",
		"glfw/include/GLFW/glfw3native.h",
		"glfw/src/internal.h",
		"glfw/src/mappings.h",
		"glfw/src/null_joystick.h",
		"glfw/src/null_platform.h",
		"glfw/src/platform.h",
		"glfw/src/context.c",
		"glfw/src/init.c",
		"glfw/src/input.c",
		"glfw/src/monitor.c",
		"glfw/src/null_init.c",
		"glfw/src/null_joystick.c",
		"glfw/src/null_monitor.c",
		"glfw/src/null_window.c",
		"glfw/src/platform.c",
		"glfw/src/vulkan.c",
		"glfw/src/window.c"
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

		files
		{
			"glfw/src/win32_init.c",
			"glfw/src/win32_joystick.c",
			"glfw/src/win32_module.c",
			"glfw/src/win32_monitor.c",
			"glfw/src/win32_thread.c",
			"glfw/src/win32_time.c",
			"glfw/src/win32_window.c",
			"glfw/src/wgl_context.c",
			"glfw/src/egl_context.c",
			"glfw/src/osmesa_context.c"
		}

		defines 
		{ 
			"_GLFW_WIN32",
			"_CRT_SECURE_NO_WARNINGS"
		}
