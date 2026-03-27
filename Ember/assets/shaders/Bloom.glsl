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

uniform float u_BloomIntensity;

void main()
{
    vec3 hdrColor = texture(u_Scene, TexCoord).rgb;      
    vec3 bloomColor = texture(u_BloomBlur, TexCoord).rgb;
    OutColor = vec4(hdrColor + (bloomColor * u_BloomIntensity), 1.0);
} 
