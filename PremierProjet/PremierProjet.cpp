//
//
//

// GLEW_STATIC force le linkage statique
// c-a-d que le code de glew est directement injecte dans l'executable
#define GLEW_STATIC
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// les repertoires d'includes sont:
// ../libs/glfw-3.3/include			fenetrage
// ../libs/glew-2.1.0/include		extensions OpenGL
// ../libs/stb						gestion des images (entre autre)

// les repertoires des libs sont (en 64-bit):
// ../libs/glfw-3.3/lib-vc2015
// ../libs/glew-2.1.0/lib/Release/x64

// Pensez a copier les dll dans le repertoire x64/Debug, cad:
// glfw-3.3/lib-vc2015/glfw3.dll
// glew-2.1.0/bin/Release/x64/glew32.dll		si pas GLEW_STATIC

// _WIN32 indique un programme Windows
// _MSC_VER indique la version du compilateur VC++
#if defined(_WIN32) && defined(_MSC_VER)
#pragma comment(lib, "glfw3dll.lib")
#pragma comment(lib, "glew32s.lib")			// glew32.lib si pas GLEW_STATIC
#pragma comment(lib, "opengl32.lib")
#elif defined(__APPLE__)
#elif defined(__linux__)
#endif

#include <iostream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "DragonData.h"

#define TINYOBJLOADER_IMPLEMENTATION // define this in only *one* .cc
// Optional. define TINYOBJLOADER_USE_MAPBOX_EARCUT gives robust trinagulation. Requires C++11
//#define TINYOBJLOADER_USE_MAPBOX_EARCUT
#include "tiny_obj_loader.h"




// format des vertex du dragon XY-NXNYNZ-UV
struct Vertex {
	//float x, y, z; 
	//float nx, ny, nz; 
	//float u, v;
	float position[3];
	float normal[3];
	float uv[2];
};

// ou bien std::uint32_t 
GLuint dragonVAO;	// la structure d'attributs stockee en VRAM
GLuint skyboxVAO;	// la structure d'attributs stockee en VRAM
GLuint dragonVBO;	// les vertices de l'objet stockees en VRAM
GLuint dragonIBO;	// les indices de l'objet stockees en VRAM
GLuint texID;

// vous pouvez ajouter ce repertoire directement dans les proprietes du projet
#include "../common/GLShader.h"

GLShader lightShader;
GLShader copyShader;
GLShader skyboxShader;


unsigned int cubemapTexture;

// identifiant du Framebuffer Object
GLuint FBO;
GLuint ColorBufferFBO; // stocke les couleurs du rendu hors ecran
GLuint DepthBufferFBO; // stocke les Z du rendu hors ecran

//Position souris initiale
double xO, yO;
float rotaX, rotaY = 0;
bool isClicked = false;


unsigned int loadCubemap(std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data
			);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

void Initialize()
{
	GLenum error = glewInit();
	if (error != GLEW_OK) {
		std::cout << "erreur d'initialisation de GLEW!"
			<< std::endl;
	}

	std::cout << "Version : " << glGetString(GL_VERSION) << std::endl;
	std::cout << "Vendor : " << glGetString(GL_VENDOR) << std::endl;
	std::cout << "Renderer : " << glGetString(GL_RENDERER) << std::endl;

	lightShader.LoadVertexShader("basicLight.vs");
	lightShader.LoadFragmentShader("basicLight.fs");
	lightShader.Create();

	copyShader.LoadVertexShader("postprocess.vs");
	copyShader.LoadFragmentShader("postprocess.fs");
	copyShader.Create();

	skyboxShader.LoadVertexShader("skybox.vs");
	skyboxShader.LoadFragmentShader("skybox.fs");
	skyboxShader.Create();

	// Skybox

	std::vector<std::string> faces;
	{
			"skybox/right.jpg",
			"skybox/left.jpg",
			"skybox/top.jpg",
			"skybox/bottom.jpg",
			"skybox/front.jpg",
			"skybox/back.jpg";
	};
	cubemapTexture = loadCubemap(faces);

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

	const int SKYBOX_POS =
		glGetAttribLocation(skyboxShader.GetProgram(), "aPos");
	glEnableVertexAttribArray(SKYBOX_POS);
	glVertexAttribPointer(SKYBOX_POS, 3, GL_FLOAT, false, 0, &skyboxVertices[0]);


	// ----Load Obj ----

	std::string inputfile = "indoor plant_02.obj";
	tinyobj::ObjReaderConfig reader_config;
	reader_config.mtl_search_path = "./"; // Path to material files

	tinyobj::ObjReader reader;

	if (!reader.ParseFromFile(inputfile, reader_config)) {
		if (!reader.Error().empty()) {
			std::cerr << "TinyObjReader: " << reader.Error();
		}
		exit(1);
	}

	if (!reader.Warning().empty()) {
		std::cout << "TinyObjReader: " << reader.Warning();
	}

	auto& attrib = reader.GetAttrib();
	auto& shapes = reader.GetShapes();
	auto& materials = reader.GetMaterials();

	std::vector<float> vertices;
	std::vector<float> normals;
	std::vector<float> uvs;
	std::vector<float> indices;

	std::cout << shapes.size() << std::endl;


	// Loop over shapes
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
			

			// Loop over vertices in the face.
			for (size_t v = 0; v < fv; v++) {
				// access to vertex
				tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
				tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
				tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
				tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

				vertices.push_back(vx);
				vertices.push_back(vy);
				vertices.push_back(vz);

				indices.push_back(index_offset + v);

				// Check if `normal_index` is zero or positive. negative = no normal data
				if (idx.normal_index >= 0) {
					tinyobj::real_t nx = attrib.normals[3 * size_t(idx.normal_index) + 0];
					tinyobj::real_t ny = attrib.normals[3 * size_t(idx.normal_index) + 1];
					tinyobj::real_t nz = attrib.normals[3 * size_t(idx.normal_index) + 2];

					vertices.push_back(nx);
					vertices.push_back(ny);
					vertices.push_back(nz);
				}

				// Check if `texcoord_index` is zero or positive. negative = no texcoord data
				if (idx.texcoord_index >= 0) {
					tinyobj::real_t tx = attrib.texcoords[2 * size_t(idx.texcoord_index) + 0];
					tinyobj::real_t ty = attrib.texcoords[2 * size_t(idx.texcoord_index) + 1];

					vertices.push_back(tx);
					vertices.push_back(ty);
				}

				// Optional: vertex colors
				// tinyobj::real_t red   = attrib.colors[3*size_t(idx.vertex_index)+0];
				// tinyobj::real_t green = attrib.colors[3*size_t(idx.vertex_index)+1];
				// tinyobj::real_t blue  = attrib.colors[3*size_t(idx.vertex_index)+2];
			}
			index_offset += fv;

			// per-face material
			shapes[s].mesh.material_ids[f];
		}
	}

	float* verticesArr = &vertices[0];
	//float* normalsArr = &normals[0];
	//float* uvsArr = &uvs[0];
	float* indicesArr = &indices[0];

	/*for (int i = 0; i < vertices.size(); i++)
	{*/
		std::cout << verticesArr[ 0] << " " << verticesArr[ 1] << " " << verticesArr[ 2] << std::endl;
		std::cout << indicesArr[ 0] << " " << indicesArr[ 1] << " " << indicesArr[ 2] << std::endl;
	/*}*/
	

	// --- FBO ----

	glGenFramebuffers(1, &FBO);
	// attache une texture comme color buffer
	glGenTextures(1, &ColorBufferFBO);
	glBindTexture(GL_TEXTURE_2D, ColorBufferFBO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1280, 720, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// attache une texture comme depth buffer
	glGenTextures(1, &DepthBufferFBO);
	glBindTexture(GL_TEXTURE_2D, DepthBufferFBO);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, 1280, 720, 0,
		GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

	// bind FBO + attache la texture que l'on vient de creer
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
		GL_TEXTURE_2D, ColorBufferFBO, 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
		GL_TEXTURE_2D, DepthBufferFBO, 0);
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// ---

	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);
	int texWidth, texHeight, c;
	uint8_t* pixels =
		stbi_load("batman_logo.png", &texWidth, &texHeight, &c, STBI_rgb_alpha);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, texWidth, texHeight, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, pixels);
	glGenerateMipmap(GL_TEXTURE_2D);
	stbi_image_free(pixels);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glGenVertexArrays(1, &dragonVAO);
	glBindVertexArray(dragonVAO);
	// ATTENTION : avec glBindVertexArray TOUS les appels suivants sont 
	// enregistres dans le VAO 
	// ca inclus glBindBuffer(GL_ARRAY_BUFFER/GL_ELEMENT_ARRAY_BUFFER..)
	// ainsi que tous les appels glVertexAttribPointer(), glEnable/disableVertexAttribArray
	
	 //---------------------------------VBO TLO----------------------------------------------------------------
	/*glGenBuffers(1, &dragonVBO);
	glBindBuffer(GL_ARRAY_BUFFER, dragonVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(verticesArr), verticesArr, GL_STATIC_DRAW);

	glGenBuffers(1, &dragonIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dragonIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indicesArr), indicesArr, GL_STATIC_DRAW);

	constexpr int STRIDE = sizeof(float) * 8;
	// nos positions sont XYZ (3 floats)
	const int POSITION =
		glGetAttribLocation(lightShader.GetProgram(), "a_position");
	glEnableVertexAttribArray(POSITION);
	glVertexAttribPointer(POSITION, 3, GL_FLOAT, false, STRIDE, &verticesArr[0]);
	// nos normales sont en 3D aussi (3 floats)
	const int NORMAL =
		glGetAttribLocation(lightShader.GetProgram(), "a_normal");
	glEnableVertexAttribArray(NORMAL);
	glVertexAttribPointer(NORMAL, 3, GL_FLOAT, false, STRIDE, &verticesArr[3]);

	// NECESSAIRE POUR LES TEXTURES ---
	const int UV =
		glGetAttribLocation(lightShader.GetProgram(), "a_texcoords");
	glEnableVertexAttribArray(UV);
	glVertexAttribPointer(UV, 2, GL_FLOAT, false, STRIDE, &verticesArr[6]);*/
	//----------------------------------------------------------------------------------------------------------------

	//---------------------------------VBO Dragon----------------------------------------------------------------
	glGenBuffers(1, &dragonVBO);
	glBindBuffer(GL_ARRAY_BUFFER, dragonVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(DragonVertices), DragonVertices, GL_STATIC_DRAW);

	glGenBuffers(1, &dragonIBO);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dragonIBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(DragonIndices), DragonIndices, GL_STATIC_DRAW);

	constexpr int STRIDE = sizeof(Vertex);
	// nos positions sont XYZ (3 floats)
	const int POSITION =
		glGetAttribLocation(lightShader.GetProgram(), "a_position");
	glEnableVertexAttribArray(POSITION);
	glVertexAttribPointer(POSITION, 3, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, position));
	// nos normales sont en 3D aussi (3 floats)
	const int NORMAL =
		glGetAttribLocation(lightShader.GetProgram(), "a_normal");
	glEnableVertexAttribArray(NORMAL);
	glVertexAttribPointer(NORMAL, 3, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, normal));

	// NECESSAIRE POUR LES TEXTURES ---
	const int UV =
		glGetAttribLocation(lightShader.GetProgram(), "a_texcoords");
	glEnableVertexAttribArray(UV);
	glVertexAttribPointer(UV, 2, GL_FLOAT, false, STRIDE, (void*)offsetof(Vertex, uv));
	//-------------------------------------------------------------------------------------------------------

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void Shutdown()
{
	glDeleteVertexArrays(1, &dragonVAO);
	glDeleteBuffers(1, &dragonIBO);
	glDeleteBuffers(1, &dragonVBO);

	glDeleteTextures(1, &texID);

	glDeleteTextures(1, &ColorBufferFBO);
	glDeleteTextures(1, &DepthBufferFBO);
	glDeleteFramebuffers(1, &FBO);


	lightShader.Destroy();

	copyShader.Destroy();
}

void Display(GLFWwindow* window)
{
	int width, height;

	// rendu hors ecran
	int offscreenWidth = 1280, offscreenHeight = 720;

	glBindFramebuffer(GL_FRAMEBUFFER, FBO);
	glViewport(0, 0, offscreenWidth, offscreenHeight);
	
	glDepthMask(GL_FALSE);
	glUseProgram(skyboxShader.GetProgram());

	const float rot[] = {
	1.f, 0.f, 0.f, 0.f, // 1ere colonne
	0.f, 1.f, 0.f, 0.f,
	0.f, 0.f, 1.f, 0.f,
	0.f, 0.f, 0.f, 1.f // 4eme colonne
	};
	const GLint matRotLocationYSky = glGetUniformLocation(
		skyboxShader.GetProgram(),
		"view"
	);
	glUniformMatrix4fv(matRotLocationYSky, 1, false, rot);

	//
	// MATRICE DE PROJECTION
	//
	const float aspectRatio = float(offscreenWidth) / float(offscreenHeight);
	constexpr float nearZ = 0.01f;
	constexpr float farZ = 1000.f;
	constexpr float fov = 45.f;
	constexpr float fov_rad = fov * 3.141592654f / 180.f;
	const float f = 1.f / tanf(fov_rad / 2.f);
	const float projectionPerspective[] = {
		f / aspectRatio, 0.f, 0.f, 0.f, // 1ere colonne
		0.f, f, 0.f, 0.f,
		0.f, 0.f, -(farZ + nearZ) / (farZ - nearZ), -1.f,
		0.f, 0.f, -(2.f * farZ * nearZ) / (farZ - nearZ), 0.f // 4eme colonne
	};

	const GLint matProjectionLocationSky = glGetUniformLocation(
		skyboxShader.GetProgram(),
		"projection"
	);
	glUniformMatrix4fv(matProjectionLocationSky, 1, false, projectionPerspective);

	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);

	

	// attention le depth mask doit etre active avant d'ecrire (ou clear) le depth buffer
	glDepthMask(GL_TRUE);

	glClearColor(1.f, 1.f, 0.f, 1.f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// active le tri des faces
	glEnable(GL_DEPTH_TEST);
	// on peut egalement modifier la fonction de test
	//glDepthFunc(GL_LEQUAL); // par ex: en inferieur ou egal
	// active la suppression des faces arrieres
	glEnable(GL_CULL_FACE);

	glUseProgram(lightShader.GetProgram());
	

	// ----
	glActiveTexture(GL_TEXTURE0); // on la bind sur la texture unit #0
	glBindTexture(GL_TEXTURE_2D, texID);

	const int sampler =
		glGetUniformLocation(lightShader.GetProgram(), "u_sampler");
	glUniform1i(sampler, 0); // Le '0' correspond au '0' dans glActiveTexture()
	// ----

	// affecte le temps ecoules depuis le debut du programme à "u_time"
	const int timeLocation =
		glGetUniformLocation(lightShader.GetProgram(), "u_time");
	float time = static_cast<float>(glfwGetTime());
	glUniform1f(timeLocation, time);

	//
	// MATRICE DE SCALE
	//
	const float scale[] = {
		10.f, 0.f, 0.f, 0.f, // 1ere colonne
		0.f, 10.f, 0.f, 0.f,
		0.f, 0.f, 10.f, 0.f,
		0.f, 0.f, 0.f, 1.f // 4eme colonne
	};
	const GLint matScaleLocation = glGetUniformLocation(
		lightShader.GetProgram(),
		"u_scale"
	);
	glUniformMatrix4fv(matScaleLocation, 1, false, scale);

	//
	// MATRICE DE ROTATION
	//
	
	const float rotY[] = {
		cosf(rotaX), 0.f,	-sinf(rotaX), 0.f, // 1ere colonne
		0.f,		1.f,	0.f,		0.f,
		sinf(rotaX),	 0.f, cosf(rotaX),	0.f,
		0.f,		0.f,		0.f,	1.f // 4eme colonne
	};
	const GLint matRotLocationY = glGetUniformLocation(
		lightShader.GetProgram(),
		"u_rotation_y"
	);
	glUniformMatrix4fv(matRotLocationY, 1, false, rotY);

	const float rotX[] = {
		1.f,		 0.f,		  0.f,  0.f, // 1ere colonne
		0.f, cosf(rotaY),-sinf(rotaY),	0.f,
		0.f, sinf(rotaY), cosf(rotaY),	0.f,
		0.f,	     0.f,  	   	  0.f,	1.f // 4eme colonne
	};
	const GLint matRotLocationX = glGetUniformLocation(
		lightShader.GetProgram(),
		"u_rotation_x"
	);
	glUniformMatrix4fv(matRotLocationX, 1, false, rotX);

	
	/*const float rotZ[] = {
		cosf(rotaY) * sinf(rotaX),	    -sinf(rotaY) * sinf(rotaX),	     0.f,   0.f, // 1ere colonne
		sinf(rotaY) * sinf(rotaX),		 cosf(rotaY) * sinf(rotaX),		 0.f,	0.f,
							  0.f,							   0.f,		 1.f,	0.f,
							  0.f,  						   0.f,		 0.f,	1.f // 4eme colonne
	};
	const GLint matRotLocationZ = glGetUniformLocation(
		lightShader.GetProgram(),
		"u_rotation_z"
	);
	glUniformMatrix4fv(matRotLocationZ, 1, false, rotZ);*/

	//
	// MATRICE DE TRANSLATION
	//
	const float translation[] = {
		1.f, 0.f, 0.f, 0.f, // 1ere colonne
		0.f, 1.f, 0.f, 0.f,
		0.f, 0.f, 1.f, 0.f,
		0.f, -50.f, -200.f, 1.f // 4eme colonne
	};
	const GLint matTranslationLocation = glGetUniformLocation(
		lightShader.GetProgram(),
		"u_translation"
	);
	glUniformMatrix4fv(matTranslationLocation, 1, false, translation);

	
	const GLint matProjectionLocation = glGetUniformLocation(
		lightShader.GetProgram(),
		"u_projection"
	);
	glUniformMatrix4fv(matProjectionLocation, 1, false, projectionPerspective);

	glBindVertexArray(dragonVAO);
	// GL_UNSIGNED_SHORT car le tableau DragonIndices est compose de unsigned short (uint16_t)
	// le dernier 0 indique que les indices proviennent d'un ELEMENT_ARRAY_BUFFER
	glDrawElements(GL_TRIANGLES, _countof(DragonIndices), GL_UNSIGNED_SHORT, 0);

	// Etape finale
	// retour vers le back buffer
	glfwGetWindowSize(window, &width, &height);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	// Defini le viewport en pleine fenetre
	glViewport(0, 0, width, height);

	// ex1.1 
	//glBindFramebuffer(GL_READ_FRAMEBUFFER, FBO);
	////glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0); // redondant ici
	//glBlitFramebuffer(0, 0, 1280, 720,	// min;max de la source 
	//	0, 0, width, height, // min;max destination
	//	GL_COLOR_BUFFER_BIT, GL_LINEAR);

	// TODO: commenter les lignes precedentes + ex1.2 et 2.1

	glDepthMask(GL_FALSE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	GLuint postprocess_program = copyShader.GetProgram();
	glUseProgram(postprocess_program);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, ColorBufferFBO);
	GLint copyLoc = glGetUniformLocation(postprocess_program, "u_Texture");
	glUniform1i(copyLoc, 0);

	glBindVertexArray(0); // desactive le(s) VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	const float quad[] = { -1.f, +1.f, -1.f, -1.f, +1.f, +1.f, +1.f, -1.f };
	// les texcoords sont calculees sur la base de la position (xy * 0.5 + 0.5)
	glVertexAttribPointer(0, 2, GL_FLOAT, false, sizeof(float) * 2, quad);
	glEnableVertexAttribArray(0);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4); // {0,1,2}{1,2,3}
}

static void error_callback(int error, const char* description)
{
	std::cout << "Error GFLW " << error << " : " << description << std::endl;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		isClicked = true;
		glfwGetCursorPos(window, &xO, &yO);
		std::cout << xO << ", " << yO << std::endl;

		// [...]
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
	{
		isClicked = false;

	}
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (isClicked)
	{
		double dirX = ( xpos - xO ) * 0.001;
		double dirY = ( ypos - yO ) * 0.001;

		rotaX += dirX;
		rotaY += dirY;

		std::cout << sinf(rotaX) << ", " << cosf(rotaX) << std::endl;
	}
}

// voir https://www.glfw.org/documentation.html
// et https://www.glfw.org/docs/latest/quick.html
int main(void)
{
	GLFWwindow* window;

	glfwSetErrorCallback(error_callback);

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Corrige Projection OpenGL", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	// defini la fonction de rappel utilisee lors de l'appui d'une touche
	glfwSetKeyCallback(window, key_callback);

	// Clique souris GLFW Callback
	glfwSetMouseButtonCallback(window, mouse_button_callback);

	//Position souris
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// toutes nos initialisations vont ici
	Initialize();

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Render here */
		Display(window);

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	// ne pas oublier de liberer la memoire etc...
	Shutdown();

	glfwTerminate();
	return 0;
}