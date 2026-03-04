#shader vertex
#version 450 core

in vec4 v_Position;
in vec4 v_Color;

out vec4 o_Color;

void main()
{
	gl_Position = v_Position;
	o_Color = v_Color;
};

#shader fragment
#version 450 core

in vec4 o_Color;

out vec4 color;

//uniform vec4 u_Color;

void main()
{
	color = o_Color;
};