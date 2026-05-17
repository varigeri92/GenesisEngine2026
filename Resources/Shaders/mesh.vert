#version 450
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 outWorldPosition;
layout (location = 1) out vec3 outWorldNormal;
layout (location = 2) out vec2 outUV;

struct Vertex {

	vec3 position;
	float uv_x;
	vec3 normal;
	float uv_y;
	vec4 color;
}; 

layout(set = 0, binding = 0) uniform SceneData {
    mat4 view;
    mat4 proj;
    mat4 viewproj;
    vec4 ambientColor;
} sceneData;

layout(buffer_reference, std430) readonly buffer VertexBuffer{ 
	Vertex vertices[];
};

layout(std430, set = 3, binding = 0) readonly buffer DrawModelMatrices
{
	mat4 modelMatrices[];
} drawModelMatrices;

layout(std430, set = 3, binding = 1) readonly buffer DrawVertexBufferAddresses
{
	VertexBuffer vertexBuffers[];
} drawVertexBufferAddresses;

//push constants block
layout( push_constant ) uniform constants
{	
	uint drawIndex;
	uint padding0;
	uint padding1;
	uint padding2;
} PushConstants;

void main() 
{	
	uint drawIndex = PushConstants.drawIndex;
	mat4 modelMatrix = drawModelMatrices.modelMatrices[drawIndex];
	VertexBuffer vertexBuffer = drawVertexBufferAddresses.vertexBuffers[drawIndex];
	Vertex v = vertexBuffer.vertices[gl_VertexIndex];
	vec4 worldPosition = modelMatrix * vec4(v.position, 1.0f);

	//output data
	gl_Position = sceneData.viewproj * worldPosition;
	outWorldPosition = worldPosition.xyz;
	outWorldNormal = normalize(transpose(inverse(mat3(modelMatrix))) * v.normal);
	outUV.x = v.uv_x;
	outUV.y = v.uv_y;
}
