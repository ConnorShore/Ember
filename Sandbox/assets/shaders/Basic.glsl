#shader vertex
#version 450 core

in vec4 v_Position;

uniform mat4 u_ViewProjection;

void main()
{
	gl_Position = u_ViewProjection * v_Position;
};

#shader fragment
#version 450 core

out vec4 color;

uniform vec4 u_Color;

void main()
{
	color = u_Color;
};