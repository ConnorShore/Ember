#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;

// Pass these to the Fragment Shader
out vec3 WorldPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 u_Transform;
uniform mat4 u_ViewProjection;

void main()
{
    // Calculate World Position for the G-Buffer
    vec4 worldPosition = u_Transform * vec4(v_Position, 1.0);
    WorldPos = worldPosition.xyz;

    // Correctly rotate the normals based on the entity's rotation
    Normal = mat3(transpose(inverse(u_Transform))) * v_Normal;
    
    TexCoord = v_TextureCoord;

    // Tell OpenGL where this vertex actually is on the screen
    gl_Position = u_ViewProjection * worldPosition;
}

#shader fragment
#version 450 core

// Receive from Vertex Shader
in vec3 WorldPos;
in vec3 Normal;
in vec2 TexCoord;

// These map directly to your Framebuffer's Color Attachments 0, 1, and 2!
layout(location = 0) out vec4 gAlbedoRoughness; 
layout(location = 1) out vec4 gNormalMetallic;
layout(location = 2) out vec4 gPositionAO;

// The material properties you set in C++
uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;
uniform sampler2D u_Texture;

void main()
{
    // 1. Albedo & Roughness
    vec4 texColor = texture(u_Texture, TexCoord);
    vec3 linearTexColor = pow(texColor.rgb, vec3(2.2)); // sRGB to Linear

    gAlbedoRoughness.rgb = linearTexColor * u_Albedo;
    gAlbedoRoughness.a = 1.0; // Temporarily 1.0 for testing!

    gNormalMetallic.rgb = normalize(Normal);
    gNormalMetallic.a = 1.0; // Temporarily 1.0 for testing!
    
    gPositionAO.rgb = WorldPos;
    gPositionAO.a = 1.0; 
}
