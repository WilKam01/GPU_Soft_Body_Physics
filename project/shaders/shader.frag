#version 450

layout(set = 0, binding = 0) uniform UBO {
    mat4 viewProj;
    vec3 camPos;
	float lightIntensity;
	vec3 lightPos;
	float lightCone;
	vec3 lightDir;
	float specPower;
	vec4 globalAmbient;
} ubo;

layout(push_constant) uniform constants
{
	vec3 color;
} pushConstants;

layout(set = 1, binding = 0) uniform sampler2D texSampler;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec3 worldNormal;
layout(location = 2) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

void main() 
{
	vec3 toLight = ubo.lightPos - worldPos;
	vec3 toLightDir = normalize(toLight);
	vec3 toCamDir = normalize(ubo.camPos - worldPos);
	vec3 halfwayDir = normalize(toLightDir + toCamDir);
	float attentuation = 1.0 / (1.0 + length(toLight));

	vec3 diffuse = texture(texSampler, uvCoord).rgb * clamp(dot(worldNormal, toLightDir), 0.0, 1.0) * pushConstants.color;
	vec3 specular = vec3(0.05) * pow(max(dot(worldNormal, halfwayDir), 0.0), ubo.specPower);

	vec3 final = ubo.globalAmbient.xyz * texture(texSampler, uvCoord).rgb * pushConstants.color;
	final += (diffuse + specular) * ubo.lightIntensity * attentuation * clamp((dot(-toLightDir, ubo.lightDir) - ubo.lightCone) / (1.0 - ubo.lightCone), 0.0, 1.0);
	final = mix(final, vec3(0.01), min(max(length(ubo.camPos - worldPos) - 15.0, 0.0) / 50.0, 1.0)); // Fade into background

	outColor = vec4(final, 1.0);
}