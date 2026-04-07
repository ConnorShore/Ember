#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;
layout(location = 3) in vec3 v_Tangent;
layout(location = 4) in vec3 v_Bitangent;
layout(location = 5) in uvec4 v_BoneIDs;
layout(location = 6) in vec4 v_BoneWeights;

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

const int MAX_BONES = 100;
uniform mat4 u_BoneMatrices[MAX_BONES];

void main()
{
    mat4 boneTransform = u_BoneMatrices[v_BoneIDs[0]] * v_BoneWeights[0];
    boneTransform     += u_BoneMatrices[v_BoneIDs[1]] * v_BoneWeights[1];
    boneTransform     += u_BoneMatrices[v_BoneIDs[2]] * v_BoneWeights[2];
    boneTransform     += u_BoneMatrices[v_BoneIDs[3]] * v_BoneWeights[3];

    // 1. Transform the local vertex by the bones
    vec4 localPosition = boneTransform * vec4(v_Position, 1.0);

    vec4 worldPos = u_Transform * localPosition;
    gl_Position = u_ViewProjection * worldPos;

    mat3 normalMatrix = mat3(transpose(inverse(u_Transform * boneTransform)));
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
layout(location = 3) out vec4 EmissionOut;
layout(location = 4) out int EntityID;

// @UIProperty(Name="Albedo", Type=Color3)
uniform vec3 u_Albedo;

// @UIProperty(Name="Metallic", Type=Float)
uniform float u_Metallic;

// @UIProperty(Name="Roughness", Type=Float)
uniform float u_Roughness;

// @UIProperty(Name="Ambient Occlusion", Type=Float)
uniform float u_AO;

// @UIProperty(Name="Emission Color", Type=Color3)
uniform vec3 u_EmissionColor;

// @UIProperty(Name="Emission Factor", Type=Float, Min=0.0, Max=100.0, Step=0.1, Normalize=false)
uniform float u_Emission;

uniform int u_EntityID;

// @UIProperty(Name="Albedo Texture", Type=Texture)
layout(binding = 0) uniform sampler2D u_AlbedoMap;

// @UIProperty(Name="Normal Texture", Type=Texture)
layout(binding = 1) uniform sampler2D u_NormalMap;

// @UIProperty(Name="Metallic/Roughness Texture", Type=Texture)
layout(binding = 2) uniform sampler2D u_MetallicRoughnessMap;

// @UIProperty(Name="Emissive Texture", Type=Texture)
layout(binding = 3) uniform sampler2D u_EmissiveMap;

void main()
{
    // Albedo (sRGB -> Linear)
    vec4 texColor = texture(u_AlbedoMap, FragIn.TexCoord);
    vec3 linearTexColor = pow(texColor.rgb, vec3(2.2));

    // ORM Data (Already Linear! Do NOT pow() this!)
    // Red = AO, Green = Roughness, Blue = Metallic
    vec3 orm = texture(u_MetallicRoughnessMap, FragIn.TexCoord).rgb;
    
    float finalAO        = orm.r * u_AO;
    float finalRoughness = orm.g * u_Roughness;
    float finalMetallic  = orm.b * u_Metallic;

    // Normal Map
    vec3 normalMap = texture(u_NormalMap, FragIn.TexCoord).rgb;
    normalMap = normalMap * 2.0 - 1.0; // Transform from [0,1] to [-1,1]
    vec3 finalNormal = normalize(FragIn.TBN * normalMap).rgb;
    
    // Emission (sRGB -> Linear)
    vec3 emissionMap = texture(u_EmissiveMap, FragIn.TexCoord).rgb;
    vec3 finalEmission = pow(emissionMap, vec3(2.2)) * u_EmissionColor * u_Emission;
    
    // RGB = Albedo, Alpha = Roughness
    AlbedoRoughness.rgb = linearTexColor * u_Albedo;
    AlbedoRoughness.a = finalRoughness; 

    // RGB = Normal, Alpha = Metallic
    NormalMetallic.rgb = finalNormal;
    NormalMetallic.a = finalMetallic; 
    
    // RGB = Position, Alpha = AO
    PositionAO.rgb = FragIn.WorldPos;
    PositionAO.a = finalAO; 
    
    // Emissions
    EmissionOut = vec4(finalEmission, 1.0);

    // Entity ID Buffer
    EntityID = u_EntityID;
}