#version 450

layout(set = 0, binding = 0) uniform Matrices {
    mat4 model;
    mat4 viewProj;
} ubo;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 uv;

layout(location = 0) out vec2 uvCoord;

void main() 
{
    gl_Position = ubo.viewProj * ubo.model * vec4(position, 1.0);
    uvCoord = uv;
}