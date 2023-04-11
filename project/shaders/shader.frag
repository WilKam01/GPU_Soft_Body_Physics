#version 450

layout(binding = 1) uniform sampler2D texSampler;

layout(location = 0) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
    outColor = vec4(texture(texSampler, uvCoord).rgb, 1.0);
}