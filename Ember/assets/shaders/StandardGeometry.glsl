#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoord;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

uniform mat4 u_Transform;

void main()
{
    vec4 worldPos = u_Transform * vec4(v_Position, 1.0);
    gl_Position = u_ViewProjection * worldPos;

    WorldPos = vec3(worldPos);
    Normal = mat3(transpose(inverse(u_Transform))) * v_Normal;
    TexCoord = v_TextureCoord;
}

#shader fragment
#version 450 core

in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;

layout(location = 0) out vec4 gAlbedoRoughness; 
layout(location = 1) out vec4 gNormalMetallic;
layout(location = 2) out vec4 gPositionAO;

uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;

uniform sampler2D u_AlbedoMap;

void main()
{
    vec4 texColor = texture(u_AlbedoMap, TexCoord);
    vec3 linearTexColor = pow(texColor.rgb, vec3(2.2)); // sRGB to Linear

    gAlbedoRoughness.rgb = linearTexColor * u_Albedo;
    gAlbedoRoughness.a = u_Roughness; 

    gNormalMetallic.rgb = normalize(Normal);
    gNormalMetallic.a = u_Metallic; 
    
    gPositionAO.rgb = WorldPos;
    gPositionAO.a = u_AO; 
}