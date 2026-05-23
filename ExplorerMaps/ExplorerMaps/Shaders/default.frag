#version 330 core

// Outputs colors in RGBA
out vec4 FragColor;

// Imports the normal from the Vertex Shader
in vec3 Normal;
// Imports the color from the Vertex Shader
in vec3 color;
// Imports the texture coordinates from the Vertex Shader
in vec2 texCoord;



// Gets the Texture Units from the main function
uniform sampler2D diffuse0;
// Gets the color of the light from the main function
uniform vec4 lightColor;

void main()
{
	vec3 normal = normalize(Normal);
	vec3 lightDirection = normalize(vec3(-0.45f, 1.0f, 0.35f));
	float lambert = max(dot(normal, lightDirection), 0.0f);
	float hemi = clamp(normal.y * 0.5f + 0.5f, 0.0f, 1.0f);
	float lighting = 0.68f + lambert * 0.32f + hemi * 0.18f;
	vec4 base = texture(diffuse0, texCoord) * vec4(color, 1.0f);
	vec3 litColor = base.rgb * lighting * lightColor.rgb;
	litColor = pow(max(litColor, vec3(0.0f)), vec3(1.0f / 1.35f));
	FragColor = vec4(litColor, base.a);
}
