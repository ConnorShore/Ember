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

struct DirectionalLight {
	vec3 Direction;
	vec3 Color;
	float Intensity;
};

struct SpotLight {
	vec3 Position;
	vec3 Direction;
	vec3 Color;
	float Intensity;
	float CutOff;		// Cosine of the cutoff angle
	float OuterCutOff;  // Cosine of the outer cutoff angle
};

struct PointLight {
	vec3 Position;
	vec3 Color;
	float Intensity;
};

in vec2 TextureCoord;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;

layout(binding = 0) uniform sampler2D gAlbedoRoughness; 
layout(binding = 1) uniform sampler2D gNormalMetallic;
layout(binding = 2) uniform sampler2D gPositionAO;
layout(binding = 3) uniform sampler2D directionShadowMap;
layout(binding = 4) uniform sampler2D spotShadowMap;

uniform vec3 u_CameraPos;

layout(std140, binding = 1) uniform ShadowData
{
	mat4 u_DirectionalLightViewMat;
	mat4 u_SpotLightViewMat;
};

uniform int u_ActiveDirectionalLights;
uniform int u_ActiveSpotLights;
uniform int u_ActivePointLights;

// MAX values are injected via ShaderMacros
uniform DirectionalLight u_DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
uniform SpotLight u_SpotLights[MAX_SPOT_LIGHTS];
uniform PointLight u_PointLights[MAX_POINT_LIGHTS];

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

float CalculateShadow(vec4 posLightSpace, sampler2D shadowMap, float bias)
{
	if (posLightSpace.w <= 0.0)
        return 0.0;

	// perform perspective divide
    vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
	
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
	
	// If it's outside the light's frustum entirely, it is NOT in shadow!
	if(projCoords.z > 1.0 || projCoords.x > 1.0 || projCoords.x < 0.0 || projCoords.y > 1.0 || projCoords.y < 0.0)
        return 0.0;

    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

	// PCF
	float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(shadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
			vec2 clampedUV = clamp(projCoords.xy + vec2(x, y) * texelSize, 0.0, 1.0);
            float pcfDepth = texture(shadowMap, clampedUV).r; 
            shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
        }    
    }
    shadow /= 9.0;

    return shadow;
}

vec3 ApplyDirectionalLighting(vec3 gPosition, vec3 gNormal, vec3 V, vec3 N, vec3 actualAlbedo, float metallic, float roughness)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < u_ActiveDirectionalLights; i++)
	{
		vec3 L = normalize(-u_DirectionalLights[i].Direction);
		vec3 H = normalize(V + L);

		float dirBias = max(0.005 * (1.0 - dot(N, L)), 0.0005);
		
		// Set shadow value
		vec4 PosLightSpace = u_DirectionalLightViewMat * vec4(gPosition, 1.0);
		float shadow = CalculateShadow(PosLightSpace, directionShadowMap, dirBias);

		float attenuation = 1.0;
		vec3 radiance = u_DirectionalLights[i].Color * u_DirectionalLights[i].Intensity * attenuation;

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

		result += (1.0 - shadow) * (KD * actualAlbedo / PI + specular) * radiance * NdotL;
	}

	return result;
}

vec3 ApplySpotLighting(vec3 gPosition, vec3 gNormal, vec3 V, vec3 N, vec3 actualAlbedo, float metallic, float roughness)
{
	vec3 result = vec3(0.0);

	// Spot Lights
	for (int i = 0; i < u_ActiveSpotLights; i++)
	{
		vec3 L = normalize(u_SpotLights[i].Position - gPosition);
		vec3 H = normalize(V + L);

		float theta = dot(L, normalize(-u_SpotLights[i].Direction));
        
		float epsilon = u_SpotLights[i].CutOff - u_SpotLights[i].OuterCutOff;
		float intensity = clamp((theta - u_SpotLights[i].OuterCutOff) / epsilon, 0.0, 1.0);

		// We only calculate light if we are inside the OUTER cutoff now
		if(theta > u_SpotLights[i].OuterCutOff) 
		{
			float spotBias = max(0.0005 * (1.0 - dot(N, L)), 0.00005);

			vec4 PosLightSpace = u_SpotLightViewMat * vec4(gPosition, 1.0);
			float shadow = CalculateShadow(PosLightSpace, spotShadowMap, spotBias);

			float distance = length(u_SpotLights[i].Position - gPosition);
			float attenuation = 1.0 / (distance * distance);
			vec3 radiance = u_SpotLights[i].Color * u_SpotLights[i].Intensity * attenuation * intensity;

			// BRDF (Cook-Torrance)
			float NdotL = max(dot(N, L), 0.0);
			vec3 F0 = mix(vec3(0.04), actualAlbedo, metallic);
			float D = NormalDistributionTrowbridgeReitxGGX(N, H, roughness);
			float G = max(GeometrySchlickGGXSub(N, V, roughness), 0.0) * max(GeometrySchlickGGXSub(N, L, roughness), 0.0);
			vec3 F = Fresnel(V, H, F0);

			vec3 KS = F;
			vec3 KD = vec3(1.0) - KS;
			KD *= 1.0f - metallic;

			vec3 numerator = D * G * F;
			float denomenator = 4.0 * max(dot(V, N), 0.0) * max(dot(L, N), 0.0) + 0.0001;
			vec3 specular =  numerator / denomenator;
			
			result += (1.0 - shadow) * (KD * actualAlbedo / PI + specular) * radiance * NdotL;
		}
	}

	return result;
}

vec3 ApplyPointLighting(vec3 gPosition, vec3 gNormal, vec3 V, vec3 N, vec3 actualAlbedo, float metallic, float roughness)
{
	vec3 result = vec3(0.0);

	// Point Lights
	for (int i = 0; i < u_ActivePointLights; i++)
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
		
		result += (KD * actualAlbedo / PI + specular) * radiance * NdotL;
	}

	return result;
}

void main()
{	
	vec3 gPosition = texture(gPositionAO, TextureCoord).rgb;
	vec3 gNormal = texture(gNormalMetallic, TextureCoord).rgb;
	if (length(gNormal) < 0.1) {
		OutColor = vec4(texture(gAlbedoRoughness, TextureCoord).rgb, 1.0);
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 albedo = texture(gAlbedoRoughness, TextureCoord).rgb;

	float metallic = texture(gNormalMetallic, TextureCoord).a;
	float roughness = texture(gAlbedoRoughness, TextureCoord).a;
	float ao = texture(gPositionAO, TextureCoord).a;

	vec3 actualAlbedo = albedo;

	vec3 N = normalize(gNormal);
	vec3 V = normalize(u_CameraPos - gPosition);

	// Clamp roughness to avoid NDF collapsing to 0 (produces flat ambient-only result)
	roughness = max(roughness, 0.05);

	// Apply lighting from each light type
	vec3 L0 = vec3(0.0);
	L0 += ApplyDirectionalLighting(gPosition, gNormal, V, N, actualAlbedo, metallic, roughness);
	L0 += ApplySpotLighting(gPosition, gNormal, V, N, actualAlbedo, metallic, roughness);
	L0 += ApplyPointLighting(gPosition, gNormal, V, N, actualAlbedo, metallic, roughness);

	// Ambient light
	vec3 ambient = vec3(DEFAULT_AMBIENT) * actualAlbedo * ao;
    vec3 color = ambient + L0;
   
    OutColor = vec4(color, 1.0);

	// Extract bright areas for hdr / bloom
	float brightness = dot(OutColor.rgb, vec3(0.2126, 0.7152, 0.0722));
	if(brightness > 1.0)
		BrightColor = vec4(OutColor.rgb, 1.0);
	else
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}