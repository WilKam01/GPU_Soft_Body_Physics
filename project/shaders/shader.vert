#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
} ubo;

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec3 worldPos;
layout(location = 1) out vec3 worldNormal;
layout(location = 2) out vec2 uvCoord;

void main() 
{
    gl_Position = ubo.viewProj * vec4(position.xyz, 1.0);
    worldPos = position.xyz;
	worldNormal = normal;
	uvCoord = uv;
}