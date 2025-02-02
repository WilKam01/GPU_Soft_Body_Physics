# Real-time Soft Body Simulation using Extended Position-Based Dynamics and Tetrahedral Deformation
Implementation of my [Bachelor Thesis](https://bth.diva-portal.org/smash/get/diva2:1776319/FULLTEXT02.pdf) with the purpose of implementing soft body physics on the GPU using vulkan. The implemention of Extended Position-Based Dynamics (XPBD) uses a tetrahedral mesh version of the rendered triangle mesh to simulate physical constraints on the GPU which the transformation is later applied to the rendered mesh. The implementation supports tetrahedral mesh representations of differing resolutions paired with tetrahedral deformation to uniformly apply the transformation on the higher resolution mesh. 

![Demo](demo.gif)

## Features
* Multiple active soft bodies
* Varying resolution of tetrahedral models, transforms using tetrahedral deformation
* Movable and rotatable camera

## Assets
* [Stanford Bunny](https://graphics.stanford.edu/data/3Dscanrep/) model
* [Dragon](https://graphics.stanford.edu/data/3Dscanrep/) model
* [Armadillo](https://graphics.stanford.edu/data/3Dscanrep/) model
* [Blender Tetrahedralizer Plugin](https://github.com/matthias-research/pages/blob/master/tenMinutePhysics/BlenderTetPlugin.py): Offline creation of tetrahedral models

## Libraries
* [fast_obj](https://github.com/thisistherk/fast_obj/tree/85778da5fc320b7e52885f8e869edc079695cc79): Importation of .obj files
* [glfw](https://github.com/glfw/glfw/tree/9a87635686c7fcb63ca63149c5b179b85a53a725): Multi platform OpenGl/Vulkan API used for creation of windows and input
* [glm](https://github.com/g-truc/glm/tree/efec5db081e3aad807d0731e172ac597f6a39447): Heador only mathematics library based on glsl (vectors and matrices)
* [Dear ImGui](https://github.com/ocornut/imgui/tree/bf87fbcbcc8ffb8ec70a447dfdccadfe6eefe2c2): Immediate UI rendering library
* [Premake](https://premake.github.io): Build system
* [stb](https://github.com/nothings/stb/tree/5736b15f7ea0ffb08dd38af21067c314d6a3aae9): Import/Export of images

## Building
Visual Studio 2022 is recommended to build this project, as it has only been tested using the built in compiler on the windows operating system. Before following the steps below, it is required for the user to have downloaded the [Vulkan SDK](https://vulkan.lunarg.com) beforehand.

**1 Downloading dependencies:**

Git submodules are used for the external libraries, start by cloning with the `--recursive` flag: `git clone --recursive https://github.com/WilKam01/GPU_Soft_Body_Physics.git`

The command: `git submodule update --init` can also be used if the cloning was done without the `--recursive` flag 

**2 Create `.sln` file:**

Use the following command in a terminal: `./external/premake/premake5.exe vs2022` to create the project files

**3 Build in Visual Studio**

Start Visual Studio and build and run the project
