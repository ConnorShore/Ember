#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec4 v_Color;

out vec4 color;

uniform mat4 u_Transform;
uniform mat4 u_ViewProjection;

void main()
{
	gl_Position = u_ViewProjection * u_Transform * vec4(v_Position, 1.0);
	color = v_Color;
}

#shader fragment
#version 450 core

in vec4 color;

out vec4 outColor;

void main()
{	
	outColor = color;
}