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

out vec4 OutColor;

uniform vec3 u_Color;

void main()
{
    OutColor = vec4(u_Color, 1.0);
}