#shader vertex
#version 450 core

out vec2 TexCoord;

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord;

void main()
{
    TexCoord = v_TextureCoord;
    gl_Position = vec4(v_Position, 1.0);
}

#shader fragment
#version 450 core

out vec4 OutColor;

in vec2 TexCoord;

uniform sampler2D u_Scene;
uniform sampler2D u_BloomBlur;

uniform float u_Exposure;

void main()
{           
    const float gamma = 2.2;

    vec3 hdrColor = texture(u_Scene, TexCoord).rgb;      
    vec3 bloomColor = texture(u_BloomBlur, TexCoord).rgb;
    hdrColor += bloomColor; // additive blending

    // Tone mapping
    vec3 result = vec3(1.0) - exp(-hdrColor * u_Exposure);

    // Gamma correction     
    result = pow(result, vec3(1.0 / gamma));

    OutColor = vec4(result, 1.0);
} 
