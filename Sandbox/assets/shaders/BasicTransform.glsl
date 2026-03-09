#shader vertex
#version 450 core

in vec4 v_Position;

uniform mat4 u_ViewProjection;
uniform mat4 u_Model;

void main()
{
	gl_Position = u_ViewProjection * u_Model * v_Position;
};

#shader fragment
#version 450 core

out vec4 color;

uniform vec4 u_Color;

void main()
{
	color = u_Color;
};