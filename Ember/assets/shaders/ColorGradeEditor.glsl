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

// White balance uniforms
uniform float u_Temperature;
uniform float u_Tint;

// Color adjustment uniforms
uniform float u_Contrast;
uniform float u_Saturation;

void applyWhiteBalance(inout vec3 color)
{
    float tempScale = 1.0 + (u_Temperature * 0.15); 
    float tintScale = 1.0 + (u_Tint * 0.15);        

    // RGB to LMS Color Space Conversion Matrix
    mat3 rgbToLms = transpose(mat3(
        0.3811, 0.5783, 0.0402,
        0.1967, 0.7244, 0.0782,
        0.0241, 0.1288, 0.8444
    ));

    // LMS to RGB Conversion Matrix
    mat3 lmsToRgb = transpose(mat3(
         4.4679, -3.5873,  0.1193,
        -1.2186,  2.3809, -0.1624,
         0.0497, -0.2439,  1.2045
    ));

    // Convert pixel to LMS
    vec3 lmsColor = rgbToLms * color;

    // Apply the Temperature and Tint
    lmsColor.x *= tempScale; 
    lmsColor.y /= tintScale; 
    lmsColor.z /= tempScale; 

    // Convert back to RGB
    color = lmsToRgb * lmsColor;
}

void applyColorAdjustment(inout vec3 color)
{
    // Apply Contrast
    color = (color - 0.5) * u_Contrast + 0.5;

    // Apply Saturation
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));
    vec3 grayscale = vec3(luminance);
    
    // mix() blends between grayscale (0.0) and full color (1.0)
    // Pushing u_Saturation above 1.0 extrapolates the colors natively.
    color = mix(grayscale, color, u_Saturation);

    // Clamp the final color to ensure it stays within the valid range
    color = max(color, 0.0); 
}

void main()
{
    vec3 sceneColor = texture(u_Scene, TexCoord).rgb;
    applyWhiteBalance(sceneColor);
    applyColorAdjustment(sceneColor);
    OutColor = vec4(sceneColor, 1.0);
}