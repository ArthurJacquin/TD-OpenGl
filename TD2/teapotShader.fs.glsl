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
uniform sampler2DShadow u_ShadowMap;

vec2 poissonDisk[16] = vec2[]( 
   vec2( -0.94201624, -0.39906216 ), 
   vec2( 0.94558609, -0.76890725 ), 
   vec2( -0.094184101, -0.92938870 ), 
   vec2( 0.34495938, 0.29387760 ), 
   vec2( -0.91588581, 0.45771432 ), 
   vec2( -0.81544232, -0.87912464 ), 
   vec2( -0.38277543, 0.27676845 ), 
   vec2( 0.97484398, 0.75648379 ), 
   vec2( 0.44323325, -0.97511554 ), 
   vec2( 0.53742981, -0.47373420 ), 
   vec2( -0.26496911, -0.41893023 ), 
   vec2( 0.79197514, 0.19090188 ), 
   vec2( -0.24188840, 0.99706507 ), 
   vec2( -0.81409955, 0.91437590 ), 
   vec2( 0.19984126, 0.78641367 ), 
   vec2( 0.14383161, -0.14100790 ) 
);

vec3 FresnelSchlick(vec3 f0, float cosTheta) {
	return f0 + (vec3(1.0) - f0) * pow(1.0 - cosTheta, 5.0);
}

vec3 Reflectance(vec3 albedo, float metallic)
{
	return mix(vec3(0.04), albedo, metallic);
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
	float visibility=1.0;
	float cosTheta = clamp(dot( v_normals, -dirToLight), 0.0, 1.0);
	float bias = 0.005*tan(acos(cosTheta));
	bias = clamp(bias, 0.0, 0.01);

	for (int i=0;i<4;i++){
		int index = i;
		visibility -= 0.2*(1.0-texture( u_ShadowMap, vec3(ShadowCoord.xy + poissonDisk[index]/700.0,  (ShadowCoord.z-bias)/ShadowCoord.w) ));
	}

	//gl_FragColor = vec4((ambient + (indirectColor * 0.1) + diffuseColor + specularColor) * fragColor.xyz, 1.0);
	gl_FragColor = vec4( (ambient  + visibility * (diffuseColor + specularColor) ) * fragColor.xyz, 1.0);
}