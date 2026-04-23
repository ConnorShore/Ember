#shader vertex
#version 450 core

// Static Quad Data (Advances per-vertex)
layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TexCoord;

// Dynamic Instance Data (Advances per-instance)
layout(location = 5) in vec3 i_Position;
layout(location = 6) in float i_Rotation;
layout(location = 7) in vec2 i_Scale;
layout(location = 8) in vec4 i_Color;
layout(location = 9) in uint i_TexIndex;

layout(std140, binding = 0) uniform Camera
{
    mat4 u_ViewProjection;
};

uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

out vec4 Color;
out vec2 TexCoord;
flat out uint TexIndex; // Pass the texture

void main()
{
    Color = i_Color;
    TexCoord = v_TexCoord;
    TexIndex = i_TexIndex;

    // Create a 2D rotation matrix
    float cosRot = cos(i_Rotation);
    float sinRot = sin(i_Rotation);
    mat2 rotMat = mat2(cosRot, sinRot, -sinRot, cosRot);
    
    // Scale the quad
    vec2 scaledPos = v_Position.xy * i_Scale;

    // Rotate the quad
    vec2 rotatedPos = rotMat * scaledPos;
    
    // Translate the quad to its world position
    vec3 vertexPos = i_Position 
                   + u_CameraRight * rotatedPos.x 
                   + u_CameraUp * rotatedPos.y;
                   
    gl_Position = u_ViewProjection * vec4(vertexPos, 1.0);
}

#shader fragment
#version 450 core

in vec4 Color;
in vec2 TexCoord;
flat in uint TexIndex;

out vec4 OutColor;

uniform sampler2D u_Textures[32]; // Maximum texture slots

void main()
{
    vec4 texColor = texture(u_Textures[TexIndex], TexCoord); 
    OutColor = texColor * Color;
}