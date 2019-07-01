//F = G*((m1*m2)/dt^2)
//https://metalab.at/wiki/images/4/4f/Solar_final2.cpp.txt
//https://www.youtube.com/channel/UCL5m1_llmeiAdZMo_ZanIvg
//http://www.flipcode.com/archives/Octree_Implementation.shtml
#include <iostream>
#include <math.h>
#include <array>
#include <glew.h>
#include <gl\GL.h>
#include <gl\GLU.h>
#include <glm\glm.hpp>
#include <glm\trigonometric.hpp>
#include <glm\vec3.hpp>
#include <SDL_opengl.h>
#include <SDL.h>
#include <SDL_main.h>
#include <SDL_image.h>
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_impl_sdl_gl3.h"

//Gravity constant
const double G = 6.673e-11;
//Time step (should be 1 day)
float step = 86400.0f;
//Astronomical unit in meters
const double AU = 1.49597893e11;
//Scale
const float scale = 0.0000000005f;
GLuint g_Texture[12];
/*
struct vect
{
	double x, y, z;
	vect()
	{
		x = NULL;
		y = NULL;
		z = NULL;
	};
	vect(double x, double y, double z)
		: x(x), y(y), z(z)
	{};
	vect operator*(const double num)
	{
		x *= num;
		y *= num;
		z *= num;
		return *this;
	}
};
*/
//N-body class
class Body {
	//id
	const short id;
	//Name
	std::string const name;
	std::string const ruName;
	//Position
	glm::vec3 p;
	//Velocity
	glm::vec3 v;
	//Acceleration
	glm::vec3 a;
	//Mass
	const double mass;
	//Gravitational prameter
	const double GM;
	//Radius
	const double rad;
	//Axial tilt
	const GLfloat tilt;
	//Orbital period
	const unsigned int days;
	glm::vec3* orbit;

public:
	//Constructor
	Body(const short id,
		std::string const & name, std::string const & ruName,
		glm::vec3 p,
		glm::vec3 v,
		const double mass,
		const double GM,
		const double rad,
		const GLfloat tilt,
		const int days)
		: id(id)
		, name(name), ruName(ruName)
		, p(p*1000.f)
		, v(v*1000.f)
		, a(0,0,0)
		, mass(mass)
		, GM(GM)
		, rad(rad)
		, tilt(tilt)
		, days(days)
	{
		if (days > 0) {
			orbit = new glm::vec3[days];
			for (int i = 0; i < days; i++) {
				orbit[i] = glm::vec3(NULL);
			}
		}
	};

	~Body()
	{
	};

	//Update velocity and position based on force and time step
	void update();
	//Add force
	void addG(Body& b);
	//Reset force
	void resetG();
	//Print
	void print();
	//Draw
	void draw(float rotate);
	void setCam();
	void drawOrbit(int day, bool show, bool orbit);
	//Get name
	std::string getName()
	{
		return name;
	}
};

class Camera
{

public:

	Camera();
	virtual void OnMouseMotion(const SDL_MouseMotionEvent & e);
	virtual void OnKeyboard();
	virtual void look();
	virtual ~Camera();
	virtual double getAngleY()
	{
		return _angleY;
	}
	virtual double getAngleZ()
	{
		return _angleY;
	}
	bool camFollow;
protected:

	double _motionSensivity;
	double _distance;
	double _angleY;
	double _angleZ;

};

//Starts up SDL and creates window
bool init();

//Frees media and shuts down SDL
void close();

//Initialize OpenGL
bool initGL();

//The window we'll be rendering to
SDL_Window* gWindow = NULL;

SDL_GLContext gContext;

GLuint gProgramID = 0;
GLint gVertexPos2DLocation = -1;
Camera camera;
GLdouble cPosX = 0.0f;
GLdouble cPosY = 0.0f;
GLdouble cPosZ = 0.0f;
GLdouble zoom = -200.0f;

bool init()
{
	//Initialization flag
	bool success = true;

	//Initialize SDL
	if (SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "SDL couldn't initialize!", SDL_GetError(), gWindow);
		success = false;
	}
	else
	{
		IMG_Init(IMG_INIT_JPG);
		//Set texture filtering to linear
		if (!SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1"))
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warning!", "Linear texture filtering not enabled!", gWindow);
		}

		//Create window
		gWindow = SDL_CreateWindow("N-Body", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_MAXIMIZED);
		SDL_SetWindowGrab(gWindow, SDL_TRUE);
		SDL_SetRelativeMouseMode(SDL_TRUE);
		//SDL_ShowCursor(1);
		if (gWindow == NULL)
		{
			SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Window couldn't be created!", SDL_GetError(), gWindow);
			success = false;
		}
		else
		{
			{
				//Create context
				gContext = SDL_GL_CreateContext(gWindow);
				if (gContext == NULL)
				{
					printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
					success = false;
				}
				else
				{
					//Initialize GLEW
					glewExperimental = GL_TRUE;
					GLenum glewError = glewInit();
					if (glewError != GLEW_OK)
					{
						success = false;
						printf("Error initializing GLEW! %s\n", glewGetErrorString(glewError));
					}

					//Use Vsync
					if (SDL_GL_SetSwapInterval(1) < 0)
					{
						success = false;
						printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
					}
					//Initialize OpenGL
					if (!initGL())
					{
						printf("Unable to initialize OpenGL!\n");
						success = false;
					}
				}
			}
		}

	}
	return success;
}

bool initGL()
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 5);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	//Success flag
	bool success = true;

	//Generate program
	gProgramID = glCreateProgram();

	//Create vertex shader
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);

	//Get vertex source
	const GLchar* vertexShaderSource[] =
	{
		"#version 140\nin vec3 LVertexPos2D; void main() { gl_Position = vec4( LVertexPos2D.x, LVertexPos2D.y, 0, 1 ); }"
	};

	//Set vertex source
	glShaderSource(vertexShader, 1, vertexShaderSource, NULL);

	//Compile vertex source
	glCompileShader(vertexShader);

	//Check vertex shader for errors
	GLint vShaderCompiled = GL_FALSE;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &vShaderCompiled);
	if (vShaderCompiled != GL_TRUE)
	{
		printf("Unable to compile vertex shader %d!\n", vertexShader);
		success = false;
	}
	else
	{
		//Attach vertex shader to program
		glAttachShader(gProgramID, vertexShader);

		//Create fragment shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

		//Get fragment source
		const GLchar* fragmentShaderSource[] =
		{
			"#version 140\nout vec4 LFragment; void main() { LFragment = vec4( 1.0, 1.0, 1.0, 1.0 ); }"
		};

		//Set fragment source
		glShaderSource(fragmentShader, 1, fragmentShaderSource, NULL);

		//Compile fragment source
		glCompileShader(fragmentShader);

		//Check fragment shader for errors
		GLint fShaderCompiled = GL_FALSE;
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &fShaderCompiled);
		if (fShaderCompiled != GL_TRUE)
		{
			printf("Unable to compile fragment shader %d!\n", fragmentShader);
			success = false;
		}
		else
		{
			//Attach fragment shader to program
			glAttachShader(gProgramID, fragmentShader);

			//Link program
			glLinkProgram(gProgramID);

			//Check for errors
			GLint programSuccess = GL_TRUE;
			glGetProgramiv(gProgramID, GL_LINK_STATUS, &programSuccess);
			if (programSuccess != GL_TRUE)
			{
				printf("Error linking program %d!\n", gProgramID);
				success = false;
			}
			else
			{
				//Get vertex attribute location
				gVertexPos2DLocation = glGetAttribLocation(gProgramID, "LVertexPos2D");
				if (gVertexPos2DLocation == -1)
				{
					printf("LVertexPos2D is not a valid glsl program variable!\n");
					success = false;
				}
				else
				{
					//Initialize clear color
					glClearColor(0.f, 0.f, 0.f, 0.f);
					int width, height;
					SDL_GetWindowSize(gWindow, &width, &height);
					glViewport(0, 0, width, height);

					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					gluPerspective(45.0, (float)width / (float)height, 10.f, 10000.0f);
					glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
					glEnable(GL_DEPTH_TEST);
					glMatrixMode(GL_MODELVIEW);
					glEnable(GL_POLYGON_SMOOTH);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glHint(GL_POLYGON_SMOOTH_HINT, GL_DONT_CARE);
					ImGui_ImplSdlGL3_Init(gWindow);
				}
			}
		}
	}

	return success;
}

void Body::update()
{
	//Velocity
	v += a * step;
	//Position
	p += v * step;
}

void Body::addG(Body& b)
{
	//Distance vectors
	glm::vec3 d;
	d.x	= b.p.x - p.x;
	d.y = b.p.y - p.y;
	d.z = b.p.z - p.z;
	//Normalized
	float dist = glm::distance(p, b.p);
	//Calculating force
	float F = b.GM / (dist*dist*dist);
	//Acceleration
	a += d * F;
}

void Body::resetG()
{
	a.x = 0.0;
	a.y = 0.0;
	a.z = 0.0;
}

void Body::print()
{
	ImGui::Text("%s\n px: %e m\n py: %e m\n pz: %e m\n vx: %g m/s\n vy: %g m/s\n vz: %g m/s\n Скорость: %g m/s\n ax: %f\n ay: %f\n az: %f\n Ускорение: %f\n Масса: %e kg", ruName.c_str(), p.x, p.y, p.z, v.x, v.y, v.z, glm::length(v), a.x, a.y, a.z, glm::length(a), mass);
}

Camera::Camera()
{
	_angleY = 100;
	_angleZ = 0;
	_distance = 0.5f;
	_motionSensivity = 0.25;
	camFollow = false;
}

void Camera::OnMouseMotion(const SDL_MouseMotionEvent & e)
{
	_angleZ += e.xrel * _motionSensivity;
	_angleY += e.yrel * _motionSensivity;
	if (_angleY > 180)
		_angleY = 180;
	else if (_angleY < 0)
		_angleY = 0;
}

void Camera::OnKeyboard()
{
	_angleY = 0;
	_angleZ = 0;
	cPosX = 0.0f;
	cPosY = 0.0f;
	cPosZ = 0.0f;
	camFollow = false;
}

void Camera::look()
{
	gluLookAt(_distance, 0, zoom,
		0, 0, 0,
		0, 0, 1);
	glRotated(_angleY, 0, 1, 0);
	glRotated(_angleZ, 0, 0, 1);
	glTranslated(cPosX, cPosY, cPosZ);
}

Camera::~Camera()
{
}

void init_texture(std::string name, int cnt)
{
	name.append(".jpg");
	name.insert(0, "res/texture_");
	SDL_Surface* tempSurface = IMG_Load(name.c_str());
	if (tempSurface)
	{
		glGenTextures(1, &g_Texture[cnt]);
		glBindTexture(GL_TEXTURE_2D, g_Texture[cnt]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tempSurface->w, tempSurface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, tempSurface->pixels);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		SDL_FreeSurface(tempSurface);
	}
	else
	{
		SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Warning!", IMG_GetError(), gWindow);
	}
}

void delete_texture()
{
	glDeleteTextures(12, g_Texture);
}

void lighting()
{
	GLfloat lightPosition[4] = { 0, 0, 0, 1 };
	GLfloat lightDiff[4] = { 0.9f, 0.9f, 0.9f, 1.f };
	glShadeModel(GL_SMOOTH);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, lightDiff);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.0005f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
}

void Body::draw(float rotate)
{
	GLfloat sunLight[4] = { 1,1,1,1 };
	GLfloat otherLight[4] = { 0,0,0,0 };
	glCullFace(GL_BACK);
	if (id < 10)
		glBindTexture(GL_TEXTURE_2D, g_Texture[id]);
	else
		glBindTexture(GL_TEXTURE_2D, g_Texture[10]);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();
	camera.look();
	GLUquadricObj *pObj = gluNewQuadric();
	gluQuadricDrawStyle(pObj, GLU_FILL);
	gluQuadricTexture(pObj, GLU_TRUE);
	gluQuadricNormals(pObj, GLU_SMOOTH);
	if (id == 0) glMaterialfv(GL_FRONT, GL_EMISSION, sunLight); else { glMaterialfv(GL_FRONT, GL_EMISSION, otherLight); }
	glPushMatrix();
	glTranslatef(p.x * scale, p.y * scale, p.z * scale);
	glScalef(0.0004f, 0.0004f, 0.0004f);
	glRotatef(tilt, 0.f, 1.f, 0.f);
	glRotatef(rotate, 0.f, 0.f, 1.f);
	gluSphere(pObj, rad, 64, 64);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	gluDeleteQuadric(pObj);
}

void Body::drawOrbit(int day, bool show, bool asteroid)
{
	if (days > 0)
	{
		if (day > days)
		{
			orbit[day % days] = p*scale;
		}
		else
		{
			orbit[day-1] = p*scale;
		}
		if (!asteroid)
		{
			if (id > 9)
				return;
		}
		if (!show)
		{
			if (id < 9)
				return;
		}
		glDisable(GL_LIGHT0);
		glDisable(GL_LIGHTING);
		if (day > days)
			glBegin(GL_LINE_LOOP);
		else
			glBegin(GL_POINTS);
		for (int i = 0; i < days; i++)
		{
			glVertex3fv(&orbit[i].x);
		}
		glEnd();
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
	}
}

void Body::setCam()
{
	cPosX = -(p.x*scale);
	cPosY = -(p.y*scale);
	cPosZ = -(p.z*scale);
}

void drawStars()
{
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, g_Texture[11]);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();
	camera.look();
	GLUquadricObj *pObj = gluNewQuadric();
	gluQuadricTexture(pObj, true);
	glPushMatrix();
	glTranslatef(0.f, 0.f, 0.f);
	gluSphere(pObj, 6000, 12, 12);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_CULL_FACE);
	glCullFace(GL_FRONT);
	gluDeleteQuadric(pObj);
}

int currBody = 3;
bool handleInput()
{
	bool quit = false;
	SDL_Event e;
	ImGuiIO& io = ImGui::GetIO();
	while (SDL_PollEvent(&e) != 0)
	{
		ImGui_ImplSdlGL3_ProcessEvent(&e);
		switch (e.type)
		{
		case SDL_QUIT:
			quit = true;
			break;
		case SDL_KEYDOWN:
			switch (e.key.keysym.sym)
			{
			case SDLK_ESCAPE:
				quit = true;
				break;
			case SDLK_RETURN:
				break;
			case SDLK_SPACE:
				camera.OnKeyboard();
				break;
			case SDLK_1:
				currBody = 0;
				break;
			case SDLK_2:
				currBody = 1;
				break;
			case SDLK_3:
				currBody = 2;
				break;
			case SDLK_4:
				currBody = 3;
				break;
			case SDLK_5:
				currBody = 4;
				break;
			case SDLK_6:
				currBody = 5;
				break;
			case SDLK_7:
				currBody = 6;
				break;
			case SDLK_8:
				currBody = 7;
				break;
			case SDLK_9:
				currBody = 8;
				break;
			case SDLK_c:
				if (camera.camFollow) camera.camFollow = false;
				else camera.camFollow = true;
				break;
			}
			break;
		case SDL_MOUSEMOTION:
			if (!io.KeyAlt) camera.OnMouseMotion(e.motion);
			break;
		case SDL_MOUSEWHEEL:
			if (e.wheel.y > 0)
				zoom += 7.f;
			else if (e.wheel.y < 0)
				zoom -= 7.f;
			if (zoom > -50)
				zoom = -50;
			else if (zoom < -500)
				zoom = -500;
			break;
		}
	}
	return quit;
}

void close()
{
	camera.~Camera();
	//glDeleteProgram(gProgramID);
	//Destroy window	
	ImGui_ImplSdlGL3_Shutdown();
	SDL_DestroyWindow(gWindow);
	SDL_GL_DeleteContext(gContext);
	gContext = NULL;
	gWindow = NULL;
	delete_texture();
	//Quit SDL subsystems
	IMG_Quit();
	SDL_Quit();
}

void mainLoop()
{
	int w, h;
	SDL_GetWindowSize(gWindow, &w, &h);
	float rotate = 0.f;
	float day = 0.f;
	bool showWindow = true;
	bool loopPause = true;
	bool showOrbits = true;
	bool showAsteroidOrbits = false;
	std::array <Body, 14> bodies =
 { {
		Body(0, "Sun", "Солнце", 
			glm::vec3(4.321102017786880E5,  7.332029220261886E5, -2.173368199099370E4),
			glm::vec3(-7.189825590990792E-3, 1.078719625403305E-2, 1.611861005709897E-4),
			1.98892e30, 1.327124400189e20, 695700 * 0.07, 0.0, 0),
		Body(1, "Mercury", "Меркурий", 
			glm::vec3(4.827977920456001E7, -3.593848175336351E7, -7.407821383784354E6),
			glm::vec3(2.001998825524805E1,  4.093878958529241E1,  1.507185201945884E0),
			0.33011e24, 2.20329e13, 2439.7, 0.1f, 88),
		Body(2, "Venus", "Венера", 
			glm::vec3(3.081230776389965E7, -1.037662195648798E8, -3.208271884613059E6),
			glm::vec3(3.338454368178189E1,  9.667951253108880E0, -1.794288490452711E0),
			4.8685e24, 3.248599e14, 6051.8, 177.36f, 224),
		Body(3, "Earth", "Земля", 
			glm::vec3(-5.969534792469550E7, -1.384328330091216E8, -1.539442552493513E4),
			glm::vec3(2.686684747512509E1, -1.191462405401140E1, -1.789306645543220E-4),
			5.97219e24, 3.9860044189e14, 6378.1, 23.45f, 365),
		Body(4, "Mars", "Марс", 
			glm::vec3(-2.457432353017395E7,  2.363419494094334E8,  5.529273093965277E6),
			glm::vec3(-2.318343075686108E1, -4.864803362557868E-1, 5.585276593733890E-1),
			6.4185e23, 4.2828372e13, 3396.2, 25.19f, 687),
		Body(5, "Jupiter", "Юпитер", 
			glm::vec3(-7.569735834852062E8, -3.021406720368661E8,  1.818386330099623E7),
			glm::vec3(4.691818986427708E0, -1.151682040045604E1, -5.715174720721627E-2),
			1898.19e24,           1.266865349e17,       71499, 3.13f, 4332),
		Body(6, "Saturn", "Сатурн",  
			glm::vec3(-1.644374719486625E8, -1.494399735641233E9,  3.252789329873234E7),
			glm::vec3(9.071782458885481E0, -1.088556624490897E0, -3.415607795777443E-1),
			568.34e24,            3.79311879e16,        60268, 26.73f, 10759),
		Body(7, "Uranus", "Уран",   
			glm::vec3(2.708324010627307E9,  1.246406855535664E9, -3.045762012091076E7),
			glm::vec3(-2.896814088567216E0,  5.868838238458174E0,  5.925130485071195E-2),
			86.8103e24,           5.7939399e15,         24973, 97.77f, 30685),
		Body(8, "Neptune", "Нептун",  
			glm::vec3(4.260892227618985E9, -1.383625744199709E9, -6.970338029718751E7),
			glm::vec3(1.643322631738114E0,  5.202138396781843E0, -1.453302666708880E-1),
			102.41e24,            6.8365299e15,         24342, 28.32f, 60189),
		Body(9, "Pluto", "Плутон",  
			glm::vec3(1.512995164271581E9, -4.750856430852462E9, 7.072265548052478E7),
			glm::vec3(5.291385672638518E0, 5.025608122645461E-1, -1.605961044129924E0),
			0.01303e24,            8.719e11,         2187, 122.53f, 90560),
		Body(10, "Ceres", "Церера", 
			glm::vec3(1.171404652999866E8, 3.898786439553194E8, -9.285654429283202E6),
			glm::vec3(-1.749907309328118E1, 3.939248912132300E0, 3.348447503365496E0),
			939300e15,            0.626284e11,         469.7 * 15, 4, 1679),
		Body(11, "Pallas", "Паллада",  
			glm::vec3(4.291592316654775E8, 4.130737867650583E7, -6.396862972703221E7),
			glm::vec3(-6.437734248047895E0, 1.322396929216813E1, -8.591155928319642E0),
			205000e15,            0.143e11,         272.5 * 15, 84, 1683),
		Body(12, "Juno", "Юнона",  
			glm::vec3(3.857026210081980E7, -4.565575631972048E+08, 1.022702769186957E8),
			glm::vec3(1.466155363544588E1, 3.856604107629098E0, -1.468972870833428E0),
			20000e15,            20000e15*G,         123.298 * 15, 50, 1591),
		Body(13, "Vesta", "Веста",  
			glm::vec3(-3.096651822378528E8, 1.773993080224814E8, 3.240229550527219E7),
			glm::vec3(-7.810311925207990E0, -1.736557473811398E1, 1.470044682760689E0),
			259000e15,            0.178e11,         262.7 * 15, 29, 1325)
	} };
	/*name ruName px,  py   pz
	vx   vy   vz
	mass GM   radius day*/
	camera = Camera();
	for (int i = 0; i < 11; ++i)
	{
		init_texture(bodies[i].getName(), i);
	}
	init_texture("stars", 11);
	//Main loop
	while (!handleInput())
	{
		//Clear buffer, draw the planets, apply lighting, draw skysphere
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		lighting();
		if(!loopPause)
			rotate += 5.f;
		for (auto& body : bodies)
		{
			body.draw(rotate);
			body.drawOrbit(day, showOrbits, showAsteroidOrbits);
		}
		if (camera.camFollow)
			bodies[currBody].setCam();

		drawStars();
		//UI
		ImGui_ImplSdlGL3_NewFrame(gWindow);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		//First frame
		ImGui::SetNextWindowPos(ImVec2(0, (float)h - 250));
		ImGui::SetNextWindowSize(ImVec2(300, 250));
		ImGui::Begin("select", &showWindow, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
		ImGui::Columns(3);
		ImGui::TextWrapped("Солнце и планеты");
		ImGui::RadioButton("Солнце", &currBody, 0);
		ImGui::RadioButton("Меркурий", &currBody, 1);
		ImGui::RadioButton("Венера", &currBody, 2);
		ImGui::RadioButton("Земля", &currBody, 3);
		ImGui::RadioButton("Марс", &currBody, 4);
		ImGui::RadioButton("Юпитер", &currBody, 5);
		ImGui::RadioButton("Сатурн", &currBody, 6);
		ImGui::RadioButton("Уран", &currBody, 7);
		ImGui::RadioButton("Нептун", &currBody, 8);
		ImGui::NextColumn();
		ImGui::TextWrapped("Карликовые планеты");
		ImGui::RadioButton("Церера", &currBody, 10);
		ImGui::RadioButton("Плутон", &currBody, 9);
		ImGui::NextColumn();
		ImGui::Text("Астероиды");
		ImGui::RadioButton("Паллада", &currBody, 11);
		ImGui::RadioButton("Юнона", &currBody, 12);
		ImGui::RadioButton("Веста", &currBody, 13);
		ImGui::End();
		//!First frame
		//Second frame
		ImGui::SetNextWindowSize(ImVec2(300, 250));
		ImGui::SetNextWindowPos(ImVec2(0, 0));
		ImGui::Begin("help", &showWindow, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
		ImGui::TextWrapped(" 1-9 - выбор тела и просмотр его характеристик. Так же возможно использование панели выбора\n C - закрепить камеру на выбранном теле\n ALT - показать курсор\n Space - сбросить камеру\n ESC - выход\n Пуск/Пауза - запуск и приостановка симуляции");
		ImGui::Separator();
		ImGui::SliderFloat("Секунд в шаг", &step, 1.f, 86400.0f * 2);
		ImGui::Text("84000 сек = 1 день");
		if (ImGui::Button("Сбросить шаг")) { step = 86400.0f; }
		ImGui::SameLine();
		if (loopPause)
		{
			if (ImGui::Button("Пуск")) loopPause = false;
		}
		else
		{
			if (ImGui::Button("Пауза")) loopPause = true;
		}
		ImGui::Text("Кол-во дней с запуска: %g", day);
		ImGui::Text("Орбиты");
		ImGui::Checkbox("Планеты", &showOrbits);
		ImGui::SameLine();
		ImGui::Checkbox("Астероиды", &showAsteroidOrbits);
		ImGui::End();
		//!Second frame
		//Third frame
		ImGui::SetNextWindowPos(ImVec2((float)w - 200, (float)h - 200));
		ImGui::SetNextWindowSize(ImVec2(200, 200));
		ImGui::Begin("stats", &showWindow, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
		bodies[currBody].print();
		ImGui::End();
		//!Third frame
		ImGui::PopStyleVar(1);
		//Render UI
		ImGui::Render();
		//Swap buffers
		SDL_GL_SwapWindow(gWindow);
		//n-body simulation
		if (!loopPause) {
			for (auto& body : bodies)
			{
				body.resetG();
				for (auto& other : bodies)
				{
					if (body.getName().compare(other.getName()))
					{
						body.addG(other);
					}
				}
			}
			for (auto& body : bodies)
			{
				body.update();
			}
			day += step / 86400.0f;
		}
		//!n-body simulation
		SDL_Delay(10);
	}
	for (auto & body : bodies)
	{
		body.~Body();
	}
	//Close the program
	close();
	//!Main loop
}

int main(int argc, char* args[])
{
	//Init SDL and OpenGL
	if (init())
	{
		//Start the main loop
		mainLoop();
	}
	return 0;
}