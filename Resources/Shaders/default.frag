#version 450

//shader input
layout (location = 0) in vec3 inWorldPosition;
layout (location = 1) in vec3 inWorldNormal;
layout (location = 2) in vec2 inUV;

const uint MAX_LIGHTS = 128u;

struct DirectionalLight
{
	vec4 direction;
	vec4 color;
};

struct PointLight
{
	vec4 position;
	vec4 color;
};

struct SpotLight
{
	vec4 position;
	vec4 direction;
	vec4 color;
	vec4 cone;
};

layout(set = 0, binding = 0) uniform SceneData
{
	mat4 view;
	mat4 proj;
	mat4 viewproj;
	vec4 ambientColor;
} sceneData;

layout(std430, set = 0, binding = 1) readonly buffer DirectionalLights
{
	uint count;
	DirectionalLight lights[MAX_LIGHTS];
} directionalLights;

layout(std430, set = 0, binding = 2) readonly buffer PointLights
{
	uint count;
	PointLight lights[MAX_LIGHTS];
} pointLights;

layout(std430, set = 0, binding = 3) readonly buffer SpotLights
{
	uint count;
	SpotLight lights[MAX_LIGHTS];
} spotLights;

layout(set = 1, binding = 0) uniform MaterialData
{
	vec4 albedo_color;
} materialData;

layout(set = 2, binding = 0) uniform sampler2D albedoTexture;

//output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
	vec4 albedo = texture(albedoTexture, inUV) * materialData.albedo_color;
	vec3 normal = normalize(inWorldNormal);
	vec3 lighting = sceneData.ambientColor.rgb * sceneData.ambientColor.a;

	for (uint i = 0u; i < directionalLights.count && i < MAX_LIGHTS; ++i)
	{
		DirectionalLight light = directionalLights.lights[i];
		vec3 lightDirection = normalize(-light.direction.xyz);
		float diffuse = max(dot(normal, lightDirection), 0.0f);
		lighting += light.color.rgb * max(light.direction.w, 0.0f) * diffuse;
	}

	for (uint i = 0u; i < pointLights.count && i < MAX_LIGHTS; ++i)
	{
		PointLight light = pointLights.lights[i];
		vec3 toLight = light.position.xyz - inWorldPosition;
		float distanceToLight = length(toLight);
		float range = max(light.position.w, 0.001f);
		vec3 lightDirection = toLight / max(distanceToLight, 0.001f);
		float attenuation = clamp(1.0f - distanceToLight / range, 0.0f, 1.0f);
		attenuation *= attenuation;
		float diffuse = max(dot(normal, lightDirection), 0.0f);
		lighting += light.color.rgb * max(light.color.w, 0.0f) * diffuse * attenuation;
	}

	for (uint i = 0u; i < spotLights.count && i < MAX_LIGHTS; ++i)
	{
		SpotLight light = spotLights.lights[i];
		vec3 toLight = light.position.xyz - inWorldPosition;
		float distanceToLight = length(toLight);
		float range = max(light.position.w, 0.001f);
		vec3 lightDirection = toLight / max(distanceToLight, 0.001f);
		vec3 spotDirection = normalize(light.direction.xyz);
		float angle = dot(normalize(inWorldPosition - light.position.xyz), spotDirection);
		float cone = smoothstep(light.cone.y, light.cone.x, angle);
		float attenuation = clamp(1.0f - distanceToLight / range, 0.0f, 1.0f);
		attenuation *= attenuation;
		float diffuse = max(dot(normal, lightDirection), 0.0f);
		lighting += light.color.rgb * max(light.color.w, 0.0f) * diffuse * attenuation * cone;
	}

	outFragColor = vec4(albedo.rgb * lighting, albedo.a);
}
