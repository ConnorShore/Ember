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

void main()
{
    // TODO: Apply color grading here using a color grading LUT

    vec3 sceneColor = texture(u_Scene, TexCoord).rgb;
    OutColor = vec4(sceneColor, 1.0);
}