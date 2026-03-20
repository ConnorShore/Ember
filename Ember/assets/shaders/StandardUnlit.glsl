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

// @UIProperty(Name = "Color", Type = Color3)
uniform vec3 u_Color;

// @UIProperty(Name="Emission Intensity", Type=Float, Min=1.0, Max=100.0, Step=0.5, Normalize=false)
uniform float u_Emission;

uniform int u_EntityID;

void main()
{
    vec3 finalColor = u_Color * u_Emission;
    OutColor = vec4(finalColor, 1.0);
    BrightColor = vec4(max(OutColor.rgb - vec3(1.0), vec3(0.0)), 1.0);
    EntityID = u_EntityID;
}