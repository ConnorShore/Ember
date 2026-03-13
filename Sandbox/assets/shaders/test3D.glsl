#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;

out vec3 position;
out vec3 normal;
out vec2 textureCoord;

uniform mat4 u_Transform;
uniform mat4 u_ViewProjection;

void main()
{
	gl_Position = u_ViewProjection * u_Transform * vec4(v_Position, 1.0);
	position = v_Position;
	normal = v_Normal;
	textureCoord = v_TextureCoord;
}

#shader fragment
#version 450 core

in vec3 position;
in vec3 normal;
in vec2 textureCoord;

out vec4 outColor;

uniform vec4 u_TintColor;
uniform sampler2D u_Texture;

void main()
{	
	outColor = texture(u_Texture, textureCoord) * u_TintColor;
}