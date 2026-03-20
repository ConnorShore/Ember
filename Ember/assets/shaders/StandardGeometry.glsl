#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;
layout(location = 3) in vec3 v_Tangent;
layout(location = 4) in vec3 v_Bitangent;

out VertexOut {
    vec3 WorldPos;
    vec2 TexCoord;
    mat3 TBN;
} VertOut;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

uniform mat4 u_Transform;

void main()
{
    vec4 worldPos = u_Transform * vec4(v_Position, 1.0);
    gl_Position = u_ViewProjection * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(u_Transform)));
    vec3 T = normalize(normalMatrix * v_Tangent);
    vec3 B = normalize(normalMatrix * v_Bitangent);
    vec3 N = normalize(normalMatrix * v_Normal);
    
    VertOut.WorldPos = vec3(worldPos);
    VertOut.TexCoord = v_TextureCoord;
    VertOut.TBN = mat3(T, B, N);
}

#shader fragment
#version 450 core

in VertexOut {
    vec3 WorldPos;
    vec2 TexCoord;
    mat3 TBN;
} FragIn;

layout(location = 0) out vec4 AlbedoRoughness; 
layout(location = 1) out vec4 NormalMetallic;
layout(location = 2) out vec4 PositionAO;
layout(location = 3) out int EntityID;

// @UIProperty(Name="Albedo", Type=Color3)
uniform vec3 u_Albedo;

// @UIProperty(Name="Metallic", Type=Float)
uniform float u_Metallic;

// @UIProperty(Name="Roughness", Type=Float)
uniform float u_Roughness;

// @UIProperty(Name="Ambient Occlusion", Type=Float)
uniform float u_AO;

uniform int u_EntityID;

layout(binding = 0) uniform sampler2D u_AlbedoMap;
layout(binding = 1) uniform sampler2D u_NormalMap;

void main()
{
    vec4 texColor = texture(u_AlbedoMap, FragIn.TexCoord);
    vec3 linearTexColor = pow(texColor.rgb, vec3(2.2)); // sRGB to Linear

    // Albedo / Roughness
    AlbedoRoughness.rgb = linearTexColor * u_Albedo;
    AlbedoRoughness.a = u_Roughness; 

    // Normal / metallic
    vec3 normalMap = texture(u_NormalMap, FragIn.TexCoord).rgb;
    
    // Transform from [0,1] to [-1,1])
    normalMap = normalMap * 2.0 - 1.0;
    vec3 finalNormal = normalize(FragIn.TBN * normalMap).rgb;
    NormalMetallic.rgb = finalNormal;
    NormalMetallic.a = u_Metallic; 
    
    // Position / AO
    PositionAO.rgb = FragIn.WorldPos;
    PositionAO.a = u_AO; 

    // Code entity id into the entity ID buffer
    EntityID = u_EntityID;
}