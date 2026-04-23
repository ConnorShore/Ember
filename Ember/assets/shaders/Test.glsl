#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TexCoord;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
};

uniform mat4 u_Transform;

out vec2 TexCoord;

void main()
{
    TexCoord = v_TexCoord;
    gl_Position = u_ViewProjection * u_Transform * vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

in vec2 TexCoord;
out vec4 OutColor;

uniform sampler2DArray u_TextureArray; 
uniform float u_LayerToTest;

void main()
{
    // A sampler2DArray requires a vec3 for texture coordinates.
    // X and Y are standard UVs. Z is the explicit layer index.
    OutColor = texture(u_TextureArray, vec3(TexCoord, u_LayerToTest));
}