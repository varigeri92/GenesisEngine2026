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

struct MaterialData
{
	vec4 albedo_color;
	vec4 emissive_color;
	vec4 texture_tiling_offset;
	float metallic;
	float roughness;
	float ambient_occlusion;
	float alpha;
	float normal_strength;
	float emissive_strength;
	float alpha_cutoff;
	float padding0;
};

layout(std430, set = 1, binding = 0) readonly buffer MaterialDataBuffer
{
	MaterialData materials[];
} materialDataBuffer;

layout(set = 2, binding = 0) uniform sampler2D albedo_texture;
layout(set = 2, binding = 1) uniform sampler2D normal_map;
layout(set = 2, binding = 2) uniform sampler2D metallic_map;
layout(set = 2, binding = 3) uniform sampler2D roughness_map;
layout(set = 2, binding = 4) uniform sampler2D ambient_occlusion_map;
layout(set = 2, binding = 5) uniform sampler2D emissive_map;

layout( push_constant ) uniform constants
{
	uint drawIndex;
	uint padding0;
	uint padding1;
	uint padding2;
} PushConstants;

//output write
layout (location = 0) out vec4 outFragColor;

void main() 
{
	MaterialData materialData = materialDataBuffer.materials[PushConstants.drawIndex];
	vec4 baseColor = texture(albedo_texture, inUV) * materialData.albedo_color;
	if (materialData.alpha < -1.0f)
	{
		baseColor *= materialData.albedo_color;
		baseColor.rgb += materialData.emissive_color.rgb * materialData.emissive_strength;
		baseColor.a *= materialData.alpha;
		baseColor.rgb *= vec3(
			materialData.metallic,
			materialData.roughness,
			materialData.ambient_occlusion);
		baseColor.rg += materialData.texture_tiling_offset.xy + materialData.texture_tiling_offset.zw;
		baseColor.b += materialData.normal_strength + materialData.alpha_cutoff + materialData.padding0;
		baseColor.rgb += texture(normal_map, inUV).rgb;
		baseColor.r += texture(metallic_map, inUV).r;
		baseColor.g += texture(roughness_map, inUV).r;
		baseColor.b += texture(ambient_occlusion_map, inUV).r;
		baseColor.rgb += texture(emissive_map, inUV).rgb;
	}

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

	outFragColor = vec4(baseColor.rgb * lighting, baseColor.a);
}
