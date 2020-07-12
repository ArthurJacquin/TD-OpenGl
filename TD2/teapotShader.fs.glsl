#version 420
#extension GL_NV_shadow_samplers_cube : enable

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

varying vec4 ShadowCoord;

uniform Light u_light;
uniform Material u_mat;
uniform sampler2D u_TextureSampler;
uniform vec3 u_camPos;
uniform samplerCube u_SkyTexture;
uniform sampler2D u_ShadowMap;


vec3 FresnelSchlick(vec3 f0, float cosTheta) {
	return f0 + (vec3(1.0) - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 Reflectance(vec3 albedo, float metallic)
{
	return mix(vec3(0.04), albedo, metallic);
}

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // perform perspective divide
    vec3 projCoords = ShadowCoord.xyz / ShadowCoord.w;
    // transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;
    // get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r; 
    // get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    // check whether current frag pos is in shadow
    float shadow = currentDepth > closestDepth  ? 1.0 : 0.0;

	return shadow;
}

void main(void)
{
	vec4 texColor = texture2D(u_TextureSampler, v_texcoords); 
	vec4 fragColor = texColor; //Base color of the frag
	
	//Light
	vec3 dirToLight = normalize(u_light.pos - v_fragPos);
	
	//Ambient
	vec3 ambient = vec3(u_light.Ia * u_mat.Ka * u_light.color);
	
	//Diffuse
	vec3 diffuseColor = vec3(max(dot(v_normals, dirToLight) * u_light.Id * u_mat.Kd, 0.0));
	
	//Speculaire
	vec3 dirToCam = normalize(u_camPos - v_fragPos);
	vec3 H = normalize(dirToLight + dirToCam);
	float spec = max(pow(dot(dirToCam, H), u_mat.shininess * 10), 0.0);
	vec3 specularColor = vec3(spec * u_light.Is * u_mat.Ks);
	
	vec3 R = reflect(dirToLight, v_normals);
	vec3 f0 = Reflectance(u_mat.Ka , u_mat.shininess);
	vec3 Fspec = FresnelSchlick(f0, dot(R, v_normals));
	vec3 Fdiff = vec3(1.0) - FresnelSchlick(f0, dot(v_normals, -dirToLight));
	vec3 color = (diffuseColor * Fdiff + specularColor * Fspec);
	
	vec3 I = normalize(u_camPos - v_fragPos);
	vec3 R1 = reflect(I, normalize(v_normals));
	vec3 indirectColor = vec3(textureCube(u_SkyTexture, R1).rgb);	
	
	//Shadow
	float shadow = ShadowCalculation(ShadowCoord);  
	
	//gl_FragColor = vec4((ambient + (indirectColor * 0.1) + diffuseColor + specularColor) * fragColor.xyz, 1.0);
	gl_FragColor = vec4((ambient + (1.0 - shadow) * (diffuseColor + specularColor)) * fragColor.xyz, 1.0);
	//gl_FragColor = vec4(texture(u_ShadowMap, ShadowCoord.xy).r, 1.0);
}