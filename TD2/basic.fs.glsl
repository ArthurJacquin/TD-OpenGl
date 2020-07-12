#version 420

struct Light{
	vec3 pos;		 // Position de la source
	vec3 dirToLight; // Direction du point VERS la lumi�re
	vec3 color;		 // Couleur
	float Ia;		 // Intensit� AMBIENTE
	vec3 Id;		 // Intensit� DIFFUSE
	vec3 Is;		 // Intensit� SPECULAIRE
};

struct Material{
	vec3 Ka;		// Couleur diffuse
	vec3 Kd;		// Couleur diffuse
	vec3 Ks;		// Couleur sp�culaire
	int shininess;  // R�flections
};

varying vec3 v_fragPos;
varying vec2 v_texcoords;
varying vec3 v_normals;

uniform Light u_light;
uniform Material u_mat;
uniform sampler2D u_TextureSampler;
uniform vec3 u_camPos;
uniform samplerCube u_SkyTexture;

void main(void)
{
	vec4 texColor = texture2D(u_TextureSampler,	v_texcoords); 
	vec4 fragColor = texColor; //Base color of the frag
	
	//Light
	vec3 dirToLight = normalize(u_light.pos - v_fragPos);
	
	//Ambient
	vec4 ambient = vec4(u_light.Ia * u_mat.Ka * u_light.color, 1.0f);

	//Diffuse
	vec4 diffuseColor = vec4(max(dot(v_normals, dirToLight) * u_light.Id * u_mat.Kd, 0.0), 1.0f);
	
	//Speculaire
	vec3 dirToCam = normalize(u_camPos - v_fragPos);
	vec3 H = normalize(dirToLight + dirToCam);
	float spec = max(pow(dot(dirToCam, H), u_mat.shininess), 0.0);
	vec4 specularColor = vec4(spec * u_light.Is * u_mat.Ks, 1.0f);

	vec3 I = normalize(v_fragPos - u_camPos);
    vec3 R = reflect(I, normalize(v_normals));


	gl_FragColor = vec4(texture(u_SkyTexture, R).rgb, 1.0);	
}