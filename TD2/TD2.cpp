//
//
//


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

#include "Vertex.h"
#include "Matrix4.h"
#include "../common/GLShader.h"
#include "Mesh.h"

// fonctions de chargement d'images (bitmap)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tinyobjloader/tiny_obj_loader.h"

using namespace std;

// variables globales (dans le cadre de cet exemple, evitez les globales si possible)
GLShader g_basicShader;
GLuint g_TextureObject;

//Buffers
GLuint VAO;
GLuint VBO;
GLuint IBO;
GLuint FBO;

//Buffers 2D
GLuint VAO2D;
GLuint VBO2D;
GLShader g_2DShader;

//Load colorBuffer
GLuint colorBufferTexture;

Mesh mesh;
float currentTime;

GLFWwindow* window;
int windowWidth = 800;
int windowHeight = 600;

GLuint LoadTexture(const char* path)
{
	// 1. chargement de la bitmap
	int w, h, c;
	uint8_t* data = stbi_load(path, &w, &h, &c, STBI_rgb_alpha);
	// 2. creation du texture object OpenGL
	GLuint TextureID = 0;
	glGenTextures(1, &TextureID);
	// 3. chargement et parametrage
	// pour pouvoir travailler sur/avec la texture
	// on doit d'abord la "bind" / attacher sur un identifiant
	// en l'occurence GL_TEXTURE_2D
	glBindTexture(GL_TEXTURE_2D, TextureID);
	// les 6 premiers params definissent le stockage de la texture en VRAM (memoire video)
	// les 3 derniers specifient l'image source
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
		GL_LINEAR);
	// on remet la valeur par defaut (pas obligatoire mais preferable ici)
	glBindTexture(GL_TEXTURE_2D, 0);
	// 4. liberation de la memoire
	stbi_image_free(data);
	return TextureID;
}

void DestroyTexture(GLuint* textureID)
{
	glDeleteTextures(1, textureID);
	textureID = 0;
}

void loadModel(const string path)
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
			mesh.indices.push_back(mesh.indices.size());
			mesh.vertices.push_back(vertex);
		}

		
		
	}
	mesh.vertexCount = mesh.vertices.size() - 1;

	for (size_t f = 0; f < materials.size(); f++)
	{
		mesh.mat.Ka = { materials[f].ambient[0],
						materials[f].ambient[1],
						materials[f].ambient[2] };

		mesh.mat.Kd = { materials[f].diffuse[0],
						materials[f].diffuse[1],
						materials[f].diffuse[2] };

		mesh.mat.Ks = { materials[f].specular[0],
						materials[f].specular[1],
						materials[f].specular[2] };

		mesh.mat.shininess = materials[f].shininess;
	}
}

//Init VAO, VBO and IBO
void InitBuffers() 
{
	uint32_t program = g_basicShader.GetProgram();

	//Création VAO
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	//Création VBO
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * mesh.vertexCount, &mesh.vertices[0], GL_STATIC_DRAW);

	//Création IBO
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

	//Texture
	int texture_location = glGetUniformLocation(program, "u_TextureSampler");
	glUniform1i(texture_location, g_TextureObject);

	//Désactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SetUniform()
{
	uint32_t program = g_basicShader.GetProgram();

	//------------------------------------------------- Position de la lumiere-----------------------------------------------
	int lightPos_location = glGetUniformLocation(program, "u_light.pos");
	glUniform3f(lightPos_location, 0.5f, 0.0f, -2.0f);
	int lightColor_location = glGetUniformLocation(program, "u_light.color");
	glUniform3f(lightColor_location, 1.0f, 1.0f, 1.0f);

	//-------------------------------------------------------Paramètre lumière------------------------------------------------
	int lightPos_Ia = glGetUniformLocation(program, "u_light.Ia");
	glUniform1f(lightPos_location, 1.0f);
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
	//Spéculaire
	int matKs_location = glGetUniformLocation(program, "u_mat.Ks");
	glUniform3f(matKs_location, mesh.mat.Ks.x, mesh.mat.Ks.y, mesh.mat.Ks.z);
	//Reflection
	int shininessPos_location = glGetUniformLocation(program, "u_mat.shininess");
	glUniform1i(shininessPos_location, mesh.mat.shininess);

}

//Init FBO
void InitFBO()
{

	glEnable(GL_FRAMEBUFFER_SRGB);

	//Création
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	glGenTextures(1, &colorBufferTexture);
	glBindTexture(GL_TEXTURE_2D, colorBufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorBufferTexture, 0);

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

	//Création VAO
	glGenVertexArrays(1, &VAO2D);
	glBindVertexArray(VAO2D);

	//Création VBO
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

	//Texture
	//int texture_location = glGetUniformLocation(program2D, "u_TextureSampler");
	//glUniform1i(texture_location, 0);

	//Désactivation des buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
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
	glBindTexture(GL_TEXTURE_2D, colorBufferTexture);
	glBindVertexArray(VAO2D);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

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

	//Link shaders 2D
	g_2DShader.LoadVertexShader("postprocess.vs.glsl");
	g_2DShader.LoadFragmentShader("postprocess.fs.glsl");
	g_2DShader.Create();

	//Load OBJ
	loadModel("suzanne.obj");
	loadModel("teapot.obj");
	
	g_TextureObject = LoadTexture("suzanne.jpg");
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_TextureObject);

	//glUseProgram(g_basicShader.GetProgram());

	InitFBO();
	InitBuffers();
	Init2DRender();
}

void Shutdown()
{
	DestroyTexture(&g_TextureObject);
	glDeleteFramebuffers(1, &FBO);
	g_basicShader.Destroy();
	g_2DShader.Destroy();
}

int timeLocation;
int modelMatrixLocation;
int viewMatrixLocation;
int projectionMatrixLocation;
int cameraPos_location;

void Display(GLFWwindow* window)
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

	glUseProgram(g_basicShader.GetProgram());

	//Time
	currentTime = (float)glfwGetTime();
	glUniform1f(timeLocation, currentTime);

	Matrix4 modelMatrix;
	Matrix4 projectionMatrix;
	Matrix4 viewMatrix;
	Vec3 camPos(0.f, 0.f, -5.f);

	//-----------------------------------------------------Matrix update---------------------------------------------------------------------
	modelMatrix = modelMatrix.Scale(1.f) * modelMatrix.Rotate(Vec3(0.0f, currentTime, currentTime)) * modelMatrix.Translate(0.f, -0.7f, 0.f);
	//projectionMatrix = projectionMatrix.Ortho(-windowWidth/2, windowWidth/2, -windowHeight/2, windowHeight/2, -1, 1);
	projectionMatrix = projectionMatrix.Perspective(60, windowWidth / (float)windowHeight, 0.0001f, 100.f);
	viewMatrix = viewMatrix.LookAt(camPos, Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, -1.0f, 0.0f));

	//Matrix uniforms
	glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, modelMatrix.getMatrix());
	glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, projectionMatrix.getMatrix());
	glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, viewMatrix.getMatrix());
	glUniform3f(cameraPos_location, camPos.x, camPos.y, camPos.z);

	//Active VAO -> Render -> reset VAO
	/*
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawElements(GL_TRIANGLES, mesh.vertexCount, GL_UNSIGNED_SHORT, nullptr);
	*/
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, mesh.vertexCount, GL_UNSIGNED_SHORT, nullptr);
	
	glBindVertexArray(0);
	
	/*
	glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);*/
}

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

	//Uniforms
	timeLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_time");
	modelMatrixLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_modelMatrix");
	viewMatrixLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_viewMatrix");
	projectionMatrixLocation = glGetUniformLocation(g_basicShader.GetProgram(), "u_projectionMatrix");
	cameraPos_location = glGetUniformLocation(g_basicShader.GetProgram(), "u_camPos");

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{

		/* Render here */
		//3D
		Display(window);
		// Uniform for 3D render
		SetUniform();
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