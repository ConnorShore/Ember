#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;

out vec2 TexCoord;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(v_Position, 1.0);
    TexCoord = v_TextureCoord;
}

#shader fragment
#version 450 core

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int EntityID;

in vec2 TexCoord;

uniform sampler2D u_Image;
uniform vec4 u_Color;
uniform int u_EntityID;

void main()
{
    vec4 texColor = texture(u_Image, TexCoord);
    OutColor = texColor * u_Color;
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    EntityID = u_EntityID;
}