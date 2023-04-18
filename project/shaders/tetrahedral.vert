#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
} ubo;

layout(std140, set = 1, binding = 0) readonly buffer PositionSSBO
{
	vec4 positions[];
};
layout(std140, set = 1, binding = 1) readonly buffer TetrahedralSSBO
{
	uvec4 tetrahedralIDs[];
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

    vec4 pos = positions[tetrahedralIDs[gl_InstanceIndex][indices[gl_VertexIndex]]];
    gl_Position = ubo.viewProj * vec4(pos.xyz, 1.0);
    fragColor = vec3(1.0);
}