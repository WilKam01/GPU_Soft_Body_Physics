#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
    vec3 camPos;
} ubo;

struct Particle
{
    vec4 position;
    vec4 velocity;
};

layout(std140, set = 2, binding = 0) readonly buffer ParticlesSSBO
{
	Particle particles[];
};
layout(std140, set = 2, binding = 1) readonly buffer TetrahedralSSBO
{
	uvec4 tetrahedralIDs[];
};

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 uvCoord;

void main() 
{
    const uint indices[12] = { 
        2, 1, 0,
        0, 1, 3,
        1, 2, 3,
        2, 0, 3 
    };

    // "Excluded" point for each triangle
    const uint excluded[4] = { 3, 2, 0, 1 };

    uint index = indices[gl_VertexIndex];
    uint triNum = int(gl_VertexIndex / 3) * 3;
    vec3 pos = particles[tetrahedralIDs[gl_InstanceIndex][index]].position.xyz;

    gl_Position = ubo.viewProj * vec4(pos, 1.0);
    worldPos = pos;
    worldNormal = -normalize(pos - ubo.camPos);
//    worldNormal = normalize(cross(
//        particles[tetrahedralIDs[gl_InstanceIndex][triNum + 1]].position.xyz - particles[tetrahedralIDs[gl_InstanceIndex][triNum]].position.xyz,
//        particles[tetrahedralIDs[gl_InstanceIndex][triNum + 2]].position.xyz - particles[tetrahedralIDs[gl_InstanceIndex][triNum]].position.xyz)
//    );
//    bool positive = dot(
//        particles[tetrahedralIDs[gl_InstanceIndex][triNum]].position.xyz - 
//        particles[tetrahedralIDs[gl_InstanceIndex][excluded[triNum / 3]]].position.xyz,
//        worldNormal
//    ) > 0.0;
//    worldNormal = worldNormal * (int(positive) * 2 - 1);
    uvCoord = vec2(0.0, 0.0);
}