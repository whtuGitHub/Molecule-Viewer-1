#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <cassert>

#define NO_SDL_GLEXT
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>

//#include "Molecule.h"
#include "object/Mesh.h"
#include "scene/Scene.h"
#include "render/Renderer.h"
#include "material/PhongMaterial.h"
#include "Molecule.h"
#include "math/SphericalCoord.h"

#define PI 3.1415927
#define EPS 0.000001
#define MAXANG PI
#define MINANG 0
#define NUM_SPHERES 64

Renderer* renderer;
Scene* scene;
Mesh* sphere;
DirectionalLight* light1;

const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

SDL_Window* window = NULL;
SDL_Surface* screenSurface;
SDL_GLContext context = NULL;

void initializeContext(){
	if(SDL_Init(SDL_INIT_VIDEO) < 0){
		printf("SDL could not initialize! SDL_Error: %s\n",SDL_GetError());
	}
	else{
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
		SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
		SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);
		SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 32);
		window = SDL_CreateWindow(
			"Molecule",
			SDL_WINDOWPOS_UNDEFINED,
			SDL_WINDOWPOS_UNDEFINED,
			SCREEN_WIDTH,
			SCREEN_HEIGHT,
			SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL 
		);
		if(window == NULL){
			printf("Window could not be created! SDL_Error: %s\n",SDL_GetError());
		}else{
            context = SDL_GL_CreateContext( window );
			if( context == NULL ){
				printf( "OpenGL context could not be created! SDL Error: %s\n", SDL_GetError() );
			}
			else{
				glewExperimental = GL_TRUE;
				GLenum err = glewInit();
				if (GLEW_OK != err)
				{
					fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
				}
				if(!GLEW_VERSION_4_0){
					fprintf(stderr,"OpenGL 4.0 not available\n");
					exit(-1);
				}
				const unsigned char* version = glGetString(GL_VERSION);
				if (version == NULL) {
					exit(-1);
				}
				printf("%s\n", version);
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LEQUAL);
				//glEnable(GL_CULL_FACE);
				//glCullFace(GL_BACK);
				//glFrontFace(GL_CCW);
				glEnable(GL_MULTISAMPLE);
				glHint(GL_LINE_SMOOTH_HINT, GL_NICEST );
				glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST );
				glEnable(GL_LINE_SMOOTH);
				glEnable(GL_POLYGON_SMOOTH);
				glEnable( GL_PROGRAM_POINT_SIZE);
			}
        }
	}
}

void updateLightSphericalPosition(float deltaPhi, float deltaTheta){
	Vec3* origin = new Vec3(0,0,0);
	SphericalCoord* sphCoord = new SphericalCoord(light1->getPosition(),origin);
	sphCoord->setR(10);
	float phi = sphCoord->getPhi() + deltaPhi;
	float theta = sphCoord->getTheta() + deltaTheta;
	phi = fmax(MINANG,fmin(MAXANG,phi));
	phi = fmax( EPS, fmin( PI - EPS, phi ));
	sphCoord->setPhi(phi);
	sphCoord->setTheta(theta);
	Vec3* newPos = sphCoord->getCartesian(origin);
	light1->getPosition()->setX(newPos->getX());
	light1->getPosition()->setY(newPos->getY());
	light1->getPosition()->setZ(newPos->getZ());
	delete sphCoord;
	delete origin;
	delete newPos;
}

//move this function as a method of Object3D
void updateCamSphericalPosition(float deltaPhi, float deltaTheta, float radiusFactor){
	Camera* camera = scene->getCamera();
	Vec3* molPos = new Vec3(0,0,0);
	SphericalCoord* sphCoord = new SphericalCoord(camera->getPosition(),molPos);
	float r = sphCoord->getR()*radiusFactor;
	r = fmax(0.2,fmin(99.0,r));
	sphCoord->setR(r);
	float phi = sphCoord->getPhi() + deltaPhi;
	float theta = sphCoord->getTheta() + deltaTheta;
	phi = fmax(MINANG,fmin(MAXANG,phi));
	phi = fmax( EPS, fmin( PI - EPS, phi ));
	sphCoord->setPhi(phi);
	sphCoord->setTheta(theta);
	Vec3* newPos = sphCoord->getCartesian(molPos);
	camera->getPosition()->setX(newPos->getX());
	camera->getPosition()->setY(newPos->getY());
	camera->getPosition()->setZ(newPos->getZ());
	delete sphCoord;
	delete molPos;
	delete newPos;
	scene->getOctree()->calculateVisibility(camera);
}

bool handleEvents(){
	SDL_Event event;
	while( SDL_PollEvent( &event ) ){
		switch(event.type){
			case SDL_KEYDOWN:
				switch(event.key.keysym.sym){
					case SDLK_SPACE:
						//mol->toggleSpaceFill();
						break;
					case SDLK_w:
						updateLightSphericalPosition(-0.2,0);
						break;
					case SDLK_s:
						updateLightSphericalPosition(0.2,0);
						break;
					case SDLK_a:
						updateLightSphericalPosition(0,-0.2);
						break;
					case SDLK_d:
						updateLightSphericalPosition(0,0.2);
						break;
					case SDLK_l:
						sphere->getPosition()->setX(sphere->getPosition()->getX() + 2);
						//sphere->updateOctreeNode();
						//scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_j:
						sphere->getPosition()->setX(sphere->getPosition()->getX() - 2);
						//sphere->updateOctreeNode();
						//scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_i:
						sphere->getPosition()->setY(sphere->getPosition()->getY() + 2);
						//sphere->updateOctreeNode();
						//scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_k:
						sphere->getPosition()->setY(sphere->getPosition()->getY() - 2);
						//sphere->updateOctreeNode();
						//scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_u:
						sphere->getPosition()->setZ(sphere->getPosition()->getZ() + 2);
						//sphere->updateOctreeNode();
						//scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_o:
						sphere->getPosition()->setZ(sphere->getPosition()->getZ() - 2);
						//sphere->updateOctreeNode();
						//scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_u:
						mol->getPosition()->setZ(mol->getPosition()->getZ() + 0.5);
						mol->updateOctreeNode();
						scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_o:
						mol->getPosition()->setZ(mol->getPosition()->getZ() - 0.5);
						mol->updateOctreeNode();
						scene->getOctree()->generateTreeMesh();
						break;
					case SDLK_p:
						printf("printing tree!\n");
						//scene->getOctree()->print();
						break;
				}
				break;
			case SDL_MOUSEMOTION:
				if(event.motion.state & SDL_BUTTON(1)){
					float deltaTheta = -2 * PI * event.motion.xrel / SCREEN_WIDTH;
					float deltaPhi = -2 * PI * event.motion.yrel / SCREEN_HEIGHT;
					updateCamSphericalPosition(deltaPhi,deltaTheta,1.0);
				}
				break;
			case SDL_MOUSEWHEEL:
				float factor;
				factor = event.wheel.y * 0.05;
				updateCamSphericalPosition(0,0,1-factor);
				break;
			case SDL_QUIT:
				return true;
		}
	}
	return false;
}

void render(){
	/*newTime = glutGet(GLUT_ELAPSED_TIME);
	int diff = newTime - oldTime;
	printf("%ld\n",diff);
	oldTime=newTime;*/
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	renderer->render(scene);
    SDL_GL_SwapWindow(window);
}

void mainLoop(){
	bool quit = false;
	while(!quit){
		quit = handleEvents();
		render();
	}
}

void cleanUp(){
	delete scene;
	delete renderer;
	SDL_GL_DeleteContext(context);
    SDL_DestroyWindow( window );
    SDL_Quit();
}

int main(int argc, char** argv){
	initializeContext();
	/*int c;
	scanf("%d",&c);*/
	scene = new Scene();
<<<<<<< HEAD
=======
	mol = new Molecule("dna.pdb");
	mol->addToScene(scene);
>>>>>>> master
	Camera* camera = scene->getCamera();
	camera->setTarget(new Vec3(0,0,0));
	Geometry* geom = new Geometry();
	geom->loadDataFromFile("highres-icosphere.mesh");
	PhongMaterial* mat = new PhongMaterial();
	mat->getDiffuseColor()->setRGB(0.4,0.1,0.1);
	sphere = new Mesh(geom,mat);
	sphere->getPosition()->setX(1.0);
	sphere->getPosition()->setY(1.0);
	sphere->getPosition()->setZ(1.0);
	Mesh* esferas[NUM_SPHERES][NUM_SPHERES];
	for(int i = 0; i < NUM_SPHERES;i++){
		for(int j=0; j< NUM_SPHERES;j++){
			esferas[i][j] = new Mesh(geom,mat);
			esferas[i][j]->getPosition()->setX(3*(i- NUM_SPHERES/2));
			esferas[i][j]->getPosition()->setY(3*(NUM_SPHERES/2 - j));
			scene->addObject((Object3D*)esferas[i][j]);
		}
	}
	scene->addObject((Object3D*)sphere);
	light1 = new DirectionalLight();
	light1->getPosition()->setX(2.0);
	light1->getPosition()->setY(4.0);
	light1->getPosition()->setZ(5.0);
	light1->getColor()->setRGB(1,1,1);
	scene->addDirectionalLight(light1);

	scene->generateOctree();
	renderer = new Renderer();
	mainLoop();
	cleanUp();
    return 0;
}