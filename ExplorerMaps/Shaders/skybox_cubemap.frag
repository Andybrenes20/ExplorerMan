#version 330 core

out vec4 FragColor;

in vec3 TexCoords;

uniform samplerCube daySkybox;
uniform samplerCube nightSkybox;
uniform float blendFactor;

void main()
{
	vec4 dayColor = texture(daySkybox, TexCoords);
	vec4 nightColor = texture(nightSkybox, TexCoords);
	FragColor = mix(dayColor, nightColor, clamp(blendFactor, 0.0, 1.0));
}
