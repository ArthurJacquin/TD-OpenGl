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
	vec4 fragColor = texColor; //Base color of the frag
	v_normals = normalize(v_normals);
	
	//Light
	u_light.dirToLight = normalize(u_light.pos - v_fragPos);
	
	//Ambient
	u_light.Ia = 1.0f;
	vec4 ambient = vec4(u_light.Ia * u_mat.Ka * u_light.color, 1.0f);

	//Diffuse
	u_light.Id = vec3(1.0f, 1.0f, 1.0f);
	vec4 diffuseColor = vec4(dot(v_normals, u_light.dirToLight) * u_light.Id * u_mat.Kd, 1.0f);
	
	//Speculaire
	u_light.Is = vec3(1.0f, 1.0f, 1.0f);
	vec3 dirToCam = normalize(u_camPos - v_fragPos);
	vec3 H = normalize(u_light.dirToLight + dirToCam);
	vec3 spec = max(pow(dot(dirToCam, H), u_mat.shininess), 0.0);
	vec4 specularColor = vec4(spec * u_light.Is * u_mat.Ks, 1.0f);

	gl_FragColor = (ambient + diffuseColor + specularColor) * fragColor;	
}