#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec4 v_Color;

out vec4 Color;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

void main()
{
    // Fixed: RP3D lines are already in World Space, so we just apply the camera!
    gl_Position = u_ViewProjection * vec4(v_Position, 1.0);
    Color = v_Color;
}

#shader fragment
#version 450 core

in vec4 Color;
out vec4 outColor;

void main()
{
    outColor = Color;
}