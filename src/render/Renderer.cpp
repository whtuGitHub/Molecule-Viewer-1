#include "render/Renderer.h"
#include <list>
#include "object/Object3D.h"
#include <cstdlib>
#include "object/Mesh.h"
#include "scene/Scene.h"
#include "material/PointMaterial.h"

Renderer::Renderer(){
	this->vao=0;
	this->buffers=NULL;
}

GLuint Renderer::makeBuffer(GLenum target, void* bufferData, GLsizei bufferSize){
	GLuint buffer;
	glGenBuffers(1,&buffer);
	glBindBuffer(target,buffer);
	glBufferData(target,bufferSize,bufferData, GL_STATIC_DRAW);
	return buffer;
}
GLuint Renderer::makeUBO(void* bufferData, GLsizei bufferSize){
	GLuint buf;
	glGenBuffers(1,&buf);
	glBindBuffer(GL_UNIFORM_BUFFER,buf);
	glBufferData(GL_UNIFORM_BUFFER,bufferSize,bufferData, GL_STREAM_DRAW);
	return buf;
}

GLuint Renderer::makePointBuffer(GLenum target, void* bufferData, GLsizei bufferSize){
	GLuint buffer;
	glGenBuffers(1,&buffer);
	glBindBuffer(target,buffer);
	glBufferData(target,bufferSize,bufferData, GL_STREAM_DRAW);
	return buffer;
}

GLuint Renderer::calculateDirectionalLights(Scene* scene){
	GLint numLights = scene->getDirectionalLights().size();
	if(numLights == 0) return 0;
	int bufferSize = sizeof(struct dirLight) * numLights;
	if(scene->getDirectionalLightsUBO() == 0){
		GLuint buf = makeUBO(NULL, bufferSize);
		scene->setDirectionalLightsUBO(buf);
		glBindBufferRange(
			GL_UNIFORM_BUFFER,//target
			DIRLIGHTS_UBI,//binding point
			buf,//data
			0,//offset
			bufferSize//size in bytes
		);
	}
	
	list<DirectionalLight*> lightsList = scene->getDirectionalLights();
	list<DirectionalLight*>::iterator itLights = lightsList.begin();
	list<DirectionalLight*>::iterator endLights = lightsList.end();
	struct dirLight lights[numLights];
	for(int i=0;itLights != endLights && i<10 ;itLights++ , i++){
		DirLight light = (*itLights)->getAsStruct(scene->getCamera());
		lights[i]= *light;
		delete light;
	}

	glBindBuffer(GL_UNIFORM_BUFFER,scene->getDirectionalLightsUBO());
	glBufferSubData(GL_UNIFORM_BUFFER,0,bufferSize,&lights);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return scene->getDirectionalLightsUBO();
}

GLuint Renderer::calculatePointLights(Scene* scene){
	GLint numLights = scene->getPointLights().size();
	if(numLights == 0) return 0;
	int bufferSize = sizeof(struct pLight)*numLights;
	if(scene->getPointLightsUBO() == 0){
		GLuint buf = makeUBO(NULL, bufferSize);
		scene->setPointLightsUBO(buf);
		glBindBufferRange(
			GL_UNIFORM_BUFFER,//target
			PLIGHTS_UBI,//binding point
			buf,//data
			0,//offset
			bufferSize//size in bytes
		);
	}
	
	list<PointLight*> lightsList = scene->getPointLights();
	list<PointLight*>::iterator itLights = lightsList.begin();
	list<PointLight*>::iterator endLights = lightsList.end();
	struct pLight lights[numLights];

	for(int i=0;itLights != endLights && i<10 ;itLights++ , i++){
		PLight light = (*itLights)->getAsStruct(scene->getCamera());
		lights[i] = *light;
		delete light;
	}

	glBindBuffer(GL_UNIFORM_BUFFER,scene->getPointLightsUBO());
	glBufferSubData(GL_UNIFORM_BUFFER,0,bufferSize,&lights);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return scene->getPointLightsUBO();
}

GLuint Renderer::calculateAmbientLights(Scene* scene){
	if(scene->getAmbientLightUBO() == 0){
		GLfloat* data = scene->getAmbientLight()->getColor()->getAsArray();
		GLuint buf = makeUBO(data,sizeof(GLfloat)*4);
		scene->setAmbientLightUBO(buf);
		glBindBufferRange(
			GL_UNIFORM_BUFFER,//target
			AMBLIGHT_UBI,//binding point
			buf,//data
			0,//offset
			sizeof(GLfloat) * 4//size in bytes
		);
		delete[] data;
	}
	return scene->getAmbientLightUBO();
}

GLuint Renderer::calculateGlobalMatrices(Scene* scene){
	//update UBO TODO: update world matrix only as projection never changes
	scene->getCamera()->updateWorldMatrix();
	GLfloat* data = scene->getCamera()->getMatricesArray();
	if(scene->getCamera()->getMatricesUBO()==0){	
		GLuint buf = makeUBO(data,sizeof(GLfloat)*32);
		scene->getCamera()->setMatricesUBO(buf);
		glBindBufferRange(
			GL_UNIFORM_BUFFER,//target
			GLOBAL_MATRICES_UBI,//binding point
			buf,//data
			0,//offset
			sizeof(GLfloat) * 32//size in bytes
		);
	}else{
		glBindBuffer(GL_UNIFORM_BUFFER,scene->getCamera()->getMatricesUBO());
		glBufferSubData(GL_UNIFORM_BUFFER,0,sizeof(GLfloat)*32, data);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	delete[] data;
	return scene->getCamera()->getMatricesUBO();
}

void Renderer::setMaterialUniforms(Material* material,Scene* scene){
	
	//set diffuse color
	GLfloat* diffuseColor = material->getDiffuseColor()->getAsArray();
	glUniform4fv(
		material->getProgram()->getUniforms()->unifDiffuseColor,
		1,
		diffuseColor
	);
	delete diffuseColor;

	//set specular color
	GLfloat* specularColor = material->getSpecularColor()->getAsArray();
	glUniform4fv(
		material->getProgram()->getUniforms()->unifSpecularColor,
		1,
		specularColor
	);
	delete specularColor;

	//set shininess
	GLfloat shininess = material->getShininess();
	glUniform1fv(
		material->getProgram()->getUniforms()->unifShininess,
		1,
		&shininess
	);
}

void Renderer::render(Scene * scene){
	list<Object3D*> objects = scene->getObjects();
	list<Object3D*>::iterator it = objects.begin();
	list<Object3D*>::iterator end = objects.end();
	//vao initialization;
	if(this->vao == 0){
		glGenVertexArrays(1, &(this->vao));
		glBindVertexArray(this->vao);
	}

	//AmbientLight UBO
	this->calculateAmbientLights(scene);

	//Camera UBO initialization
	this->calculateGlobalMatrices(scene);

	//DirectionalLightsUBO

	this->calculateDirectionalLights(scene);

	//PointLightsUBO

	this->calculatePointLights(scene);

	for(;it != end;it++){
		Mesh* mesh= (Mesh*)(*it);
		if(!mesh->getVisible()) continue;
		if(mesh->getGeometry()->getVertexBuffer() == 0 && mesh->getGeometry()->getVertices() != NULL){
			GLuint buf = this->makeBuffer(GL_ARRAY_BUFFER,
							mesh->getGeometry()->getVertices(),
							mesh->getGeometry()->getNumVertices() * sizeof(GLfloat)
							);
            mesh->getGeometry()->setVertexBuffer(buf);
		}
		if(mesh->getGeometry()->getElementBuffer() == 0 && mesh->getGeometry()->getElements() != NULL){
			GLuint buf = this->makeBuffer(GL_ELEMENT_ARRAY_BUFFER,
							mesh->getGeometry()->getElements(),
							mesh->getGeometry()->getNumElements() * sizeof(GLushort)
							);
            mesh->getGeometry()->setElementBuffer(buf);
		}
		if(mesh->getGeometry()->getNormalBuffer() == 0 && mesh->getGeometry()->getNormals() != NULL){
			GLuint buf = this->makeBuffer(GL_ELEMENT_ARRAY_BUFFER,
							mesh->getGeometry()->getNormals(),
							mesh->getGeometry()->getNumNormals() * sizeof(GLfloat)
							);
            mesh->getGeometry()->setNormalBuffer(buf);
		}

		//compile the program if i'ts not compiled
		if(mesh->getMaterial()->getProgram() == NULL){
			mesh->getMaterial()->makePrograms(scene->getDirectionalLights().size(),scene->getPointLights().size());
		}
		//set vertex attribute
		glBindBuffer(GL_ARRAY_BUFFER,mesh->getGeometry()->getVertexBuffer());
		glVertexAttribPointer(
			mesh->getMaterial()->getProgram()->getAttrPosition(),//attribute from prgram(position)
			3,//number of components per vertex
			GL_FLOAT,//type of data
			GL_FALSE,//normalized
			0,//separation between 2 values
			(void*)0 //offset
		);
		glEnableVertexAttribArray(mesh->getMaterial()->getProgram()->getAttrPosition());

		//set normal attribute
		if(mesh->getGeometry()->getNormalBuffer() != 0){
			glBindBuffer(GL_ARRAY_BUFFER,mesh->getGeometry()->getNormalBuffer());
			glVertexAttribPointer(
				mesh->getMaterial()->getProgram()->getAttrNormal(),//attribute from prgram(position)
				3,//number of components per vertex
				GL_FLOAT,//type of data
				GL_FALSE,//normalized
				0,//separation between 2 values
				(void*)0 //offset
			);
			glEnableVertexAttribArray(mesh->getMaterial()->getProgram()->getAttrNormal());
		}
		
		glUseProgram(mesh->getMaterial()->getProgram()->getProgram());

		//set model matrix
		mesh->updateModelMatrix();
		glUniformMatrix4fv(
			mesh->getMaterial()->getProgram()->getUniforms()->unifModelMatrix,
			1,
			GL_TRUE,
			mesh->getModelMatrix()->getElements()
		);
		GLfloat dist = mesh->getDistanceToCamera();
		glUniform1fv(
			mesh->getMaterial()->getProgram()->getUniforms()->unifDistanceToCamera,
			1,
			&dist
		);
		//set material uniforms
		setMaterialUniforms(mesh->getMaterial(),scene);
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->getGeometry()->getElementBuffer());
		if(mesh->getMaterial()->getType() == TESS_MATERIAL){
			glPatchParameteri(GL_PATCH_VERTICES, 3);
			glDrawElements(
				GL_PATCHES, //drawing mode
				mesh->getGeometry()->getNumElements(), //count
				GL_UNSIGNED_SHORT, //type,
				(void*)0 //offset
			);
		}
		else{
			glDrawElements(
				GL_TRIANGLES, //drawing mode
				mesh->getGeometry()->getNumElements(), //count
				GL_UNSIGNED_SHORT, //type,
				(void*)0 //offset
			);
		}
		
		glDisableVertexAttribArray(mesh->getMaterial()->getProgram()->getAttrPosition());
	}
	//this->renderOctreeNode(scene->getOctree());
}

void Renderer::renderOctreeNode(OctreeNode* node, Scene* scene){

	Mesh* mesh = node->getBoundingBox();
	if(mesh->getGeometry()->getVertexBuffer() == 0 && mesh->getGeometry()->getVertices() != NULL){
		GLuint buf = this->makeBuffer(GL_ARRAY_BUFFER,
						mesh->getGeometry()->getVertices(),
						mesh->getGeometry()->getNumVertices() * sizeof(GLfloat)
						);
        mesh->getGeometry()->setVertexBuffer(buf);
	}

	if(mesh->getGeometry()->getElementBuffer() == 0 && mesh->getGeometry()->getElements() != NULL){
		GLuint buf = this->makeBuffer(GL_ELEMENT_ARRAY_BUFFER,
						mesh->getGeometry()->getElements(),
						mesh->getGeometry()->getNumElements() * sizeof(GLushort)
						);
        mesh->getGeometry()->setElementBuffer(buf);
	}

	if(mesh->getGeometry()->getNormalBuffer() == 0 && mesh->getGeometry()->getNormals() != NULL){
		GLuint buf = this->makeBuffer(GL_ELEMENT_ARRAY_BUFFER,
						mesh->getGeometry()->getNormals(),
						mesh->getGeometry()->getNumNormals() * sizeof(GLfloat)
						);
        mesh->getGeometry()->setNormalBuffer(buf);
	}
	//set vertex attribute

	glBindBuffer(GL_ARRAY_BUFFER,mesh->getGeometry()->getVertexBuffer());
	glVertexAttribPointer(
		mesh->getMaterial()->getProgram()->getAttrPosition(),//attribute from prgram(position)
		3,//number of components per vertex
		GL_FLOAT,//type of data
		GL_FALSE,//normalized
		0,//separation between 2 values
		(void*)0 //offset
	);
	glEnableVertexAttribArray(mesh->getMaterial()->getProgram()->getAttrPosition());

	//set normal attribute
	if(mesh->getGeometry()->getNormalBuffer() != 0){
		glBindBuffer(GL_ARRAY_BUFFER,mesh->getGeometry()->getNormalBuffer());
		glVertexAttribPointer(
			mesh->getMaterial()->getProgram()->getAttrNormal(),//attribute from prgram(position)
			3,//number of components per vertex
			GL_FLOAT,//type of data
			GL_FALSE,//normalized
			0,//separation between 2 values
			(void*)0 //offset
		);
		glEnableVertexAttribArray(mesh->getMaterial()->getProgram()->getAttrNormal());
	}

	glUseProgram(mesh->getMaterial()->getProgram()->getProgram());

	//set model matrix
	mesh->updateModelMatrix();
	glUniformMatrix4fv(
		mesh->getMaterial()->getProgram()->getUniforms()->unifModelMatrix,
		1,
		GL_TRUE,
		mesh->getModelMatrix()->getElements()
	);

	//set material uniforms
	setMaterialUniforms(mesh->getMaterial(),scene);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh->getGeometry()->getElementBuffer());
	glDrawElements(
		GL_LINES, //drawing mode
		mesh->getGeometry()->getNumElements(), //count
		GL_UNSIGNED_SHORT, //type,
		(void*)0 //offset
	);
	glDisableVertexAttribArray(mesh->getMaterial()->getProgram()->getAttrPosition());
	if(!node->isDivided())return;
	OctreeNodeIterator it = node->children.begin();
	OctreeNodeIterator end = node->children.end();
	for(;it != end;it++){
		renderOctreeNode((*it),scene);
	}
}

BufferObjects Renderer::getBuffers(){
	return this->buffers;
}

GLuint Renderer::createMaterialBuffer(Scene* scene){
	list<Material*> materials = scene->getMaterials();
	int numMaterials = materials.size();
	MaterialStruct materialList = new struct materialStruct[numMaterials];

	list<Material*>::iterator it = materials.begin();

	for(int i=0 ;it != materials.end(); it++ , i++){
		MaterialStruct mat = (*it)->getAsStruct(); 
		materialList[i] = *mat;
		delete mat;
		if ((*it)->getProgram() == NULL){
			(*it)->makePrograms(scene->getDirectionalLights().size(),scene->getPointLights().size());
		}
	}

	GLuint ubo = makeUBO((void*)materialList, numMaterials * sizeof(struct materialStruct));
	delete[] materialList;
	return ubo;
}

GLuint* Renderer::createGeometryBuffers(Scene* scene){
	GLuint* buffers = new GLuint[3];
	list<Geometry*> geometries= scene->getGeometries();
	list<Geometry*>::iterator it = geometries.begin();
	int totalNumElements=0;
	int totalNumVertices=0;
	int totalNumNormals=0;
	for(;it != geometries.end();it++){
		(*it)->setSceneIndicesOffset(totalNumElements);
		(*it)->setSceneVerticesOffset(totalNumVertices);
		totalNumElements += (*it)->getNumElements();
		totalNumVertices += (*it)->getNumVertices();
		totalNumNormals += (*it)->getNumNormals();
	}
	GLfloat* vertices = new GLfloat[totalNumVertices];
	GLfloat* normals = new GLfloat[totalNumNormals];
	GLushort* elements = new GLushort[totalNumElements];
	GLfloat* ptrVertices = vertices;
	GLfloat* ptrNormals = normals;
	GLushort* ptrElements = elements;
	for(it = geometries.begin(); it != geometries.end();it++){
		Geometry* geom = *it;
		memcpy(ptrVertices,geom->getVertices(),sizeof(GL_FLOAT)*geom->getNumVertices());
		ptrVertices += geom->getNumVertices();
		memcpy(ptrNormals,geom->getNormals(),sizeof(GL_FLOAT)*geom->getNumNormals());
		ptrNormals += geom->getNumNormals();
		memcpy(ptrElements,geom->getElements(),sizeof(GL_UNSIGNED_SHORT)*geom->getNumElements());
		ptrElements += geom->getNumElements();
	}
	buffers[VERTICES] = this->makeBuffer(
		GL_ARRAY_BUFFER,
		vertices,
		sizeof(GL_FLOAT)*totalNumVertices
	);
	buffers[ELEMENTS] = this->makeBuffer(
		GL_ELEMENT_ARRAY_BUFFER,
		elements,
		sizeof(GL_UNSIGNED_SHORT)*totalNumElements
	);
	buffers[NORMALS] = this->makeBuffer(
		GL_ARRAY_BUFFER,
		normals,
		sizeof(GL_FLOAT)*totalNumNormals
	);
	delete[] vertices;
	delete[] normals;
	delete[] elements;
	return buffers;
}

GLuint* Renderer::createObjectBuffers(list<Object3D*> objectList){
	GLuint* buffers = new GLuint[3];
	list<Object3D*>::iterator it = objectList.begin();
	int size = objectList.size();
	GLfloat* matrices = new GLfloat[size *16];
	struct bufferIndices* indices = new struct bufferIndices[size];
	struct indirect* indirects = new struct indirect[size];
	GLfloat* ptrMatrices = matrices;

	for(int i=0;it!= objectList.end();it++ , i++){
		
		(*it)->updateModelMatrix();
		GLfloat* mat = (*it)->getModelMatrix()->getElements();
		memcpy(ptrMatrices,mat, sizeof(GLfloat)*16);
		ptrMatrices += 16;
		delete[] mat;

		indices[i].materialIndex = ((Mesh*)(*it))->getMaterial()->getSceneIndex();
		indices[i].visible = (GLuint)(((Mesh*)(*it))->getOctreeNode()->isVisible());

		indirects[i].count = ((Mesh*)(*it))->getGeometry()->getNumElements();
		indirects[i].instanceCount =1;
		indirects[i].firstIndex = ((Mesh*)(*it))->getGeometry()->getSceneIndicesOffset();
		indirects[i].baseVertex = ((Mesh*)(*it))->getGeometry()->getSceneVerticesOffset();
		indirects[i].baseInstance= 0;
	}
	buffers[MODEL_MATRIX] = this->makeBuffer(
		GL_UNIFORM_BUFFER,
		matrices,
		sizeof(GL_FLOAT)*size*16
	);
	buffers[BUFFER_INDICES] = this->makeBuffer(
		GL_UNIFORM_BUFFER,
		indices,
		sizeof(struct bufferIndices)*size
	);
	buffers[INDIRECT] = this->makeBuffer(
		GL_DRAW_INDIRECT_BUFFER,
		indirects,
		sizeof(struct indirect)*size
	);
	delete[] matrices;
	delete[] indirects;
	delete[] indices;
	return buffers;
}

void Renderer::renderMultiDraw(Scene* scene){
	//vao initialization;
	if(this->vao == 0){
		glGenVertexArrays(1, &(this->vao));
		glBindVertexArray(this->vao);
	}
	if(this->buffers == NULL){
		this->buffers = new struct bufferObjects;//must be an array for multimaterial rendering
		this->buffers->pointLights = 0;
		this->buffers->directionalLights = 0;
		this->buffers->ambientLight = 0;
		this->buffers->materials = 0;
		this->buffers->bufferIndices = 0;
		this->buffers->modelMatrices = 0;
		this->buffers->vertexBuffer = 0;
		this->buffers->elementBuffer = 0;
		this->buffers->normalBuffer = 0;
		this->buffers->globalMatrices =0;
		this->buffers->indirectBuffer = 0;
	}

	this->buffers->ambientLight = this->calculateAmbientLights(scene);

	this->buffers->globalMatrices = this->calculateGlobalMatrices(scene);


	this->buffers->directionalLights = this->calculateDirectionalLights(scene);


	this->buffers->pointLights = this->calculatePointLights(scene);
	
	if(this->buffers->vertexBuffer == 0){
		GLuint* buffers = this->createGeometryBuffers(scene);
		this->buffers->vertexBuffer = buffers[VERTICES];
		this->buffers->elementBuffer = buffers[ELEMENTS];
		this->buffers->normalBuffer = buffers[NORMALS];
		delete[] buffers;
	}
	if(this->buffers->indirectBuffer == 0 ||this->buffers->modelMatrices == 0 || this->buffers->bufferIndices == 0){
		GLuint* buffers = this->createObjectBuffers(scene->getObjects());
		this->buffers->bufferIndices = buffers[BUFFER_INDICES];
		this->buffers->indirectBuffer = buffers[INDIRECT];
		this->buffers->modelMatrices = buffers[MODEL_MATRIX];
		delete[] buffers;
	}

	if(this->buffers->materials == 0){
		this->buffers->materials = this->createMaterialBuffer(scene);
	}
	GLProgram* program = (*(scene->getMaterials().begin()))->getProgram();
	//set vertex attribute
	glBindBuffer(GL_ARRAY_BUFFER,this->buffers->vertexBuffer);
	glVertexAttribPointer(
		program->getAttrPosition(),//attribute from prgram(position)
		3,//number of components per vertex
		GL_FLOAT,//type of data
		GL_FALSE,//normalized
		0,//separation between 2 values
		(void*)0 //offset
	);
	glEnableVertexAttribArray(program->getAttrPosition());

	//set normal attribute

	glBindBuffer(GL_ARRAY_BUFFER,this->buffers->normalBuffer);
	glVertexAttribPointer(
		program->getAttrNormal(),//attribute from prgram(position)
		3,//number of components per vertex
		GL_FLOAT,//type of data
		GL_FALSE,//normalized
		0,//separation between 2 values
		(void*)0 //offset
	);
	glEnableVertexAttribArray(program->getAttrNormal());
	
	glUseProgram(program->getProgram());

	this->updateModelMatrices(this->buffers->modelMatrices,scene->getObjects());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,this->buffers->elementBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, this->buffers->indirectBuffer);
	glMultiDrawElementsIndirect(GL_TRIANGLES,GL_UNSIGNED_SHORT,(void*)0,scene->getObjects().size(),sizeof(struct indirect));

	glDisableVertexAttribArray(program->getAttrPosition());
	glDisableVertexAttribArray(program->getAttrNormal());
}

void Renderer::updateModelMatrices(GLuint modelMatricesBuffer,list<Object3D*> objects){
	list<Object3D*>::iterator it = objects.begin();
	int size = objects.size();
	GLfloat* matrices = new GLfloat[size *16];
	GLfloat* ptrMatrices= matrices;
	for(;it!= objects.end();it++){
		(*it)->updateModelMatrix();
		GLfloat* mat = (*it)->getModelMatrix()->getElements();
		memcpy(ptrMatrices,mat, sizeof(GLfloat)*16);
		ptrMatrices += 16;
		delete[] mat;
	}
	glBindBuffer(GL_UNIFORM_BUFFER,modelMatricesBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER,0,size*sizeof(GLfloat)*16,matrices);
	glBindBuffer(GL_UNIFORM_BUFFER,0);

}