#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec4 v_Color;
layout(location = 2) in vec2 v_TextureCoord;
layout(location = 3) in float v_TextureIndex;
layout(location = 4) in int v_EntityID;
layout(location = 5) in float v_IsBillboard;
layout(location = 6) in float v_LockYAxis;
layout(location = 7) in vec3 v_BillboardCenter;
layout(location = 8) in vec2 v_BillboardOffset;

out vec4 color;
out vec2 textureCoord;
flat out float texIndex;
flat out int entityID;

layout(std140, binding = 0) uniform CameraData
{
    mat4 u_ViewProjection;
};

uniform vec3 u_CameraPosition;
uniform vec3 u_CameraRight;
uniform vec3 u_CameraUp;

void main()
{
	vec3 worldPosition = v_Position;

	if (v_IsBillboard > 0.5)
	{
		vec3 right = u_CameraRight;
		vec3 up = u_CameraUp;

		if (v_LockYAxis > 0.5)
		{
			vec3 toCamera = u_CameraPosition - v_BillboardCenter;
			toCamera.y = 0.0;
			float lengthSq = dot(toCamera, toCamera);
			if (lengthSq > 0.0001)
			{
				vec3 forward = normalize(toCamera);
				vec3 worldUp = vec3(0.0, 1.0, 0.0);
				right = normalize(cross(worldUp, forward));
				up = worldUp;
			}
		}

		worldPosition = v_BillboardCenter + right * v_BillboardOffset.x + up * v_BillboardOffset.y;
	}

	gl_Position = u_ViewProjection * vec4(worldPosition, 1.0);
	color = v_Color;
	textureCoord = v_TextureCoord;
	texIndex = v_TextureIndex;
	entityID = v_EntityID;
}

#shader fragment
#version 450 core

in vec4 color;
in vec2 textureCoord;
flat in float texIndex;
flat in int entityID;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outEmissions; // Keeps the FBO happy
layout(location = 2) out int outEntityID;   // Writes to the RedInteger attachment!

uniform sampler2D u_Textures[32];

void main()
{
	outColor = texture(u_Textures[int(texIndex)], textureCoord) * color;
	outEmissions = vec4(0.0); // TODO: Implement emission for quads
	outEntityID = entityID;
}