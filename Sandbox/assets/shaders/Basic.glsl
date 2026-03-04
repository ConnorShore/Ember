#shader vertex
#version 330 core

layout(location = 0) in vec4 v_Position;

void main()
{
	gl_Position = v_Position;
};

#shader fragment
#version 330 core

layout(location = 0) out vec4 color;

//uniform vec4 u_Color;

void main()
{
	color = vec4(1.0, 0.0, 0.0, 1.0);
};