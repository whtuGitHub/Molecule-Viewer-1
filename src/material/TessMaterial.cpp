#include "material/TessMaterial.h"
#include <string.h>
#include <cassert>
#include <stdio.h>
#include "scene/Scene.h"

TessMaterial::TessMaterial():Material(){
	this->type = TESS_MATERIAL;
	this->vertexShaderSource= strdup(
		"#version 440 core\n\
		#define NUM_OBJECTS %d\n\
		in vec3 normal;\n\
		in vec3 position;\n\
		in int drawID;\n\
		out vec3 vNormal;\n\
		out vec3 vPos;\n\
		flat out int vDrawID;\n\
		void main(){\n\
			vPos = position;\n\
			vNormal = normal;\n\
		}");
	this->tessControlShaderSource = strdup(
		"#version 440\n\
		#define NUM_OBJECTS %d\n\
		layout(vertices = 3) out;\n\
		in vec3 vNormal[];\n\
		in vec3 vPos[];\n\
		flat in int vDrawID;\n\
		flat out int tcDrawID;\n\
		out vec3 tcPos[];\n\
		out vec3 tcNormal[];\n\
		struct Indices{\n\
			int materialIndex;\n\
			int visible;\n\
			float distanceToCamera;\n\
		};\n\
		layout(std140) uniform materialIndices{\n\
			Indices index[NUM_OBJECTS];\n\
		};\n\
		\n\
		#define ID gl_InvocationID\n\
		\n\
		void main(){\n\
			tcPos[ID] = vPos[ID];\n\
			tcNormal[ID] = vNormal[ID];\n\
			float distanceToCamera = materialIndices[vDrawID].distanceToCamera;\n\
			if(ID ==0){\n\
				int inner = 1;\n\
				int outer = 1;\n\
				if(distanceToCamera > 80){\n\
					inner= 1;\n\
					outer =1;\n\
				} else if (distanceToCamera > 65){\n\
					inner = 1;\n\
					outer = 2;\n\
				} else if (distanceToCamera > 30){\n\
					inner=2;\n\
					outer=2;\n\
				} else if (distanceToCamera > 10){\n\
					inner=3;\n\
					outer=3;\n\
				} else if (distanceToCamera > 0){\n\
					inner=4;\n\
					outer=4;\n\
				}\n\
				gl_TessLevelInner[0] = inner;\n\
				gl_TessLevelOuter[0] = outer;\n\
				gl_TessLevelOuter[1] = outer;\n\
				gl_TessLevelOuter[2] = outer;\n\
			}\n\
		}\n\
		");
	this->tessEvaluationShaderSource= strdup(
		"#version 440\n\
		#define NUM_OBJECTS %d\n\
		layout(triangles, equal_spacing, ccw) in;\n\
		in vec3 tcPos[];\n\
		in vec3 tcNormal[];\n\
		flat in int tcDrawID;\n\
		flat out int teDrawID;\n\
		out vec4 tePos;\n\
		out vec4 teNormal;\n\
		layout(std430) buffer modelMatrices{\n\
			mat4 modelMatrix[NUM_OBJECTS];\n\
		};\n\
		layout(std140) uniform globalMatrices{\n\
			mat4 worldMatrix;\n\
			mat4 projectionMatrix;\n\
		};\n\
		void main(){\n\
			vec3 p0 = gl_TessCoord.x * tcPos[0];\n\
			vec3 p1 = gl_TessCoord.y * tcPos[1];\n\
			vec3 p2 = gl_TessCoord.z * tcPos[2];\n\
			\n\
			vec3 n0 = gl_TessCoord.x * tcNormal[0];\n\
			vec3 n1 = gl_TessCoord.y * tcNormal[1];\n\
			vec3 n2 = gl_TessCoord.z * tcNormal[2];\n\
			tePos = vec4(normalize(p0+p1+p2),1.0);\n\
			vec3 n = normalize(n0+n1+n2);\n\
			teNormal = worldMatrix * modelMatrix[tcDrawID] * vec4(n,0.0);\n\
			gl_Position = projectionMatrix * worldMatrix * modelMatrix[tcDrawID] * tePos;\n\
		}");
    this->fragmentShaderSource=strdup(
    	"#version 440\n\
    	#define NUM_OBJECTS %d\n\
    	#define NUM_MATERIALS %d\n\
    	#define MAX_DIR_LIGHTS %d\n\
		#define MAX_P_LIGHTS %d\n\
		#if MAX_DIR_LIGHTS > 0\n\
		struct DirectionalLight{\n\
			vec4 color;\n\
			vec4 vectorToLight;\n\
			float intensity;\n\
		};\n\
		#endif\n\
		\n\
		#if MAX_P_LIGHTS > 0\n\
		struct PointLight{\n\
			vec4 color;\n\
			vec4 position;\n\
			float intensity;\n\
			float attenuation;\n\
		};\n\
		#endif\n\
		\n\
		struct Material{\n\
			vec4 diffuseColor;\n\
			vec4 specularColor;\n\
			float shininess;\n\
		};\n\
		\n\
		#if MAX_DIR_LIGHTS > 0\n\
		layout(std140) uniform directionalLights{\n\
			DirectionalLight dirLights[MAX_DIR_LIGHTS];\n\
		};\n\
		#endif\n\
		#if MAX_P_LIGHTS > 0\n\
		layout(std140) uniform pointLights{\n\
			PointLight pLights[MAX_P_LIGHTS];\n\
		};\n\
		#endif\n\
		layout(std140) uniform ambLight{\n\
			vec4 ambientLight;\n\
		};\n\
		\n\
		struct Indices{\n\
			int materialIndex;\n\
			int visible;\n\
			float distanceToCamera;\n\
		};\n\
		layout(std140) uniform materialIndices{\n\
			Indices index[NUM_OBJECTS];\n\
		};\n\
		layout(std140) uniform materials{\n\
			Material material[NUM_MATERIALS];\n\
		};\n\
    	in vec4 teNormal;\n\
		in vec4 tePos;\n\
		flat in int teDrawID;\n\
    	out vec4 outputColor;\n\
    	#if MAX_P_LIGHTS > 0\n\
    	vec4 attenuateLight(in vec4 color, in float attenuation, in vec4 vectorToLight){\n\
			float distSqr = dot(vectorToLight,vectorToLight);\n\
			vec4 attenLightIntensity = color * (1/(1.0 + attenuation * sqrt(distSqr)));\n\
			return attenLightIntensity;\n\
    	}\n\
    	#endif\n\
    	\n\
    	float warp (in float value,in float factor){\n\
    		return (value + factor ) / (1+ clamp(factor,0,1));\n\
    	}\n\
    	float calculateBlinnPhongTerm(in vec4 direction,vec4 normal, in vec4 viewDirection, in float shininess, out float cosAngIncidence){\n\
    		cosAngIncidence = dot( normal , direction);\n\
    		cosAngIncidence = warp(cosAngIncidence,1);\n\
            cosAngIncidence = clamp(cosAngIncidence, 0, 1);\n\
            vec4 halfAngle = normalize(direction + viewDirection);\n\
			float blinnPhongTerm = dot(normal, halfAngle);\n\
			blinnPhongTerm = clamp(blinnPhongTerm, 0, 1);\n\
			blinnPhongTerm = cosAngIncidence != 0.0 ? blinnPhongTerm : 0.0;\n\
			blinnPhongTerm = pow(blinnPhongTerm, shininess);\n\
			return blinnPhongTerm;\n\
    	}\n\
    	\n\
    	void main(){\n\
    		vec4 viewDirection = normalize(-tePos);\n\
			outputColor = vec4(0.0,0.0,0.0,1.0);\n\
			#if MAX_DIR_LIGHTS > 0\n\
			for(int i=0; i< MAX_DIR_LIGHTS ;i++){\n\
				vec4 normDirection = normalize(dirLights[i].vectorToLight);\n\
				vec4 normal = normalize(teNormal);\n\
				float cosAngIncidence;\n\
				float blinnPhongTerm = calculateBlinnPhongTerm(normDirection,normal,viewDirection,material.shininess,cosAngIncidence);\n\
				\n\
            	outputColor = outputColor + (dirLights[i].color * material[index[teDrawID].materialIndex].diffuseColor * cosAngIncidence);\n\
            	outputColor = outputColor + (material[index[teDrawID].materialIndex].specularColor * blinnPhongTerm);\n\
			}\n\
			#endif\n\
			#if MAX_P_LIGHTS > 0\n\
			for(int i=0; i< MAX_P_LIGHTS ;i++){\n\
				vec4 difference = pLights[i].position - tePos;\n\
				vec4 normDirection = normalize(difference);\n\
				vec4 attenLightIntensity = attenuateLight(pLights[i].color,pLights[i].attenuation,difference);\n\
				vec4 normal = normalize(teNormal);\n\
				float cosAngIncidence;\n\
				float blinnPhongTerm = calculateBlinnPhongTerm(normDirection,normal,viewDirection,material.shininess,cosAngIncidence);\n\
				\n\
            	outputColor = outputColor + (attenLightIntensity * material[index[teDrawID].materialIndex].diffuseColor * cosAngIncidence);\n\
            	outputColor = outputColor + (material[index[teDrawID].materialIndex].specularColor * attenLightIntensity * blinnPhongTerm);\n\
			}\n\
			#endif\n\
            outputColor = outputColor + (material[index[teDrawID].materialIndex].diffuseColor * ambientLight);\n\
    	}");
}
void TessMaterial::makePrograms(Scene* scene){
	this->program = new GLProgram();
	char* vs = this->configureSource(this->vertexShaderSource,scene->getDirectionalLights().size(),scene->getPointLights().size());
	char* fs = this->configureSource(this->fragmentShaderSource,scene->getDirectionalLights().size(),scene->getPointLights().size(),scene->getObjects().size(),scene->getGeometries().size());
	char* tcs = this->configureSource(this->tessControlShaderSource,scene->getDirectionalLights().size(),scene->getPointLights().size(),scene->getObjects().size(),scene->getGeometries().size());
	char* tes = this->configureSource(this->tessEvaluationShaderSource,scene->getDirectionalLights().size(),scene->getPointLights().size(),scene->getObjects().size(),scene->getGeometries().size());
	GLuint vertexShader = this->program->compileShader(GL_VERTEX_SHADER,vs);
	delete vs;
	GLuint fragmentShader = this->program->compileShader(GL_FRAGMENT_SHADER,fs);
	delete fs;
	GLuint tessControlShader = this->program->compileShader(GL_TESS_CONTROL_SHADER,tcs);
	delete tcs;
	GLuint tessEvaluationShader = this->program->compileShader(GL_TESS_EVALUATION_SHADER,tes);
	delete tes;
	this->program->setVertexShader(vertexShader);
	this->program->setFragmentShader(fragmentShader);
	this->program->setTessControlShader(tessControlShader);
	this->program->setTessEvaluationShader(tessEvaluationShader);
	GLuint prog = this->program->linkProgramTessellation(vertexShader,fragmentShader,tessControlShader,tessEvaluationShader);
	this->program->setProgram(prog);
	this->program->setAttrPosition(glGetAttribLocation(prog, "position"));
	this->program->setAttrNormal(glGetAttribLocation(prog, "normal"));
	this->program->setAttrDrawID(glGetAttribLocation(prog, "drawID"));
	this->program->getUniforms()->unifModelMatrix =  glGetProgramResourceIndex(prog,GL_SHADER_STORAGE_BLOCK,"modelMatrices");
	glShaderStorageBlockBinding(prog, this->program->getUniforms()->unifModelMatrix,MODEL_MATRICES_UBI);
	this->program->getUniforms()->unifMaterial = glGetUniformBlockIndex(prog,"materials");
	glUniformBlockBinding(prog, this->program->getUniforms()->unifMaterial,MATERIALS_UBI);
	this->program->getUniforms()->unifIndices = glGetUniformBlockIndex(prog,"materialIndices");
	glUniformBlockBinding(prog, this->program->getUniforms()->unifIndices,INDICES_UBI);
	this->program->getUniforms()->unifBlockMatrices = glGetUniformBlockIndex(prog,"globalMatrices");
	glUniformBlockBinding(prog, this->program->getUniforms()->unifBlockMatrices,GLOBAL_MATRICES_UBI);
	this->program->getUniforms()->unifBlockDirectionalLights = glGetUniformBlockIndex(prog,"directionalLights");
	glUniformBlockBinding(prog, this->program->getUniforms()->unifBlockDirectionalLights,DIRLIGHTS_UBI);
	this->program->getUniforms()->unifBlockAmbientLight = glGetUniformBlockIndex(prog,"ambLight");
	glUniformBlockBinding(prog, this->program->getUniforms()->unifBlockAmbientLight,AMBLIGHT_UBI);
	this->program->getUniforms()->unifBlockPointLights = glGetUniformBlockIndex(prog,"pointLights");
	glUniformBlockBinding(prog, this->program->getUniforms()->unifBlockPointLights,PLIGHTS_UBI);
}