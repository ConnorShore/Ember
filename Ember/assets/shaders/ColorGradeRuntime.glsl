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
uniform sampler2D u_BakedLUT;

void main() 
{
    vec3 color = texture(u_Scene, TexCoord).rgb;
    
    // Clamp to ensure extremely bright HDR values don't sample off the edge of the LUT
    color = clamp(color, 0.0, 1.0);

    // Calculate the Blue fractional blend
    float blueColor = color.b * 15.0; // Scale 0-1 to 0-15 blocks
    float block1 = floor(blueColor);
    
    // Don't overflow past the 15th block!
    float block2 = min(block1 + 1.0, 15.0); 
    
    // Get the decimal remainder (e.g., 0.3) to mix between the two blocks
    float fractBlue = fract(blueColor);

    // Calculate Half-Pixel offsets to prevent block bleeding
    // We map 0.0->1.0 to the center of the pixels (0.5 to 15.5) inside each 16x16 block
    float rOffset = (color.r * 15.0 + 0.5) / 256.0; 
    float gOffset = (color.g * 15.0 + 0.5) / 16.0;

    // Calculate the exact UVs for both adjacent blue blocks
    float blockWidth = 1.0 / 16.0;
    vec2 uv1 = vec2((block1 * blockWidth) + rOffset, gOffset);
    vec2 uv2 = vec2((block2 * blockWidth) + rOffset, gOffset);

    // Sample both blocks and smoothly blend them!
    vec3 color1 = texture(u_BakedLUT, uv1).rgb;
    vec3 color2 = texture(u_BakedLUT, uv2).rgb;
    
    OutColor = vec4(mix(color1, color2, fractBlue), 1.0);
}