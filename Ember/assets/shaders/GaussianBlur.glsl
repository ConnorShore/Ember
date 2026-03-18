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

uniform sampler2D u_Image;
  
uniform bool u_HorizontalPass;
uniform float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

void main()
{             
    vec2 tex_offset = 1.0 / textureSize(u_Image, 0); // gets size of single texel
    vec3 result = texture(u_Image, TexCoord).rgb * weight[0]; // current fragment's contribution
    if(u_HorizontalPass)
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_Image, TexCoord + vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
            result += texture(u_Image, TexCoord - vec2(tex_offset.x * i, 0.0)).rgb * weight[i];
        }
    }
    else
    {
        for(int i = 1; i < 5; ++i)
        {
            result += texture(u_Image, TexCoord + vec2(0.0, tex_offset.y * i)).rgb * weight[i];
            result += texture(u_Image, TexCoord - vec2(0.0, tex_offset.y * i)).rgb * weight[i];
        }
    }

    OutColor = vec4(result, 1.0);
}