#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube daySkybox;
uniform samplerCube nightSkybox;
uniform float blendFactor;
uniform float time;
uniform float sunHeight;
uniform vec3 sunDir;
uniform float cloudCoverage;
uniform float cloudSpeed;
uniform float cloudCrispiness;
uniform float cloudCurliness;
uniform float cloudDensity;
uniform float rainIntensity;

const bool useProceduralClouds = true;
const bool useRepoInspiredDaySky = true;

float hash(vec2 p)
{
	p = fract(p * vec2(123.34, 456.21));
	p += dot(p, p + 45.32);
	return fract(p.x * p.y);
}

float noise(vec2 p)
{
	vec2 i = floor(p);
	vec2 f = fract(p);
	f = f * f * (3.0 - 2.0 * f);

	float a = hash(i);
	float b = hash(i + vec2(1.0, 0.0));
	float c = hash(i + vec2(0.0, 1.0));
	float d = hash(i + vec2(1.0, 1.0));

	return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p)
{
	float value = 0.0;
	float amplitude = 0.5;
	for (int i = 0; i < 4; ++i)
	{
		value += noise(p) * amplitude;
		p = p * 2.03 + vec2(13.1, 7.7);
		amplitude *= 0.5;
	}
	return value;
}

float starLayer(vec2 p, float scale, float threshold, float sizeBoost)
{
	vec2 cell = floor(p * scale);
	vec2 local = fract(p * scale) - 0.5;
	float seed = hash(cell);
	float active = smoothstep(threshold, 1.0, seed);
	float radius = length(local);
	float core = smoothstep(0.055 * sizeBoost, 0.0, radius);
	return active * core;
}

float remap01(float value, float minValue, float maxValue)
{
	return clamp((value - minValue) / (maxValue - minValue), 0.0, 1.0);
}

void main()
{
	vec4 dayColor = texture(daySkybox, TexCoords);
	vec4 nightColor = texture(nightSkybox, TexCoords);

	vec3 dir = normalize(TexCoords);
	vec3 skySunDir = normalize(sunDir);
	float rain = clamp(rainIntensity, 0.0, 1.0);
	vec4 proceduralDayColor = dayColor;
	vec4 proceduralNightColor = nightColor;
	float goldenHour = 0.0;

	if (useRepoInspiredDaySky)
	{
		float highSunBlend = remap01(sunHeight, 0.18, 0.82);
		vec3 highSunTop = vec3(0.42, 0.62, 0.84);
		vec3 highSunBottom = vec3(0.76, 0.83, 0.90);
		vec3 sunsetTop = vec3(133.0, 158.0, 214.0) / 255.0;
		vec3 sunsetBottom = vec3(241.0, 161.0, 161.0) / 255.0;

		vec3 skyTop = mix(sunsetTop, highSunTop, highSunBlend);
		vec3 skyBottom = mix(sunsetBottom, highSunBottom, highSunBlend);
		float gradient = clamp(1.0 - exp(8.5 - 17.0 * clamp(dir.y * 0.5 + 0.5, 0.0, 1.0)), 0.0, 1.0);
		vec3 proceduralSky = mix(skyBottom, skyTop, gradient);
		float sunGlow = pow(max(dot(dir, skySunDir), 0.0), 24.0) * smoothstep(-0.12, 0.45, sunHeight) * (1.0 - rain * 0.86);
		float horizonHaze = 1.0 - smoothstep(0.02, 0.34, abs(dir.y));
		float zenith = smoothstep(0.62, 1.0, dir.y * 0.5 + 0.5);
		float duskWarmth = (1.0 - highSunBlend) * smoothstep(-0.16, 0.24, sunHeight);
		goldenHour = (1.0 - highSunBlend) * smoothstep(0.10, 0.68, sunHeight) * (1.0 - rain * 0.65);
		float wideSunGlow = pow(max(dot(dir, skySunDir), 0.0), 4.0) * smoothstep(-0.10, 0.55, sunHeight) * (1.0 - rain * 0.80);
		proceduralSky += mix(vec3(1.0, 0.58, 0.36), vec3(1.0, 0.80, 0.52), highSunBlend) * sunGlow * 0.32;
		proceduralSky += vec3(0.26, 0.12, 0.065) * horizonHaze * duskWarmth * 0.36;
		proceduralSky += vec3(1.0, 0.68, 0.32) * wideSunGlow * goldenHour * 0.26;
		proceduralSky = mix(proceduralSky, vec3(0.95, 0.77, 0.52), horizonHaze * goldenHour * 0.48);
		proceduralSky = mix(proceduralSky, vec3(0.68, 0.80, 0.92), horizonHaze * 0.08 * highSunBlend);
		proceduralSky = mix(proceduralSky, proceduralSky * vec3(0.82, 0.96, 1.22), zenith * highSunBlend * 0.44);
		vec3 stormSky = mix(vec3(0.18, 0.21, 0.26), vec3(0.31, 0.36, 0.43), clamp(dir.y * 0.5 + 0.5, 0.0, 1.0));
		stormSky += vec3(0.05, 0.055, 0.06) * horizonHaze;
		proceduralSky = mix(proceduralSky, stormSky, rain * 0.76);
		proceduralDayColor = vec4(proceduralSky, 1.0);
	}

	vec2 starUv = vec2(atan(dir.z, dir.x) / 6.2831853 + 0.5, asin(clamp(dir.y, -1.0, 1.0)) / 3.1415926 + 0.5);
	float twinkle = 0.75 + 0.25 * sin(time * 0.55 + dir.x * 43.0 + dir.z * 31.0);
	float denseStars =
		starLayer(starUv + vec2(0.0, time * 0.0002), 180.0, 0.962, 1.10) * 1.10 +
		starLayer(starUv + vec2(0.17, -0.11), 260.0, 0.972, 0.92) * 1.00 +
		starLayer(starUv + vec2(-0.23, 0.09), 340.0, 0.982, 0.78) * 0.88 +
		starLayer(starUv + vec2(0.31, 0.27), 420.0, 0.989, 0.64) * 0.62;
	float hazeStars = smoothstep(0.66, 0.96, fbm(starUv * 95.0 + vec2(dir.y * 14.0, dir.x * 11.0))) * 0.30;
	float stars = (denseStars + hazeStars) * twinkle * 1.22;
	float starVisibility = smoothstep(-0.02, -0.48, sunHeight) * smoothstep(-0.28, 0.18, dir.y);

	float nightAmount = remap01(-sunHeight, 0.08, 0.92) * (1.0 - rain * 0.55);
	vec3 nightBase = mix(vec3(0.006, 0.012, 0.036), vec3(0.045, 0.070, 0.145), clamp(dir.y * 0.5 + 0.5, 0.0, 1.0));
	float nightNebula = fbm(starUv * 18.0 + vec2(0.0, time * 0.00008));
	float upperNight = smoothstep(0.36, 1.0, dir.y * 0.5 + 0.5);
	vec3 moonDir = normalize(-skySunDir + vec3(0.0, 0.18, 0.0));
	float moonDot = max(dot(dir, moonDir), 0.0);
	float moonAura = pow(moonDot, 7.0) * nightAmount;
	float moonCore = pow(moonDot, 150.0) * nightAmount;
	float horizonNightGlow = (1.0 - smoothstep(0.0, 0.32, abs(dir.y))) * nightAmount;
	nightBase += vec3(0.030, 0.040, 0.075) * smoothstep(0.52, 0.88, nightNebula) * 0.60 * nightAmount;
	nightBase += vec3(0.070, 0.105, 0.220) * moonAura * 0.28;
	nightBase += vec3(0.78, 0.86, 1.0) * moonCore * 0.85;
	nightBase += vec3(0.040, 0.055, 0.110) * horizonNightGlow;
	nightBase = mix(nightBase, nightBase * vec3(0.90, 0.98, 1.18), upperNight * nightAmount * 0.25);
	nightBase += vec3(1.0, 0.98, 0.96) * stars * starVisibility * 1.10;
	proceduralNightColor = vec4(nightBase, 1.0);

	vec4 skyColor = mix(proceduralDayColor, proceduralNightColor, clamp(blendFactor, 0.0, 1.0));
	float cityLightDome = (1.0 - smoothstep(0.0, 0.28, abs(dir.y))) * clamp(blendFactor, 0.0, 1.0);
	skyColor.rgb += vec3(0.16, 0.08, 0.045) * cityLightDome * 0.30;
	skyColor.rgb += vec3(0.025, 0.040, 0.090) * cityLightDome * nightAmount * 0.45;
	skyColor.rgb = mix(skyColor.rgb, vec3(0.10, 0.12, 0.16), rain * clamp(blendFactor, 0.0, 1.0) * 0.52);

	float cloudBand = smoothstep(-0.12, 0.46, dir.y) * (1.0 - smoothstep(0.90, 0.995, dir.y));
	float dayCloudVisibility = (1.0 - clamp(blendFactor, 0.0, 1.0) * 0.75) * smoothstep(-0.26, 0.20, sunHeight);
	dayCloudVisibility = max(dayCloudVisibility, rain * 0.82);

	if (useProceduralClouds && dayCloudVisibility > 0.001 && cloudBand > 0.0)
	{
		vec2 cloudUv = dir.xz / max(dir.y + 0.32, 0.16);
		vec2 windDir = normalize(vec2(0.92, 0.22));
		float speed = max(cloudSpeed, 0.05);
		float crispiness = max(cloudCrispiness, 0.35);
		float curliness = max(cloudCurliness, 0.10);
		float densityBoost = max(cloudDensity, 0.20);
		vec2 windLarge = windDir * (0.0045 * speed);
		vec2 windMedium = windDir * (0.0028 * speed) + vec2(-0.0009, 0.0011) * speed;
		vec2 windDetail = windDir * (0.0018 * speed) + vec2(0.0006, -0.0008) * speed;

		vec2 anisotropicUv = vec2(dot(cloudUv, windDir), dot(cloudUv, vec2(-windDir.y, windDir.x)) * (0.42 + curliness * 0.22));
		float distortionX = fbm(anisotropicUv * 0.32 + time * vec2(0.0007, 0.0002));
		float distortionY = fbm(anisotropicUv * 0.28 - time * vec2(0.0004, 0.0005));
		vec2 distortedUv = anisotropicUv + (vec2(distortionX, distortionY) - 0.5) * (0.45 + curliness * 0.45);

		float largeShape = fbm(distortedUv * (0.72 * crispiness) + time * windLarge);
		float mediumShape = fbm(distortedUv * (1.45 * crispiness) + time * windMedium);
		float detailShape = fbm(distortedUv * (3.20 * crispiness) + time * windDetail);
		float wisps = fbm(vec2(distortedUv.x * 1.8, distortedUv.y * (5.5 + curliness * 3.0)) + time * vec2(0.0020, 0.0004) * speed);

		float cloudShape = largeShape * 0.60 + mediumShape * 0.24 + detailShape * 0.08 + wisps * 0.08;
		float coverage = mix(0.38, 0.46, remap01(sunHeight, -0.08, 0.45)) - (cloudCoverage - 0.5) * 0.28;
		float cloudMask = smoothstep(coverage, coverage + 0.14, cloudShape) * cloudBand;
		float cloudCore = smoothstep(coverage + 0.02, coverage + 0.18, cloudShape);
		float cloudRim = smoothstep(coverage - 0.02, coverage + 0.12, cloudShape) - cloudCore;

		float lightFacing = smoothstep(-0.18, 0.32, dot(dir, skySunDir));
		float warmSun = remap01(sunHeight, -0.02, 0.30);
		float highSun = remap01(sunHeight, 0.25, 0.85);

		vec3 highlightColor = mix(vec3(1.0, 0.82, 0.66), vec3(0.98, 0.99, 0.96), highSun);
		vec3 bodyColor = mix(vec3(0.70, 0.74, 0.84), vec3(0.84, 0.88, 0.92), highSun);
		vec3 shadowColor = mix(vec3(97.0, 98.0, 120.0) / 255.0, vec3(65.0, 70.0, 80.0) * (1.5 / 255.0), highSun);
		float goldenCloud = goldenHour * (0.45 + lightFacing * 0.55);
		highlightColor = mix(highlightColor, vec3(1.0, 0.78, 0.48), goldenCloud * 0.65);
		bodyColor = mix(bodyColor, vec3(0.86, 0.74, 0.58), goldenHour * 0.35);
		highlightColor = mix(highlightColor, vec3(0.48, 0.53, 0.58), rain * 0.80);
		bodyColor = mix(bodyColor, vec3(0.30, 0.35, 0.40), rain * 0.88);
		shadowColor = mix(shadowColor, vec3(0.13, 0.16, 0.20), rain * 0.92);

		float selfShadow = remap01(cloudShape, coverage + 0.01, coverage + 0.14);
		vec3 cloudColor = mix(shadowColor, bodyColor, selfShadow);
		cloudColor = mix(cloudColor, highlightColor, cloudRim * (0.25 + lightFacing * 0.75) * (0.45 + warmSun * 0.55));

		float cloudOpacity = cloudMask * (0.92 + cloudCore * 0.26 + rain * 0.34) * dayCloudVisibility * densityBoost;
		cloudOpacity = clamp(cloudOpacity, 0.0, 1.0);
		skyColor.rgb = mix(skyColor.rgb, cloudColor, cloudOpacity);
		skyColor.rgb += highlightColor * cloudRim * lightFacing * 0.28 * dayCloudVisibility;
	}

	FragColor = vec4(clamp(skyColor.rgb, 0.0, 1.0), skyColor.a);
}
