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
uniform float u_Exposure;  

void main()
{
    // Sample the HDR image
    vec3 hdrColor = texture(u_Scene, TexCoord).rgb;
    
    // Exposure Tone Mapping
    // This compresses infinite brightness into a 0.0 to 1.0 range gracefully.
    // It preserves details in the bright glowing areas instead of turning them pure white.
    vec3 mapped = vec3(1.0) - exp(-hdrColor * u_Exposure);
    
    // Gamma Correction
    // Monitors naturally display dark colors darker than they should be. 
    // We boost the signal here (1.0 / 2.2) so the monitor displays the math perfectly linearly.
    const float gamma = 2.2;
    vec3 finalColor = pow(mapped, vec3(1.0 / gamma));
    
    OutColor = vec4(finalColor, 1.0);
}