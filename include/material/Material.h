#ifndef MATERIAL_H
#define MATERIAL_H
#include <GL/glew.h>
#include "render/GLProgram.h"
#include "math/Color.h"
//materials own shaders (vertex, fragment), programs and uniforms
//future adds -> opacity, bumpmaps, textures, normal maps
//future -> make material memory self managed
class Scene;

enum MaterialType {BASIC_MATERIAL,GOURAUD_MATERIAL,PHONG_MATERIAL,TESS_MATERIAL,CEL_MATERIAL,POINT_MATERIAL,LINE_MATERIAL};
enum {GLOBAL_MATRICES_UBI,DIRLIGHTS_UBI,AMBLIGHT_UBI,PLIGHTS_UBI,MODEL_MATRICES_UBI,MATERIALS_UBI,INDICES_UBI};
#define NUM_MATERIAL_TYPES 7

struct materialStruct{
	GLfloat diffuseColor[4];
	GLfloat specularColor[4];
	GLfloat shininess;
	GLfloat stride[3];
};

typedef struct materialStruct * MaterialStruct;

class Material{
protected:
	Color* diffuseColor;
	Color* specularColor;
	GLfloat shininess;
	GLchar* vertexShaderSourceTemplate;
	GLchar* fragmentShaderSourceTemplate;
	GLchar* vertexShaderSource;
	GLchar* fragmentShaderSource;
	GLProgram* program;
	int sceneIndex;
	MaterialType type;
public:
	Material();
	virtual ~Material();
	GLProgram* getProgram();
	void setProgram(GLProgram* program);
	GLchar* getVertexShaderSource();
	void setVertexShaderSource(char * vertexShaderSource);
	GLchar* getFragmentShaderSource();
	void setFragmentShaderSource(char * fragmentShaderSource);
	void setDiffuseColor(Color* color);
	Color* getDiffuseColor();
	void setSpecularColor(Color* color);
	Color* getSpecularColor();
	void setShininess(GLfloat shininess);
	GLfloat getShininess();
	MaterialStruct getAsStruct();
	MaterialType getType();
	char* configureSource(char* source,int numDirLights, int numPLights, int numObjects=0, int numMaterials =0);
	int getSceneIndex();
	void setSceneIndex(int index);
	virtual void makePrograms(Scene* scene);
};
#endif