#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec4 v_Color;
layout(location = 2) in vec2 v_TextureCoord;
layout(location = 3) in float v_TextureIndex;

out vec4 color;
out vec2 textureCoord;
flat out float texIndex;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

void main()
{
	gl_Position = u_ViewProjection * vec4(v_Position, 1.0);
	color = v_Color;
	textureCoord = v_TextureCoord;
	texIndex = v_TextureIndex;
}

#shader fragment
#version 450 core

in vec4 color;
in vec2 textureCoord;
flat in float texIndex;

out vec4 outColor;

uniform sampler2D u_Textures[32];

void main()
{
	outColor = texture(u_Textures[int(texIndex)], textureCoord) * color;
}