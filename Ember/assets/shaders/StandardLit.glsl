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

// Alternating values to store in 16 byte aligned manner (vec3 + float)
struct DirectionalLight {
	vec3 Direction;
	float Intensity;
	vec3 Color;
};

// Alternating values to store in 16 byte aligned manner (vec3 + float)
struct SpotLight {
	vec3 Position;
	float Intensity;
	vec3 Direction;
	float CutOff;		// Cosine of the cutoff angle
	vec3 Color;
	float OuterCutOff;  // Cosine of the outer cutoff angle
};

// Alternating values to store in 16 byte aligned manner (vec3 + float)
struct PointLight {
	vec3 Position;
	float Intensity;
	vec3 Color;
	float Radius;
};

in vec2 TextureCoord;

layout(location = 0) out vec4 OutColor;
layout(location = 1) out vec4 BrightColor;

layout(binding = 0) uniform sampler2D u_AlbedoRoughness; 
layout(binding = 1) uniform sampler2D u_NormalMetallic;
layout(binding = 2) uniform sampler2D u_PositionAO;
layout(binding = 3) uniform sampler2D u_EmissionOut;
layout(binding = 4) uniform sampler2DArray u_DirectionShadowMap;
layout(binding = 5) uniform sampler2DArray u_SpotShadowMap;
layout(binding = 6) uniform samplerCube u_IrradianceMap;
layout(binding = 7) uniform samplerCube u_PrefilterMap;
layout(binding = 8) uniform sampler2D u_BRDFLUT;

uniform vec3 u_CameraPos;
uniform vec3 u_CameraForward;
uniform float u_EnvironmentIntensity;

layout(std140, binding = 1) uniform ShadowData
{
    mat4 u_DirectionalShadowMatrices[3];
    mat4 u_SpotLightMatrix;
    vec4 u_CascadeSplits;
};

layout(std140, binding = 2) uniform LightDataBlock
{
	DirectionalLight u_DirectionalLights[MAX_DIRECTIONAL_LIGHTS];
	SpotLight u_SpotLights[MAX_SPOT_LIGHTS];
	PointLight u_PointLights[MAX_POINT_LIGHTS];

	int u_ActiveDirectionalLights;
	int u_ActiveSpotLights;
	int u_ActivePointLights;
};

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

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

float CalculateShadow(vec4 posLightSpace, sampler2DArray shadowMap, float bias, float layer)
{
	if (posLightSpace.w <= 0.0)
		return 0.0;

	vec3 projCoords = posLightSpace.xyz / posLightSpace.w;
	projCoords = projCoords * 0.5 + 0.5;
	
	if(projCoords.z > 1.0 || projCoords.x > 1.0 || projCoords.x < 0.0 || projCoords.y > 1.0 || projCoords.y < 0.0)
		return 0.0;

	float currentDepth = projCoords.z;
	float shadow = 0.0;
	
	// A spread of 1.0 to 1.2 is usually best for a 5x5 kernel
	float pcfSpread = 1.0; 
	vec2 texelSize = (1.0 / vec2(textureSize(shadowMap, 0).xy)) * pcfSpread;
	
	// --- 5x5 PCF KERNEL ---
	int halfKernel = 2; 
	float sampleCount = 0.0;
	
	for(int x = -halfKernel; x <= halfKernel; ++x)
	{
		for(int y = -halfKernel; y <= halfKernel; ++y)
		{
			vec2 clampedUV = clamp(projCoords.xy + vec2(x, y) * texelSize, 0.0, 1.0);
			float pcfDepth = texture(shadowMap, vec3(clampedUV, layer)).r;
			shadow += currentDepth - bias > pcfDepth  ? 1.0 : 0.0;
			sampleCount += 1.0;
		}    
	}
	shadow /= sampleCount; // Divides by 25.0

	return shadow;
}

vec3 ApplyDirectionalLighting(vec3 gPosition, vec3 gNormal, vec3 V, vec3 N, vec3 actualAlbedo, float metallic, float roughness)
{
	vec3 result = vec3(0.0);
	for (int i = 0; i < u_ActiveDirectionalLights; i++)
	{
		vec3 L = normalize(-u_DirectionalLights[i].Direction);
		vec3 H = normalize(V + L);

		// --- CSM LAYER SELECTION ---
		// 1. Calculate how far this pixel is from the camera
		float depthValue = abs(dot(gPosition - u_CameraPos, u_CameraForward));

		// 2. Figure out which cascade this pixel belongs to using the splits!
		int layer = 2;
		if (depthValue < u_CascadeSplits.x)
			layer = 0;
		else if (depthValue < u_CascadeSplits.y)
			layer = 1;

		// --- CASCADE BLENDING LOGIC ---
		// Define how wide the transition zone is (e.g., blend over 3.0 world units)
		float blendDistance = 3.0; 
		float blendFactor = 0.0;
		int nextLayer = layer;

		// Calculate if we are close to the edge of our current cascade
		if (layer == 0) 
		{
			float distToSplit = u_CascadeSplits.x - depthValue;
			if (distToSplit < blendDistance) 
			{
				blendFactor = 1.0 - (distToSplit / blendDistance);
				nextLayer = 1;
			}
		} 
		else if (layer == 1) 
		{
			float distToSplit = u_CascadeSplits.y - depthValue;
			if (distToSplit < blendDistance) 
			{
				blendFactor = 1.0 - (distToSplit / blendDistance);
				nextLayer = 2;
			}
		}

		// --- PRIMARY SHADOW SAMPLE ---
		// 1. Scale the depth bias down for larger cascades!
		float depthBiases[3] = float[](0.0005, 0.00005, 0.000005);
		float constantDepthBias = depthBiases[layer];

		// 2. Scale the normal bias up for larger cascades (because pixels get physically wider!)
		float normalBiasMultipliers[3] = float[](0.05, 0.15, 0.5); 
		
		float normalOffset = normalBiasMultipliers[layer] * (1.0 - max(dot(N, L), 0.0));
		
		// Push the position!
		vec3 biasedPosition = gPosition + (N * normalOffset);
		vec4 PosLightSpace = u_DirectionalShadowMatrices[layer] * vec4(biasedPosition, 1.0);
		
		float shadow = CalculateShadow(PosLightSpace, u_DirectionShadowMap, constantDepthBias, float(layer));

		// --- SECONDARY BLEND SAMPLE ---
		if (blendFactor > 0.0) 
		{
			float nextConstantDepthBias = depthBiases[nextLayer]; // Get the correct depth bias!
			float nextNormalOffset = normalBiasMultipliers[nextLayer] * (1.0 - max(dot(N, L), 0.0));
			
			vec3 nextBiasedPosition = gPosition + (N * nextNormalOffset);
			vec4 nextPosLightSpace = u_DirectionalShadowMatrices[nextLayer] * vec4(nextBiasedPosition, 1.0);
			
			float nextShadow = CalculateShadow(nextPosLightSpace, u_DirectionShadowMap, nextConstantDepthBias, float(nextLayer));
			
			// Smoothly blend
			shadow = mix(shadow, nextShadow, blendFactor);
		}
		// ------------------------------

		float attenuation = 1.0;
		vec3 radiance = u_DirectionalLights[i].Color * u_DirectionalLights[i].Intensity * attenuation;

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

			vec4 PosLightSpace = u_SpotLightMatrix * vec4(gPosition, 1.0);
			float shadow = CalculateShadow(PosLightSpace, u_SpotShadowMap, spotBias, 0.0);

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
		if (distance > u_PointLights[i].Radius && (u_PointLights[i].Radius > 0)) 
			continue; // Skip if outside the light's radius (and radius is > 0, otherwise treat as infinite)

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
	vec3 gPosition = texture(u_PositionAO, TextureCoord).rgb;
	vec3 gNormal = texture(u_NormalMetallic, TextureCoord).rgb;
	if (length(gNormal) < 0.1) {
		OutColor = vec4(texture(u_AlbedoRoughness, TextureCoord).rgb, 1.0);
		BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
		return;
	}

	vec3 albedo = texture(u_AlbedoRoughness, TextureCoord).rgb;

	float metallic = texture(u_NormalMetallic, TextureCoord).a;
	float roughness = texture(u_AlbedoRoughness, TextureCoord).a;
	float ao = texture(u_PositionAO, TextureCoord).a;

	vec3 actualAlbedo = albedo;

	vec3 N = normalize(gNormal);
	vec3 V = normalize(u_CameraPos - gPosition);
    vec3 R = reflect(-V, N); 

	// Clamp roughness to avoid NDF collapsing to 0 (produces flat ambient-only result)
	roughness = max(roughness, 0.05);

	// Apply lighting from each light type
	vec3 L0 = vec3(0.0);
	L0 += ApplyDirectionalLighting(gPosition, gNormal, V, N, actualAlbedo, metallic, roughness);
	L0 += ApplySpotLighting(gPosition, gNormal, V, N, actualAlbedo, metallic, roughness);
	L0 += ApplyPointLighting(gPosition, gNormal, V, N, actualAlbedo, metallic, roughness);

	// Calculate Specular IBL (Reflections)
	const float MAX_REFLECTION_LOD = 7.0;	// This should match the max LOD level used when generating the prefilter map (7 = 128x128 for a 1024x1024 base resolution)
	vec3 prefilteredColor = textureLod(u_PrefilterMap, R, roughness * MAX_REFLECTION_LOD).rgb;    
	
	// Calculate Fresnel for ambient lighting
	vec3 F0 = mix(vec3(0.04), actualAlbedo, metallic);
	vec3 F = FresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);

	vec2 brdfUV = clamp(vec2(max(dot(N, V), 0.0), roughness), 0.001, 0.999);
	vec2 brdf = texture(u_BRDFLUT, brdfUV).rg;
	
	vec3 specular = prefilteredColor * (F * brdf.x + brdf.y);

	//Calc ulate Diffuse IBL (Ambient Light)
	vec3 kS = F;
	vec3 kD = 1.0 - kS;
	kD *= 1.0 - metallic; // Metals don't have diffuse light!
	
	vec3 diffuseAmbient = texture(u_IrradianceMap, N).rgb * actualAlbedo * kD;

	// Combine IBL and apply Intensity/AO
	vec3 ambient = (diffuseAmbient + specular) * ao;
	
	if (u_EnvironmentIntensity <= 0.0) {
		ambient = vec3(DEFAULT_AMBIENT) * actualAlbedo * ao; // Fallback
	} else {
		ambient *= u_EnvironmentIntensity;
	}

	// Final Composition
	vec3 color = ambient + L0;
	vec3 emission = texture(u_EmissionOut, TextureCoord).rgb;
	
	vec3 finalColor = color + emission;
	OutColor = vec4(finalColor, 1.0);

	// Extract bright areas for hdr / bloom
	BrightColor = vec4(max(OutColor.rgb - vec3(1.0), vec3(0.0)), 1.0);
}