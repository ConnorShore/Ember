#shader vertex
#version 450 core

layout(location = 0) in vec3 v_Position;
layout(location = 2) in vec2 v_TextureCoord; // Location 2 is UVs (skip 1)

out vec2 TextureCoord;

void main()
{
    TextureCoord = v_TextureCoord;
    gl_Position = vec4(v_Position, 1.0); 
}

#shader fragment
#version 450 core

const float PI = 3.14159265359;

struct PointLight {
	vec3 Position;
	vec3 Color;
	float Intensity;
};

in vec2 TextureCoord;

out vec4 OutColor;

layout(binding = 0) uniform sampler2D gAlbedoRoughness; 
layout(binding = 1) uniform sampler2D gNormalMetallic;
layout(binding = 2) uniform sampler2D gPositionAO;

uniform vec3 u_CameraPos;
uniform int u_ActiveLights;

// MAX_LIGHTS is injected via ShaderMacros
uniform PointLight u_PointLights[MAX_LIGHTS];

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

	vec3 gPosition = texture(gPositionAO, TextureCoord).rgb;
	vec3 gNormal = texture(gNormalMetallic, TextureCoord).rgb;
	vec3 albedo = texture(gAlbedoRoughness, TextureCoord).rgb;

	float metallic = texture(gNormalMetallic, TextureCoord).a;
	float u_Roughness = texture(gAlbedoRoughness, TextureCoord).a;
	float ao = texture(gPositionAO, TextureCoord).a;

	vec3 actualAlbedo = albedo;

	vec3 N = normalize(gNormal);
	vec3 V = normalize(u_CameraPos - gPosition);

	// Clamp roughness to avoid NDF collapsing to 0 (produces flat ambient-only result)
	float roughness = max(u_Roughness, 0.05);

	vec3 L0 = vec3(0.0);
	for (int i = 0; i < u_ActiveLights; i++)
	{
		vec3 L = normalize(u_PointLights[i].Position - gPosition);
		vec3 H = normalize(V + L);

		float distance = length(u_PointLights[i].Position - gPosition);
		float attenuation = 1.0 / (distance * distance);
		vec3 radiance = u_PointLights[i].Color * u_PointLights[i].Intensity * attenuation;

		// BRDF (Cook-Torrance)
		float NdotL = max(dot(N, L), 0.0);
		vec3 F0 = mix(vec3(0.04), actualAlbedo, metallic);
		float D = NormalDistributionTrowbridgeReitxGGX(N, H, roughness);
		float G = max(GeometrySchlickGGXSub(N, V, roughness), 0.0) * max(GeometrySchlickGGXSub(N, L, roughness), 0.0);
		vec3 F = Fresnel(V, H, F0);

		vec3 KS = F;				// Specular factor
		vec3 KD = vec3(1.0) - KS;	// diffuse factor
		KD *= 1.0f - metallic;

		vec3 numerator = D * G * F;
		float denomenator = 4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.0001;
		vec3 specular =  numerator / denomenator;
		
		L0 += (KD * actualAlbedo / PI + specular) * radiance * NdotL;
	}

	// Ambient light
	vec3 ambient = vec3(DEFAULT_AMBIENT) * actualAlbedo * ao;
    vec3 color = ambient + L0;
	
	// HDR Tonemapping & Gamma Correction
    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0/2.2));  
   
    OutColor = vec4(color, 1.0);
}