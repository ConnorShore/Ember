#shader vertex
#version 450 core

in vec4 v_Position;

void main()
{
	gl_Position = v_Position;
};

#shader fragment
#version 450 core

out vec4 color;

uniform vec4 u_Color;

void main()
{
	color = u_Color;
};