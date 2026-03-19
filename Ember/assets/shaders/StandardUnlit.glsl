#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

uniform mat4 u_Transform;

void main()
{
    gl_Position = u_ViewProjection * u_Transform * vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;
layout(location = 2) out int EntityID;

uniform vec3 u_Color;
uniform int u_EntityID;

void main()
{
    OutColor = vec4(u_Color, 1.0);
    BrightColor = vec4(max(OutColor.rgb - vec3(1.0), vec3(0.0)), 1.0);
    EntityID = u_EntityID;
}