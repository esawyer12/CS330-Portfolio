///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager *pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(bFound);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/


/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring the various material
 *  settings for all of the objects in the 3D scene.  Each
 *  material controls how the Phong lighting model reflects
 *  light off of the surface it is applied to.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// wood material for the desk - warm diffuse color with a
	// low, soft specular highlight so the surface looks matte
	OBJECT_MATERIAL woodMaterial;
	woodMaterial.ambientColor = glm::vec3(0.2f, 0.15f, 0.1f);
	woodMaterial.ambientStrength = 0.3f;
	woodMaterial.diffuseColor = glm::vec3(0.55f, 0.4f, 0.25f);
	woodMaterial.specularColor = glm::vec3(0.2f, 0.15f, 0.1f);
	woodMaterial.shininess = 8.0f;
	woodMaterial.tag = "wood";
	m_objectMaterials.push_back(woodMaterial);

	// metal material for the drawer cabinets - cool diffuse color
	// with a bright, tight specular highlight for a reflective look
	OBJECT_MATERIAL metalMaterial;
	metalMaterial.ambientColor = glm::vec3(0.2f, 0.2f, 0.25f);
	metalMaterial.ambientStrength = 0.4f;
	metalMaterial.diffuseColor = glm::vec3(0.4f, 0.4f, 0.45f);
	metalMaterial.specularColor = glm::vec3(0.8f, 0.8f, 0.9f);
	metalMaterial.shininess = 48.0f;
	metalMaterial.tag = "metal";
	m_objectMaterials.push_back(metalMaterial);

	// darker wood material for the drawer fronts and desk apron so the
	// recessed panels read as a slightly different tone than the desktop
	OBJECT_MATERIAL darkWoodMaterial;
	darkWoodMaterial.ambientColor = glm::vec3(0.15f, 0.1f, 0.06f);
	darkWoodMaterial.ambientStrength = 0.25f;
	darkWoodMaterial.diffuseColor = glm::vec3(0.42f, 0.3f, 0.18f);
	darkWoodMaterial.specularColor = glm::vec3(0.18f, 0.13f, 0.08f);
	darkWoodMaterial.shininess = 12.0f;
	darkWoodMaterial.tag = "darkWood";
	m_objectMaterials.push_back(darkWoodMaterial);

	// dark polished metal material for the drawer handles - low diffuse
	// with a tight, bright highlight so the small bars catch the light
	OBJECT_MATERIAL handleMaterial;
	handleMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.06f);
	handleMaterial.ambientStrength = 0.3f;
	handleMaterial.diffuseColor = glm::vec3(0.15f, 0.15f, 0.18f);
	handleMaterial.specularColor = glm::vec3(0.95f, 0.95f, 1.0f);
	handleMaterial.shininess = 128.0f;
	handleMaterial.tag = "handle";
	m_objectMaterials.push_back(handleMaterial);

	// glass material for the monitor screens - dark diffuse color
	// with a very sharp, bright highlight to mimic a glossy panel
	OBJECT_MATERIAL glassMaterial;
	glassMaterial.ambientColor = glm::vec3(0.05f, 0.05f, 0.05f);
	glassMaterial.ambientStrength = 0.2f;
	glassMaterial.diffuseColor = glm::vec3(0.1f, 0.1f, 0.1f);
	glassMaterial.specularColor = glm::vec3(0.9f, 0.9f, 0.9f);
	glassMaterial.shininess = 96.0f;
	glassMaterial.tag = "glass";
	m_objectMaterials.push_back(glassMaterial);

	// painted material for the walls and floor - mostly diffuse
	// with a very weak specular so the surfaces stay flat looking
	OBJECT_MATERIAL paintedMaterial;
	paintedMaterial.ambientColor = glm::vec3(0.15f, 0.15f, 0.15f);
	paintedMaterial.ambientStrength = 0.3f;
	paintedMaterial.diffuseColor = glm::vec3(0.5f, 0.5f, 0.5f);
	paintedMaterial.specularColor = glm::vec3(0.1f, 0.1f, 0.1f);
	paintedMaterial.shininess = 4.0f;
	paintedMaterial.tag = "painted";
	m_objectMaterials.push_back(paintedMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is used for adding and configuring the light
 *  sources for the 3D scene.  A bright key point light lights
 *  the front of the scene while a cooler fill point light makes
 *  sure nothing is left in complete shadow.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// turn on the Phong lighting calculations in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// key light - warm overhead light 
	// above the desk to act as the main illumination of the scene
	m_pShaderManager->setVec3Value("lightSources[0].position", glm::vec3(0.0f, 14.0f, 10.0f));
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", glm::vec3(0.08f, 0.08f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", glm::vec3(0.55f, 0.5f, 0.42f));
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", glm::vec3(0.6f, 0.6f, 0.6f));
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 32.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.3f);

	// fill light - cooler light coming from the left side 
	m_pShaderManager->setVec3Value("lightSources[1].position", glm::vec3(-12.0f, 8.0f, 6.0f));
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", glm::vec3(0.06f, 0.06f, 0.08f));
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", glm::vec3(0.25f, 0.28f, 0.38f));
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", glm::vec3(0.3f, 0.3f, 0.4f));
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 16.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.2f);
}

/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// load the texture image files from disk and convert them into
	// OpenGL textures that can be applied to the 3D shapes in the scene
	CreateGLTexture("textures/wood.jpg", "wood");
	CreateGLTexture("textures/metal.jpg", "metal");

	// bind all of the loaded textures to their OpenGL texture memory slots
	BindGLTextures();

	// define the materials and configure the lights that will be
	// used by the Phong lighting model when rendering the scene
	DefineObjectMaterials();
	SetupSceneLights();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	m_basicMeshes->LoadBoxMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by 
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	
	/* BACK WALL */
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 7.0f, -5.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.144f, 0.144f, 0.144f, 1.0f);
	// apply the painted material so the wall reflects the lights
	SetShaderMaterial("painted");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/* FLOOR */
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	SetShaderColor(0.125, 0.085, 0.003,	1);
	// apply the painted material so the floor reflects the lights
	SetShaderMaterial("painted");

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/****************************************************************/

	/* DESK  */
	/******************************************************************/
	scaleXYZ = glm::vec3(20.0f, 0.4f, 9.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 4.0f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// apply the tiled wood texture to give the desk 
	SetShaderTexture("darkWood");
	// tile the texture so the grain is repeated
	// instead of being stretched across the whole object
	SetTextureUVScale(4.0f, 2.0f);
	// apply the wood material for a soft, matte highlight
	SetShaderMaterial("darkWood");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/* DESK FRONT APRON  */
	// a thin recessed rail under the front edge of the desktop adds
	// visual thickness and a shadow line so the top does not look like
	// a single floating slab
	/******************************************************************/
	scaleXYZ = glm::vec3(19.0f, 1.2f, 0.3f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 3.4f, 4.3f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	SetShaderTexture("darkWood");
	SetTextureUVScale(4.0f, 1.0f);
	// use the darker wood tone so the apron reads as a recessed panel
	SetShaderMaterial("darkWood");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/* LEFT DRAWER CABINET  */
	/******************************************************************/
	scaleXYZ = glm::vec3(3.5f, 4.0f, 8.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(-7.5f, 1.8f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// apply the brushed-metal texture to the drawer cabinet
	SetShaderTexture("metal");
	// light tiling keeps the vertical brushed grain crisp without stretching
	SetTextureUVScale(1.0f, 2.0f);
	// apply the metal material for a bright, reflective highlight
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/* LEFT DRAWER FRONTS & HANDLES  */
	// three stacked drawer faces with small handle bars break up the
	// flat cabinet side so it reads as an actual set of drawers
	/******************************************************************/
	for (int i = 0; i < 3; i++)
	{
		float drawerY = 0.6f + i * 1.15f;

		// drawer face - a thin panel slightly proud of the cabinet front
		scaleXYZ = glm::vec3(3.2f, 1.0f, 0.15f);
		SetTransformations(
			scaleXYZ,
			0.0f, 0.0f, 0.0f,
			glm::vec3(-7.5f, drawerY, 4.1f));
		SetShaderTexture("metal");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderMaterial("darkWood");
		m_basicMeshes->DrawBoxMesh();

		// drawer handle - a small dark bar centered on the drawer face
		scaleXYZ = glm::vec3(1.4f, 0.18f, 0.18f);
		SetTransformations(
			scaleXYZ,
			0.0f, 0.0f, 0.0f,
			glm::vec3(-7.5f, drawerY, 4.25f));
		SetShaderMaterial("handle");
		m_basicMeshes->DrawBoxMesh();
	}
	/****************************************************************/

	/*  RIGHT DRAWER CABINET  */
	/******************************************************************/
	scaleXYZ = glm::vec3(3.5f, 4.0f, 8.0f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(7.5f, 1.8f, 0.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// apply the brushed-metal texture to the drawer cabinet
	SetShaderTexture("metal");
	// light tiling keeps the vertical brushed grain crisp without stretching
	SetTextureUVScale(1.0f, 2.0f);
	// apply the metal material for a bright, reflective highlight
	SetShaderMaterial("metal");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/


	/*  RIGHT DRAWER FRONTS & HANDLES  */
	// matching stack of drawer faces and handles on the right cabinet
	/******************************************************************/
	for (int i = 0; i < 3; i++)
	{
		float drawerY = 0.6f + i * 1.15f;

		// drawer face - a thin panel slightly proud of the cabinet front
		scaleXYZ = glm::vec3(3.2f, 1.0f, 0.15f);
		SetTransformations(
			scaleXYZ,
			0.0f, 0.0f, 0.0f,
			glm::vec3(7.5f, drawerY, 4.1f));
		SetShaderTexture("metal");
		SetTextureUVScale(1.0f, 1.0f);
		SetShaderMaterial("darkWood");
		m_basicMeshes->DrawBoxMesh();

		// drawer handle - a small dark bar centered on the drawer face
		scaleXYZ = glm::vec3(1.4f, 0.18f, 0.18f);
		SetTransformations(
			scaleXYZ,
			0.0f, 0.0f, 0.0f,
			glm::vec3(7.5f, drawerY, 4.25f));
		SetShaderMaterial("handle");
		m_basicMeshes->DrawBoxMesh();
	}
	/****************************************************************/


	/*  TOP MONITOR  */ 
	/******************************************************************/
	scaleXYZ = glm::vec3(7.0f, 3.0f, 0.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 10.5f, -3.0f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// black monitor screen
	SetShaderColor(0.05f, 0.05f, 0.05f, 0.9f);
	// apply the glass material for a sharp, glossy highlight
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/

	/*  BOTTOM MONITOR  */
	/******************************************************************/
	scaleXYZ = glm::vec3(8.0f, 3.5f, 0.2f);
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;
	positionXYZ = glm::vec3(0.0f, 7.0f, -1.5f);
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);
	// black monitor screen
	SetShaderColor(0.05f, 0.05f, 0.05f, 0.9f);
	// apply the glass material for a sharp, glossy highlight
	SetShaderMaterial("glass");
	m_basicMeshes->DrawBoxMesh();
	/****************************************************************/
}
