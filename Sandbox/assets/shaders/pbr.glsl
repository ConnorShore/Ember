#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 1) in vec3 v_Normal;
layout(location = 2) in vec2 v_TextureCoord;

out vec3 WorldPos;
out vec3 Normal;
out vec2 TextureCoord;

uniform mat4 u_Transform;
uniform mat4 u_ViewProjection;

void main()
{
	vec4 worldPos = u_Transform * vec4(v_Position, 1.0);
	gl_Position = u_ViewProjection * worldPos;

	WorldPos = vec3(worldPos);
	Normal = mat3(transpose(inverse(u_Transform))) * v_Normal;
	TextureCoord = v_TextureCoord;
}

#shader fragment
#version 450 core

const float PI = 3.14159265359;
const float AMBIENT = 0.03;
const int MAX_LIGHTS = 4;

struct PointLight {
	vec3 Position;
	vec3 Color;
	float Intensity;
};

in vec3 WorldPos;
in vec3 Normal;
in vec2 TextureCoord;

out vec4 OutColor;

uniform vec3 u_Albedo;
uniform float u_Metallic;
uniform float u_Roughness;
uniform float u_AO;

uniform vec3 u_CameraPos;

uniform PointLight u_PointLights[MAX_LIGHTS];

uniform sampler2D u_Texture;

float NormalDistributionTrowbridgeReitxGGX(vec3 N, vec3 H, float roughness)
{
	float aSqr = roughness * roughness;
	float nDotH = max(dot(N, H), 0.0);
	float nDotHSqr = nDotH * nDotH;
	float denomBulk = (nDotHSqr * (aSqr - 1.0)) + 1.0;
	float fullDenom = PI * (denomBulk * denomBulk);
	return aSqr / fullDenom;
}

float GeometrySchlickGGXSub(vec3 N, vec3 V, float roughness)
{
	float K = pow((roughness + 1.0), 2.0) / 8.0;
	float nDotV = max(dot(N, V), 0.0);

	return nDotV / ((nDotV * (1.0 - K)) + K);
}

vec3 Fresnel(vec3 V, vec3 H, vec3 F0)
{
	return F0 + (1.0 - F0) * pow(clamp(1.0 - dot(H, V), 0.0, 1.0), 5.0);
}

void main()
{	
	vec4 texColor = texture(u_Texture, TextureCoord);

	// Convert the texture from sRGB to Linear Space
	vec3 linearTexColor = pow(texColor.rgb, vec3(2.2));
	vec3 actualAlbedo = linearTexColor * u_Albedo;

	vec3 N = normalize(Normal);
	vec3 V = normalize(u_CameraPos - WorldPos);

	// Clamp roughness to avoid NDF collapsing to 0 (produces flat ambient-only result)
	float roughness = max(u_Roughness, 0.05);

	vec3 L0 = vec3(0.0);
	for (int i = 0; i < MAX_LIGHTS; i++)
	{
		vec3 L = normalize(u_PointLights[i].Position - WorldPos);
		vec3 H = normalize(V + L);

		float distance = length(u_PointLights[i].Position - WorldPos);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = u_PointLights[i].Color * u_PointLights[i].Intensity * attenuation;

		// BRDF (Cook-Torrance)
		float NdotL = max(dot(N, L), 0.0);
		vec3 F0 = mix(vec3(0.04), actualAlbedo, u_Metallic);
		float D = NormalDistributionTrowbridgeReitxGGX(N, H, roughness);
		float G = max(GeometrySchlickGGXSub(N, V, roughness), 0.0) * max(GeometrySchlickGGXSub(N, L, roughness), 0.0);
		vec3 F = Fresnel(V, H, F0);

		vec3 KS = F;				// Specular factor
		vec3 KD = vec3(1.0) - KS;	// diffuse factor
		KD *= 1.0f - u_Metallic;

		vec3 numerator = D * G * F;
		float denomenator = 4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.0001;
		vec3 specular =  numerator / denomenator;
		
		L0 += (KD * actualAlbedo / PI + specular) * radiance * NdotL;
	}

	// Ambient light
	vec3 ambient = vec3(AMBIENT) * actualAlbedo * u_AO;
    vec3 color = ambient + L0;
	
	// HDR Tonemapping & Gamma Correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    OutColor = vec4(color, 1.0);
}