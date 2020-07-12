#pragma region Includes

#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "glew32s.lib")			
#pragma comment(lib, "opengl32.lib")
#elif defined(__APPLE__)
#elif defined(__linux__)
#endif

#include <iostream>
#include <vector>
#include <math.h>

#include "Vertex.h"
#include "Matrix4.h"
#include "../common/GLShader.h"
#include "Mesh.h"

// fonctions de chargement d'images (bitmap)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tinyobjloader/tiny_obj_loader.h"

#define PI 3.14159265359

using namespace std;

#pragma endregion

#pragma region Variables

// variables globales (dans le cadre de cet exemple, evitez les globales si possible)
GLShader g_basicShader;
GLShader g_teapotShader;
GLuint g_TextureObject;
GLuint g_TextureObjectTeapot;
GLuint texturesID[3];

uint32_t program;
uint32_t programTP;

//Buffers
GLuint VAO;
GLuint PVAO;
GLuint VAOTP;
GLuint FBO;

//Buffers 2D
GLuint VAO2D;
GLuint VBO2D;
GLShader g_2DShader;

//Load colorBuffer
GLuint colorBufferTexture;

Mesh mesh;
Mesh teapot;
Mesh plane;

float currentTime;

GLFWwindow* window;
int windowWidth = 800;
int windowHeight = 600;

//Camera position
Vec3 camPos(0.f, 0.f, 8.f);
float lastCamX = windowWidth / 2;
float lastCamY = windowHeight / 2;
float pitch = 0;
float yaw = 0;
bool firstMouse = true;
float FOV = 60;
int b;
bool activePP = false;

const int shadowmap_resolution = 1024;
int modelMatrixLocation;
int viewMatrixLocation;
int projectionMatrixLocation;
int cameraPos_location;
int depthMVPMatrixLocation;
int shadowMapLocation;
int texture_location;

int modelMatrixLocationTP;
int viewMatrixLocationTP;
int projectionMatrixLocationTP;
int cameraPos_locationTP;

//Cubemap
uint32_t cubeMapID;
GLShader g_skyboxShader;
GLuint skyboxVAO, skyboxVBO;

Matrix4 modelMatrix;
Matrix4 projectionMatrix;
Matrix4 viewMatrix;

Matrix4 depthMVP;
Matrix4 projectionMatrixLight;
Matrix4 viewMatrixLight;
Vec3 lightPosition = Vec3(0.5f, 2.0f, 2.0f);
GLuint shadowmapTexture;
GLuint shadowmapFBO;

#pragma endregion

void DestroyTexture(GLuint* textureID)
{
	glDeleteTextures(1, textureID);
	textureID = 0;
}

void SetUniform(GLShader &shader)
{
	uint32_t program = shader.GetProgram();

	//------------------------------------------------- Position de la lumiere-----------------------------------------------
	int lightPos_location = glGetUniformLocation(program, "u_light.pos");
	glUniform3f(lightPos_location, lightPosition.x, lightPosition.y, lightPosition.z);
	int lightColor_location = glGetUniformLocation(program, "u_light.color");
	glUniform3f(lightColor_location, 1.0f, 1.0f, 1.0f);

	//-------------------------------------------------------Param�tre lumi�re------------------------------------------------
	int lightPos_Ia = glGetUniformLocation(program, "u_light.Ia");
	glUniform1f(lightPos_Ia, 0.1f);
	//ambient
	int lightPos_Id = glGetUniformLocation(program, "u_light.Id");
	glUniform3f(lightPos_Id, 1.0f, 1.0f, 1.0f);
	//specular
	int lightPos_Is = glGetUniformLocation(program, "u_light.Is");
	glUniform3f(lightPos_Is, 1.0f, 1.0f, 1.0f);

	// -------------------------------------------------------Materiau--------------------------------------------------------
	//Ambient
	int matKa_location = glGetUniformLocation(program, "u_mat.Ka");
	glUniform3f(matKa_location, mesh.mat.Ka.x, mesh.mat.Ka.y, mesh.mat.Ka.z);
	//Diffuse
	int matKd_location = glGetUniformLocation(program, "u_mat.Kd");
	glUniform3f(matKd_location, mesh.mat.Kd.x, mesh.mat.Kd.y, mesh.mat.Kd.z);
	//Sp�culaire
	int matKs_location = glGetUniformLocation(program, "u_mat.Ks");
	glUniform3f(matKs_location, mesh.mat.Ks.x, mesh.mat.Ks.y, mesh.mat.Ks.z);
	//Reflection
	int shininessPos_location = glGetUniformLocation(program, "u_mat.shininess");
	glUniform1i(shininessPos_location, mesh.mat.shininess);

}

void Shutdown()
{
	DestroyTexture(&texturesID[0]);
	DestroyTexture(&texturesID[1]);
	DestroyTexture(&texturesID[2]);
	glDeleteTextures(1, &cubeMapID);

	glDeleteFramebuffers(1, &FBO);

	g_basicShader.Destroy();
	g_teapotShader.Destroy();
	g_2DShader.Destroy();
	g_skyboxShader.Destroy();
}

#pragma region Load_Functions

GLuint LoadTexture(const char* path)
{
	// 1. chargement de la bitmap
	GLuint texture;
	int w, h, c;
	uint8_t* data = stbi_load(path, &w, &h, &c, STBI_rgb_alpha);
	// 2. creation du texture object OpenGL
	glGenTextures(1, &texture);
	// 3. chargement et parametrage
	// pour pouvoir travailler sur/avec la texture
	// on doit d'abord la "bind" / attacher sur un identifiant
	// en l'occurence GL_TEXTURE_2D
	glBindTexture(GL_TEXTURE_2D, texture);
	// les 6 premiers params definissent le stockage de la texture en VRAM (memoire video)
	// les 3 derniers specifient l'image source
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	// on remet la valeur par defaut (pas obligatoire mais preferable ici)
	//glBindTexture(GL_TEXTURE_2D, index);
	// 4. liberation de la memoire
	stbi_image_free(data);
	return texture;
}

void loadModel(const string path, Mesh& meshes)
{
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	//Load file
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
		throw std::runtime_error(warn + err);
	}

	//Fill mesh arrays with datas
	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex;
			
			//Position
			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			//Texture coordinates,
			//if texcoord_index == -1 : No texCoords in the obj file
			if(index.texcoord_index != -1)
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
				};

			//Normals
			//if normal_index == -1 : No normals in the obj file
			if (index.normal_index != -1)
				vertex.normal = {
					attrib.normals[3 * index.normal_index + 0],
					attrib.normals[3 * index.normal_index + 1],
					attrib.normals[3 * index.normal_index + 2]
				};

			//Add vertices and indices to the mesh
			meshes.indices.push_back(meshes.indices.size());
			meshes.vertices.push_back(vertex);
		}
	}
	meshes.vertexCount = meshes.vertices.size() - 1;

	for (size_t f = 0; f < materials.size(); f++)
	{
		meshes.mat.Ka = { materials[f].ambient[0],
						materials[f].ambient[1],
						materials[f].ambient[2] };

		meshes.mat.Kd = { materials[f].diffuse[0],
						materials[f].diffuse[1],
						materials[f].diffuse[2] };

		meshes.mat.Ks = { materials[f].specular[0],
						materials[f].specular[1],
						materials[f].specular[2] };

		meshes.mat.shininess = materials[f].shininess;
	}
}

//Cubemap
uint32_t LoadCubemap(const char* pathes[6])
{
	unsigned int cubemapTexture;
	glGenTextures(1, &cubemapTexture);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

	int width, height, c;
	for (int i = 0; i < 6; i++)
	{
		uint8_t* data = stbi_load(pathes[i], &width, &height, &c, STBI_rgb_alpha);
		if (data)
		{
			std::cout << "Cubemap texture load at path: " << pathes[i] << std::endl;
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << GL_TEXTURE_CUBE_MAP_POSITIVE_X + i << std::endl;
		}
		stbi_image_free(data);
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	return cubemapTexture;
}

#pragma endregion

#pragma region Init_Functions

void InitCubeMap()
{
	const char* pathes[6] = {
		"envmaps/pisa_posx.jpg",
		"envmaps/pisa_negx.jpg",
		"envmaps/pisa_posy.jpg",
		"envmaps/pisa_negy.jpg",
		"envmaps/pisa_posz.jpg",
		"envmaps/pisa_negz.jpg"
	};

	cubeMapID = LoadCubemap(pathes);
}

void InitSkybox()
{
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f, -1.0f,
		 1.0f,  1.0f,  1.0f,
		 1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f, -1.0f,
		 1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		 1.0f, -1.0f,  1.0f
	};

	glGenVertexArrays(1, &skyboxVAO);
	glBindVertexArray(skyboxVAO);

	glGenBuffers(1, &skyboxVBO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);

	program = g_skyboxShader.GetProgram();

	//Position
	int loc_position = glGetAttribLocation(program, "a_position");
	glVertexAttribPointer(loc_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(loc_position);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

	viewMatrixLocationTP = glGetUniformLocation(g_skyboxShader.GetProgram(), "u_viewMatrix");
	projectionMatrixLocationTP = glGetUniformLocation(g_skyboxShader.GetProgram(), "u_projectionMatrix");

	//D�sactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//Init VAO, VBO and IBO
void InitBufferPlane()
{
	GLuint PVBO;
	GLuint PIBO;
	program = g_basicShader.GetProgram();

	//Cr�ation VAO
	glGenVertexArrays(1, &PVAO);
	glBindVertexArray(PVAO);

	//Cr�ation VBO
	glGenBuffers(1, &PVBO);
	glBindBuffer(GL_ARRAY_BUFFER, PVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * plane.vertexCount, &plane.vertices[0], GL_STATIC_DRAW);

	//Cr�ation IBO
	glGenBuffers(1, &PIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, PIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * plane.indices.size() - 1, &plane.indices[0], GL_STATIC_DRAW);

	//Position
	int loc_position = glGetAttribLocation(program, "a_position");
	glVertexAttribPointer(loc_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(loc_position);

	//UV
	int texcoords_location = glGetAttribLocation(program, "a_texcoords");
	glVertexAttribPointer(texcoords_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texCoord.x));
	glEnableVertexAttribArray(texcoords_location);

	//Normals
	int normals_location = glGetAttribLocation(program, "a_normals");
	glVertexAttribPointer(normals_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal.x));
	glEnableVertexAttribArray(normals_location);

	//D�sactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InitBuffersSuzanne() 
{
	GLuint VBO;
	GLuint IBO;
	program = g_basicShader.GetProgram();

	//Cr�ation VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//Cr�ation VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertexCount, &mesh.vertices[0], GL_STATIC_DRAW);

	//Cr�ation IBO
	glGenBuffers(1, &IBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * mesh.indices.size() - 1, &mesh.indices[0], GL_STATIC_DRAW);

	//Position
	int loc_position = glGetAttribLocation(program, "a_position");
	glVertexAttribPointer(loc_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(loc_position);

	//UV
	int texcoords_location = glGetAttribLocation(program, "a_texcoords");
	glVertexAttribPointer(texcoords_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texCoord.x));
	glEnableVertexAttribArray(texcoords_location);

	//Normals
	int normals_location = glGetAttribLocation(program, "a_normals");
	glVertexAttribPointer(normals_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal.x));
	glEnableVertexAttribArray(normals_location);

	g_TextureObject = LoadTexture("suzanne.jpg");

	//D�sactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void InitBuffersTeapot()
{
	GLuint IBOTP;
	GLuint VBOTP;
	programTP = g_teapotShader.GetProgram();

	//Cr�ation VAO
	glGenVertexArrays(1, &VAOTP);
	glBindVertexArray(VAOTP);

	//Cr�ation VBO
	glGenBuffers(1, &VBOTP);
	glBindBuffer(GL_ARRAY_BUFFER, VBOTP);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * teapot.vertexCount, &teapot.vertices[0], GL_STATIC_DRAW);

	//Cr�ation IBO
	glGenBuffers(1, &IBOTP);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBOTP);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * teapot.indices.size() - 1, &teapot.indices[0], GL_STATIC_DRAW);

	//Position
	int loc_position = glGetAttribLocation(programTP, "a_position");
	glVertexAttribPointer(loc_position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), nullptr);
	glEnableVertexAttribArray(loc_position);

	//UV
	int texcoords_location = glGetAttribLocation(programTP, "a_texcoords");
	glVertexAttribPointer(texcoords_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, texCoord.x));
	glEnableVertexAttribArray(texcoords_location);

	//Normals
	int normals_location = glGetAttribLocation(programTP, "a_normals");
	glVertexAttribPointer(normals_location, 3, GL_FLOAT, false, sizeof(Vertex), (void*)offsetof(Vertex, normal.x));
	glEnableVertexAttribArray(normals_location);

	g_TextureObjectTeapot = LoadTexture("teapot.png");

	//D�sactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//Init FBO
void InitFBO()
{
	glEnable(GL_FRAMEBUFFER_SRGB);

	//Cr�ation
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glGenTextures(1, &texturesID[0]);
	glBindTexture(GL_TEXTURE_2D, texturesID[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texturesID[0], 0);

	//Load Depth
	GLuint textureDepth = 0;
	glGenTextures(1, &textureDepth);
	glBindTexture(GL_TEXTURE_2D, textureDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, windowWidth, windowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, textureDepth, 0);

	//Check FBO
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	//Reset
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Init2DRender()
{
	uint32_t program2D = g_2DShader.GetProgram();
	
	const float quadScreen[] = {
		-1.f, 1.f, 0.f, 0.f,
		-1.f, -1.f, 0.f, 1.f,
		1.f, 1.f, 1.f, 0.f,
		1.f, -1.f, 1.f, 1.f,
	};

	//Cr�ation VAO
	glGenVertexArrays(1, &VAO2D);
	glBindVertexArray(VAO2D);

	//Cr�ation VBO
	glGenBuffers(1, &VBO2D);
	glBindBuffer(GL_ARRAY_BUFFER, VBO2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadScreen), quadScreen, GL_STATIC_DRAW);

	glUseProgram(program2D);

	// Position
	int loc_position = glGetAttribLocation(program2D, "a_position");
	glVertexAttribPointer(loc_position, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 4, nullptr);
	glEnableVertexAttribArray(loc_position);

	//UV
	int texcoords_location = glGetAttribLocation(program2D, "a_texcoords");
	glVertexAttribPointer(texcoords_location, 2, GL_FLOAT, false, sizeof(float) * 4, (void*)(sizeof(float) * 2));
	glEnableVertexAttribArray(texcoords_location);

	//D�sactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

//Init Shadow buffer
void InitFBOLight()
{
	GLuint shadowmapTexture;
	GLuint shadowmapFBO;
	GLenum shadowmap_precision = GL_DEPTH_COMPONENT24; // 32, 24 ou 16
	GLenum shadowmap_type = GL_UNSIGNED_INT; //GL_UNSIGNED_INT ou _FLOAT

	glGenTextures(1, &shadowmapTexture);
	glBindTexture(GL_TEXTURE_2D, shadowmapTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, shadowmap_precision, shadowmap_resolution, shadowmap_resolution, 0, GL_DEPTH_COMPONENT, shadowmap_type, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);

	glGenFramebuffers(1, &shadowmapFBO);
	glBindFramebuffer(GL_FRAMEBUFFER, shadowmapFBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowmapTexture, 0);

	glDrawBuffer(GL_NONE); // tres important, autrement FBO non complet!
	glReadBuffer(GL_NONE); // optionnellement

	//Check FBO
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void Initialize()
{
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		std::cout << "erreur d'initialisation de GLEW!"
			<< endl;
	}

	cout << "Version : " << glGetString(GL_VERSION) << endl;
	cout << "Vendor : " << glGetString(GL_VENDOR) << endl;
	cout << "Renderer : " << glGetString(GL_RENDERER) << endl;

	//Link shaders and create program
	g_basicShader.LoadVertexShader("basic.vs.glsl");
	g_basicShader.LoadFragmentShader("basic.fs.glsl");
	g_basicShader.Create();

	g_teapotShader.LoadVertexShader("teapotShader.vs.glsl");
	g_teapotShader.LoadFragmentShader("teapotShader.fs.glsl");
	g_teapotShader.Create();

	g_skyboxShader.LoadVertexShader("skyboxShader.vs.glsl");
	g_skyboxShader.LoadFragmentShader("skyboxShader.fs.glsl");
	g_skyboxShader.Create();

	//Link shaders 2D
	g_2DShader.LoadVertexShader("postprocess.vs.glsl");
	g_2DShader.LoadFragmentShader("postprocess.fs.glsl");
	g_2DShader.Create();

	//Load OBJ
	loadModel("suzanne.obj", mesh);
	loadModel("teapot.obj", teapot);
	loadModel("plane.obj", plane);

	InitFBO();
	InitBuffersSuzanne();
	InitBuffersTeapot();
	InitBufferPlane();

	Init2DRender();
	InitCubeMap();
	InitSkybox();

	InitFBOLight();

	//Uniforms
	modelMatrixLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_modelMatrix");
	viewMatrixLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_viewMatrix");
	projectionMatrixLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_projectionMatrix");
	cameraPos_location = glGetUniformLocation(g_basicShader.GetProgram(), "u_camPos");

	modelMatrixLocationTP = glGetUniformLocation(g_teapotShader.GetProgram(), "u_modelMatrix");
	viewMatrixLocationTP = glGetUniformLocation(g_teapotShader.GetProgram(), "u_viewMatrix");
	projectionMatrixLocationTP = glGetUniformLocation(g_teapotShader.GetProgram(), "u_projectionMatrix");
	cameraPos_locationTP = glGetUniformLocation(g_teapotShader.GetProgram(), "u_camPos");

	//Uniform Light
	depthMVPMatrixLocation = glGetUniformLocation(g_teapotShader.GetProgram(), "u_modelMatrixLight");
	shadowMapLocation = glGetUniformLocation(g_teapotShader.GetProgram(), "u_ShadowMap");
}

#pragma endregion

#pragma region Render_Functions

void RenderSuzanne()
{
	glUseProgram(g_basicShader.GetProgram());
	SetUniform(g_basicShader);

	//Texture
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	int texture_location = glGetUniformLocation(program, "u_TextureSampler");
	glUniform1i(texture_location, 0);
	glBindTexture(GL_TEXTURE_2D, g_TextureObject);

	//Time
	currentTime = (float)glfwGetTime();

	modelMatrix = modelMatrix.Scale(1.f) * modelMatrix.Rotate(Vec3(0.0f, -currentTime, 0.f)) * modelMatrix.Translate(1.5f, 2.f, 0.f) * modelMatrix.Rotate(Vec3(0.0f, -currentTime, 0.f));

	//Matrix uniforms
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, modelMatrix.getMatrix());
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, projectionMatrix.getMatrix());
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, viewMatrix.getMatrix());
	glUniform3f(cameraPos_location, camPos.x, camPos.y, camPos.z);

	//Active VAO -> Render -> reset VAO
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, mesh.vertexCount, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);
}

void RenderTeapot()
{
	glUseProgram(g_teapotShader.GetProgram());
	SetUniform(g_teapotShader);

	//Texture
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	int TP_location = glGetUniformLocation(programTP, "u_TextureSampler");
	glUniform1i(TP_location, 0);
	glBindTexture(GL_TEXTURE_2D, g_TextureObjectTeapot);

	Matrix4 modelMatrixTeapot;
	modelMatrixTeapot = modelMatrixTeapot.Scale(1.f) * modelMatrixTeapot.Rotate(Vec3(0.f, 0.f, 0.f)) * modelMatrixTeapot.Translate(0.f, 0.f, 0.f);

	glUniformMatrix4fv(modelMatrixLocationTP, 1, GL_FALSE, modelMatrixTeapot.getMatrix());
	glUniformMatrix4fv(projectionMatrixLocationTP, 1, GL_FALSE, projectionMatrix.getMatrix());
	glUniformMatrix4fv(viewMatrixLocationTP, 1, GL_FALSE, viewMatrix.getMatrix());
	glUniform3f(cameraPos_locationTP, camPos.x, camPos.y, camPos.z);

	glBindVertexArray(VAOTP);
	glDrawElements(GL_TRIANGLES, teapot.vertexCount, GL_UNSIGNED_SHORT, nullptr);

	glBindVertexArray(0);
}

void RenderPlane()
{
	glUseProgram(g_teapotShader.GetProgram());
	SetUniform(g_teapotShader);

	//Texture
	glActiveTexture(GL_TEXTURE0);
	glEnable(GL_TEXTURE_2D);
	texture_location = glGetUniformLocation(program, "u_TextureSampler");
	glUniform1i(texture_location, 0);
	glBindTexture(GL_TEXTURE_2D, g_TextureObject);

	modelMatrix = modelMatrix.Scale(1.f) * modelMatrix.Rotate(Vec3(0.0f, 0.0f, 0.0f)) * modelMatrix.Translate(0.f, 0.f, 0.f);

	//Matrix uniforms
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, modelMatrix.getMatrix());
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, projectionMatrix.getMatrix());
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, viewMatrix.getMatrix());
	glUniform3f(cameraPos_location, camPos.x, camPos.y, camPos.z);

	//Active VAO -> Render -> reset VAO
	glBindVertexArray(PVAO);
	glDrawElements(GL_TRIANGLES, plane.vertexCount, GL_UNSIGNED_SHORT, nullptr);
	glBindVertexArray(0);
}

void RenderSkybox()
{
	glDepthFunc(GL_LEQUAL);
	glUseProgram(g_skyboxShader.GetProgram());
	SetUniform(g_skyboxShader);

	glActiveTexture(GL_TEXTURE1);
	int32_t skyTextureLocation = glGetUniformLocation(program, "u_SkyTexture");
	glUniformMatrix4fv(projectionMatrixLocationTP, 1, GL_FALSE, projectionMatrix.getMatrix());
	glUniformMatrix4fv(viewMatrixLocationTP, 1, GL_FALSE, viewMatrix.getMatrix());
	glUniform1i(skyTextureLocation, 1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeMapID);

	glBindVertexArray(skyboxVAO);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS);
}

void Render2D()
{
	int width, height;
	glfwGetWindowSize(window, &width, &height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
	glClearColor(0.5f, 0.5f, 0.5f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	glUseProgram(g_2DShader.GetProgram());
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texturesID[0]);
	glBindVertexArray(VAO2D);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void RenderLightBuffer()
{
	glBindFramebuffer(GL_FRAMEBUFFER, shadowmapFBO);
	glViewport(0, 0, shadowmap_resolution, shadowmap_resolution);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(programTP);

	projectionMatrixLight = projectionMatrixLight.Ortho(-10, 10, 10, -10, -1, 10);
	viewMatrixLight = viewMatrixLight.LookAt(lightPosition, Vec3(0, 0, 0), Vec3(0, 1, 0));
	depthMVP = projectionMatrixLight * viewMatrixLight;
	Matrix4 biasMatrix = biasMatrix.biasMatrix();
	depthMVP = biasMatrix * depthMVP;

	glUniformMatrix4fv(depthMVPMatrixLocation, 1, GL_FALSE, depthMVP.getMatrix());

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void RenderScene(GLFWwindow* window)
{

	int width, height;
	glfwGetWindowSize(window, &width, &height);

	// vers le FBO
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	// Defini le viewport en pleine fenetre
	glViewport(0, 0, width, height);
	glClearColor(0.5f, 0.5f, 0.5f, 1.f);

	//Culling and depth buffer
	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//----------------------------------------Matrix---------------------------------------------------
	//projectionMatrix = projectionMatrix.Ortho(-windowWidth/2, windowWidth/2, -windowHeight/2, windowHeight/2, -1, 1);
	projectionMatrix = projectionMatrix.Perspective(FOV, windowWidth / (float)windowHeight, 0.0001f, 100.f);
	viewMatrix = viewMatrix.LookAt(camPos, Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 1.0f, 0.0f));

	//-----------------------------------------------------Render---------------------------------------------------------------------
	RenderSuzanne();
	RenderTeapot();
	RenderPlane();
	RenderSkybox();


	//---------------------------------------------------Default binding---------------------------------------------------------------------
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glDisable(GL_TEXTURE_2D);
}

#pragma endregion

#pragma region Inputs_Functions

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastCamX = windowWidth / 2;
		lastCamY = windowHeight / 2;
		firstMouse = false;
	}

	float xoffset = (xpos - lastCamX) / 100.f;
	float yoffset = (lastCamY - ypos) / 100.f;
	lastCamX = xpos;
	lastCamY = ypos;


	Matrix4 mat;
	Vec3 oldCam = camPos;
	camPos = mat.Rotate(Vec3(yoffset, xoffset, 0.0f)) * mat.Translate(camPos.x, camPos.y, camPos.z) * Vec3(0.f, 0.f, 0.f);
}

void keyboard_button_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	uint32_t program2D = g_2DShader.GetProgram();
	int boolLocation = glGetUniformLocation(program2D, "activePP");

	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && activePP == false)
	{
		b = 1;
		activePP = true;
	}
	else if (key == GLFW_KEY_ENTER && action == GLFW_PRESS && activePP == true)
	{
		b = 0;
		activePP = false;
	}

	glUniform1i(boolLocation, b);
}

//Zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	float zoomSpeed = 2.0f;
	FOV += yoffset * zoomSpeed;

	if (FOV <= 1.0f)
		FOV = 1.0f;

}

void InitInputs() {
	
	//Disable cursor
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	//Cursor Position callback
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetKeyCallback(window, keyboard_button_callback);

	//Scroll callback
	glfwSetScrollCallback(window, scroll_callback);
}
#pragma endregion

int main(void)
{

	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(windowWidth, windowHeight, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// toutes nos initialisations vont ici
	Initialize();
	InitInputs();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		//RenderBuffer light for shadow
		RenderLightBuffer();

		//3D
		RenderScene(window);

		//2D
		Render2D();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	Shutdown();

	glfwTerminate();
	return 0;
}