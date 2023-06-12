#version 450

layout(push_constant) uniform constants {
    mat4 viewProj;
} pc;

layout(location = 0) in vec3 position;

void main() 
{
    gl_Position = pc.viewProj * vec4(position, 1.0);
}