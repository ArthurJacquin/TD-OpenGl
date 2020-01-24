#version 420

struct Light{
	vec3 pos;		 // Position de la source
	vec3 dirToLight; // Direction du point VERS la lumière
	vec3 color;		 // Couleur
	float Ia;		 // Intensité AMBIENTE
	vec3 Id;		 // Intensité DIFFUSE
	vec3 Is;		 // Intensité SPECULAIRE
};

struct Material{
	vec3 Ka;		// Couleur diffuse
	vec3 Kd;		// Couleur diffuse
	vec3 Ks;		// Couleur spéculaire
	int shininess;  // Réflections
};

varying vec3 v_fragPos;
varying vec2 v_texcoords;
varying vec3 v_normals;

uniform Light u_light;
uniform Material u_mat;
uniform sampler2D u_TextureSampler;
uniform vec3 u_camPos;

void main(void)
{
	vec4 texColor = texture2D(u_TextureSampler,	v_texcoords); 
	//vec4 fragColor = texColor; //Base color of the frag
	vec4 fragColor = vec4(1.0, 0.0, 0.0, 1.0);

	//Light
	vec3 dirToLight = normalize(u_light.pos - v_fragPos);
	
	//Ambient
	vec4 ambient = vec4(1.0, 0.0, 0.0, 1.0);

	//Diffuse
	vec4 diffuseColor = vec4(dot(v_normals, dirToLight) * u_light.Id * u_mat.Kd, 1.0f);
	
	//Speculaire
	vec3 dirToCam = normalize(u_camPos - v_fragPos);
	vec3 H = normalize(dirToLight + dirToCam);
	float spec = max(pow(dot(dirToCam, H), u_mat.shininess), 0.0);
	vec4 specularColor = vec4(spec * u_light.Is * u_mat.Ks, 1.0f);

	gl_FragColor = fragColor;	
}