#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
    vec3 camPos;
} ubo;

struct Particle
{
    vec3 position;
    vec3 velocity;
};

layout(std140, set = 1, binding = 0) readonly buffer ParticlesSSBO
{
	Particle particles[];
};

struct Tetrahedral
{
    uvec4 indices;
    float restVolume;
};

layout(std140, set = 1, binding = 1) readonly buffer TetrahedralSSBO
{
	Tetrahedral tetrahedrals[];
};

layout(location = 0) out vec3 fragColor;

void main() 
{
    const uint indices[12] = { 
        2, 1, 0,
        0, 1, 3,
        1, 2, 3,
        2, 0, 3 
    };

    vec3 pos = particles[tetrahedrals[gl_InstanceIndex].indices[indices[gl_VertexIndex]]].position.xyz;
    gl_Position = ubo.viewProj * vec4(pos, 1.0);
    fragColor = vec3(1.0);
}