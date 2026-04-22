#shader vertex
#version 450 core

// Static Quad Data (Advances per-vertex)
layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TexCoord;

// Dynamic Instance Data (Advances per-instance)
layout(location = 3) in vec3 i_Position;
layout(location = 4) in float i_Scale;
layout(location = 5) in vec4 i_Color;

uniform mat4 u_ViewProjection;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

out vec4 Color;
out vec2 TexCoord;

void main()
{
    Color = i_Color;
    TexCoord = v_TexCoord;
    
    // Build a spherical billboard using the camera vectors!
    vec3 vertexPos = i_Position 
                   + u_CameraRight * v_Position.x * i_Scale 
                   + u_CameraUp * v_Position.y * i_Scale;
                   
    gl_Position = u_ViewProjection * vec4(vertexPos, 1.0);
}