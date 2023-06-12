#version 450

layout(set = 0, binding = 1) uniform UBO {
    vec3 camPos;
	float lightIntensity;
	vec3 lightDir;
	float ambient;
	float fresnel;
	float shadowAlpha;
} ubo;

layout(set = 0, binding = 2) uniform sampler2D shadowTex;

layout(push_constant) uniform constants
{
	vec3 tint;
	float roughness;
	float metallic;
} pushConstants;

layout(set = 1, binding = 0) uniform sampler2D tex;

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec4 lightPos;
layout(location = 2) in vec3 worldNormal;
layout(location = 3) in vec2 uvCoord;

layout(location = 0) out vec4 outColor;

#define PI 3.1415926559
#define epsilon 0.0025

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
	float a2 = roughness * roughness;
	float NdotH = max(dot(N, H), 0.0);

	float denom = NdotH * NdotH * (a2 - 1.0) + 1.0;
	denom = PI * denom * denom;
	return a2 / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
	float r = roughness + 1.0;
	float k = (r * r) / 8.0;
	return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
	float ggx1 = GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
	float ggx2 = GeometrySchlickGGX(max(dot(N, V), 0.0), roughness);
	return ggx1 * ggx2;
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float getShadow(vec4 shadowCoord, vec2 off, vec3 normal)
{
	vec2 uv = vec2(0.5 * shadowCoord.x + 0.5, -0.5 * shadowCoord.y + 0.5);
	shadowCoord.z -= epsilon;
	return (
		texture(shadowTex, uv + off).r < shadowCoord.z &&
		dot(ubo.lightDir, normal) < 0.0 && 
		abs(shadowCoord.z) < 1.0
	) ? 1.0 - ubo.shadowAlpha : 1.0;
}

float filterPCF(vec4 sc, vec3 normal)
{

	float d = 1.0 / float(textureSize(shadowTex, 0).x);
	float shadowFactor = 0.0;
	int count = 0;
	int range = 2;
	
	for (int x = -range; x <= range; x++)
	{
		for (int y = -range; y <= range; y++)
		{
			shadowFactor += getShadow(sc, vec2(d*x, d*y), normal);
			count++;
		}
	
	}
	return shadowFactor / count;
}

void main() 
{
	vec3 N = worldNormal;
	vec3 V = normalize(ubo.camPos - worldPos);
	vec3 tex = texture(tex, uvCoord).rgb;
	float lightness = (tex.r + tex.g + tex.b) / 3.0;

	vec3 albedo = tex * pushConstants.tint;
	float roughness = pushConstants.roughness;
	float metallic = pushConstants.metallic;

	vec3 F0 = vec3(ubo.fresnel);
	F0 = mix(F0, albedo, metallic);

	// Reflectance equation
	vec3 Lo = vec3(0.0);
	float shadow = 0.0;
	
		// For every active light
		vec3 L = -ubo.lightDir;
		vec3 H = normalize(V + L);
		vec3 radiance = vec3(1.0) * ubo.lightIntensity;

		float D = DistributionGGX(N, H, roughness * roughness);
		float G = GeometrySmith(N, V, L, roughness);
		vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

		vec3 kS = F;
		vec3 kD = vec3(1.0) - kS;
		kD *= 1.0 - metallic;
	
		float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.00001;
		vec3 specular = (D * G * F) / denominator;

		Lo += (kD * albedo / PI + specular) * radiance * max(dot(N, L), 0.0);

	shadow = filterPCF(lightPos / lightPos.w, N);
	vec3 ambient = vec3(ubo.ambient) * albedo;
	vec3 color = ambient + Lo;

	color = color / (color + vec3(1.0));
	color = pow(color, vec3(1.0/2.2));
	color *= vec3(shadow);

	color = mix(color, vec3(0.1), clamp((length(ubo.camPos - worldPos) - 10.0) / 75.0, 0.0, 1.0)); // Fade into background
	outColor = vec4(color, 1.0);

//	vec3 toLight = ubo.lightPos - worldPos;
//	vec3 toLightDir = normalize(toLight);
//	vec3 toCamDir = normalize(ubo.camPos - worldPos);
//	vec3 halfwayDir = normalize(toLightDir + toCamDir);
//	float attentuation = 1.0 / (1.0 + length(toLight));
//
//	vec3 diffuse = texture(texSampler, uvCoord).rgb * clamp(dot(worldNormal, toLightDir), 0.0, 1.0) * pushConstants.color;
//	vec3 specular = vec3(0.05) * pow(max(dot(worldNormal, halfwayDir), 0.0), ubo.specPower);
//
//	vec3 final = ubo.globalAmbient.xyz * texture(texSampler, uvCoord).rgb * pushConstants.color;
//	final += (diffuse + specular) * ubo.lightIntensity * attentuation * clamp((dot(-toLightDir, ubo.lightDir) - ubo.lightCone) / (1.0 - ubo.lightCone), 0.0, 1.0);
//	final = mix(final, vec3(0.01), min(max(length(ubo.camPos - worldPos) - 15.0, 0.0) / 50.0, 1.0)); // Fade into background
//
//	outColor = vec4(final, 1.0);
}