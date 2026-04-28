#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;

out vec2 TexCoord;

void main()
{
    TexCoord = v_TextureCoord;
    gl_Position = vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

in vec2 TexCoord;

out vec4 OutColor;

uniform sampler2D u_Scene;

uniform float u_AspectRatio;

uniform float u_Intensity;
uniform float u_Smoothness;
uniform float u_Size;
uniform vec3 u_Color;

void main() 
{
    vec3 sceneColor = texture(u_Scene, TexCoord).rgb;

    // 1. Shift UVs so (0,0) is the exact center of the screen
    vec2 centeredUV = TexCoord - 0.5;

    // 2. Multiply the X axis by the aspect ratio to make the math a perfect circle
    centeredUV.x *= u_AspectRatio;

    // 3. Calculate how far this pixel is from the center
    float dist = length(centeredUV);

    // 4. Use smoothstep to create the gradient. 
    // smoothstep(min, max, value) returns 0.0 if value is below min, 
    // 1.0 if above max, and smoothly interpolates between them.
    float vignette = smoothstep(u_Size, u_Size + u_Smoothness, dist);

    // Multiply by intensity so the user can fade the whole effect
    vignette *= u_Intensity;

    // 5. Mix the scene color with the vignette color
    OutColor = vec4(mix(sceneColor, u_Color, vignette), 1.0);
}