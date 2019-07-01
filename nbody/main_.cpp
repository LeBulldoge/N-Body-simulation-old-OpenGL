//F = G*((m1*m2)/dt^2)
//https://metalab.at/wiki/images/4/4f/Solar_final2.cpp.txt
//https://www.youtube.com/channel/UCL5m1_llmeiAdZMo_ZanIvg
//http://www.flipcode.com/archives/Octree_Implementation.shtml
#include <iostream>
#include <math.h>
#include <vector>
#include <array>
#include <glew.h>
#include <gl\GL.h>
#include <gl\GLU.h>
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

//N-body class
class Body {
	//id
	short id;
	//Name
	std::string name;
	//Position
	double px, py, pz;
	//Velocity
	double vx, vy, vz;
	//Acceleration
	double ax, ay, az;
	//Mass
	const double mass;
	//Gravitational prameter
	const double GM;
	//Radius
	const double rad;

public:
	//Constructor
	Body(short id, std::string name, double px, double py, double pz, double vx, double vy, double vz, double mass, double GM, double rad)
		: id(id)
		, name(name)
		, px(px*1000.0), py(py*1000.0), pz(pz*1000.0)
		, vx(vx*1000.0), vy(vy*1000.0), vz(vz*1000.0)
		, ax(0), ay(0), az(0)
		, mass(mass)
		, GM(GM)
		, rad(rad)
	{};
	~Body() {};

	//Update velocity and position based on force and time step
	void update();
	//Add force
	void addG(Body b);
	//Reset force
	void resetG();
	//Print
	void print();
	//Draw
	void draw(float rotate);
	void setCam();
	//Get name
	std::string getName()
	{
		return name;
	}
};

/*struct OctreeNode
{
    OctreeNode * Children[8];
    OctreeNode * Parent;
    Body	   * Bodies;
    glm::vec3    Center;
    glm::vec3    HalfSize;
};*/

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
void close(std::vector<Body> & bodies);

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
GLdouble cPosZ = -500.0f;

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
					//glEnable(GL_DEPTH_TEST);
					glViewport(0, 0, width, height);

					glMatrixMode(GL_PROJECTION);
					glLoadIdentity();
					gluPerspective(45.0, (float)width / (float)height, 0.05f, 12000.0f);

					glMatrixMode(GL_MODELVIEW);
					glEnable(GL_POLYGON_SMOOTH);
					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					glBlendFunc(GL_SRC_ALPHA_SATURATE, GL_ONE);
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
	vx += step * ax;
	vy += step * ay;
	vz += step * az;
	px += step * vx;
	py += step * vy;
	pz += step * vz;
}

void Body::addG(Body b)
{
	//Softener
	double EPS = 4E6;
	//Distance vectors
	double dx = b.px - px;
	double dy = b.py - py;
	double dz = b.pz - pz;
	//Normalized?
	double dist = sqrt(dx*dx + dy*dy + dz*dz + EPS*EPS);
	//Calculating force
	double F = b.GM / (dist*dist*dist);
	//Acceleration
	ax += F * dx;
	ay += F * dy;
	az += F * dz;
	//b.ax -= F * dx;
	//b.ay -= F * dy;
	//b.az -= F * dz;
}

void Body::resetG()
{
	ax = 0.0;
	ay = 0.0;
	az = 0.0;
}

void Body::print()
{
	double v = sqrt(vx*vx + vy*vy + vz*vz);
	double a = sqrt(ax*ax + ay*ay + az*az);
	ImGui::Text("%s\n px: %f AU\n py: %f AU\n pz: %f AU\n vx: %g m/s\n vy: %g m/s\n vz: %g m/s\n v: %g m/s\n ax: %f\n ay: %f\n az: %f\n accel: %f\n mass: %e kg", name.c_str(), px / AU, py / AU, pz / AU, vx, vy, vz, v, ax, ay, az, a, mass); 
}

Camera::Camera()
{
	_angleY = 0;
	_angleZ = 0;
	_distance = 0.5f;
	_motionSensivity = 0.25;
	camFollow = false;
}

void Camera::OnMouseMotion(const SDL_MouseMotionEvent & e)
{
	_angleZ += e.xrel * _motionSensivity;
	_angleY += e.yrel * _motionSensivity;
	if (_angleY > 90)
		_angleY = 90;
	else if (_angleY < -90)
		_angleY = -90;
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
	gluLookAt(_distance, 0, 0,
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
	if(tempSurface)
	{
		glGenTextures(1, &g_Texture[cnt]);
		glBindTexture(GL_TEXTURE_2D, g_Texture[cnt]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, tempSurface->w, tempSurface->h, 0, GL_RGB, GL_UNSIGNED_BYTE, tempSurface->pixels);
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
	glDeleteTextures(11, g_Texture);
}

void lighting()
{
	GLfloat lightPosition[4] = { 0, 0, 0, 1 };
	GLfloat lightDiff[4] = { 0.9f, 0.9f, 0.9f, 1.f };
	glShadeModel(GL_SMOOTH);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
	glMaterialfv(GL_FRONT, GL_DIFFUSE, lightDiff);
	glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.001f);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	//glEnable(GL_POLYGON_SMOOTH);
}

void Body::draw(float rotate)
{
	glCullFace(GL_BACK);
	glBindTexture(GL_TEXTURE_2D, g_Texture[id]);
	glEnable(GL_CULL_FACE);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();
	camera.look();
	GLUquadricObj *pObj = gluNewQuadric();
	gluQuadricDrawStyle(pObj, GLU_FILL);
	gluQuadricTexture(pObj, GLU_TRUE);
	gluQuadricNormals(pObj, GLU_SMOOTH);
	glPushMatrix();
	glTranslated(px * scale, py * scale, pz * scale);
	glScalef(0.0004f, 0.0004f, 0.0004f);
	glRotatef(rotate, 0.f, 0.f, 1.f);
	gluSphere(pObj, rad, 64, 64);
	glPopMatrix();
	glDisable(GL_TEXTURE_2D);
	glCullFace(GL_BACK);
	glDisable(GL_CULL_FACE);
	gluDeleteQuadric(pObj);
}

void drawStars()
{
	glDisable(GL_LIGHT0);
	glDisable(GL_LIGHTING);
	glCullFace(GL_FRONT);
	glEnable(GL_CULL_FACE);
	glBindTexture(GL_TEXTURE_2D, g_Texture[10]);
	glEnable(GL_TEXTURE_2D);
	glLoadIdentity();
	camera.look();
	GLUquadricObj *pObj = gluNewQuadric();
	gluQuadricTexture(pObj, true);
	glPushMatrix();
	glTranslatef(0.f, 0.f, 0.f);
	gluSphere(pObj, 8000, 12, 12);
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
			if(!camera.camFollow)
			{
				case SDLK_UP:
				case SDLK_w:
					if (cPosY > 2000) cPosY = 2000;
					else cPosY += 7.0f;
					break;
				case SDLK_DOWN:
				case SDLK_s:
					if (cPosY < -2000) cPosY = -2000;
					else cPosY -= 7.0f;
					break;
				case SDLK_LEFT:
				case SDLK_a:
					if (cPosX > 2000) cPosX = 2000;
					else cPosX += 7.0f;
					break;
				case SDLK_RIGHT:
				case SDLK_d:
					if (cPosX < -2000) cPosX = -2000;
					else cPosX -= 7.0f;
					break;
				case SDLK_q:
					if (cPosZ < -2000) cPosZ = -2000;
					else cPosZ -= 7.0f;
					break;
				case SDLK_e:
					if (cPosZ > 2000) cPosZ = 2000;
					else cPosZ += 7.0f;
					break;
			}
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
			case SDLK_0:
				currBody = 9;
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
			{
				cPosX -= 10.0f;
				cPosY -= 10.0f;
				cPosZ -= 10.0f;
			}
			else
				if (e.wheel.y < 0)
				{
					cPosX += 10.0f;
					cPosY += 10.0f;
					cPosZ += 10.0f;
				}
			break;
		}
	}
	return quit;
}

void Body::setCam()
{
	cPosX = -(px*scale);
	cPosY = -(py*scale);
	cPosZ = pz*scale - 100.f;
}

void close(std::vector<Body> & bodies)
{
	for (auto& body : bodies)
	{
		body.~Body();
	}
	bodies.~vector();
	camera.~Camera();
	glDeleteProgram(gProgramID);
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
	int i = 50;
	float arr[50];
	float rotate = 0;
	bool showWindow = true;
	std::vector<Body> bodies;
	bodies.reserve(10);

	/*name  px,  py   pz   
			vx   vy   vz   
			mass GM   radius*/
	bodies.emplace_back(Body(0, "Sun",		4.321102017786880E5,  7.332029220261886E5, -2.173368199099370E4,
										   -7.189825590990792E-3, 1.078719625403305E-2, 1.611861005709897E-4,
											1.98892e30,           1.327124400189e20,    695700 * 0.05));
	bodies.emplace_back(Body(1, "Mercury",  4.827977920456001E7, -3.593848175336351E7, -7.407821383784354E6,
											2.001998825524805E1,  4.093878958529241E1,  1.507185201945884E0,
											0.33011e24,           2.20329e13,           2439.7));
	bodies.emplace_back(Body(2, "Venus",	3.081230776389965E7, -1.037662195648798E8, -3.208271884613059E6,
											3.338454368178189E1,  9.667951253108880E0, -1.794288490452711E0,
											4.8685e24,            3.248599e14,          6051.8));
	bodies.emplace_back(Body(3, "Earth",    -5.969534792469550E7, -1.384328330091216E8, -1.539442552493513E4,
											2.686684747512509E1, -1.191462405401140E1, -1.789306645543220E-4, 
											5.97219e24,           3.9860044189e14,      6378.1));
	bodies.emplace_back(Body(4, "Moon",	    -5.974319433302711E7, -1.380760796700854E8, -3.959553641559929E4,
											2.578316278670249E1, -1.201488699520010E1,  5.838677218808286E-2,
											734.9e20,             4.90486959e12,        1737.4));
	bodies.emplace_back(Body(5, "Mars",	   -2.457432353017395E7,  2.363419494094334E8,  5.529273093965277E6,
										   -2.318343075686108E1, -4.864803362557868E-1, 5.585276593733890E-1,
											6.4185e23,            4.2828372e13,         3396.2));
	bodies.emplace_back(Body(6, "Jupiter", -7.569735834852062E8, -3.021406720368661E8,  1.818386330099623E7,
											4.691818986427708E0, -1.151682040045604E1, -5.715174720721627E-2,
											1898.19e24,           1.266865349e17,       71499));
	bodies.emplace_back(Body(7, "Saturn",  -1.644374719486625E8, -1.494399735641233E9,  3.252789329873234E7, 
											9.071782458885481E0, -1.088556624490897E0, -3.415607795777443E-1,
											568.34e24,            3.79311879e16,        60268));
	bodies.emplace_back(Body(8, "Uranus",   2.708324010627307E9,  1.246406855535664E9, -3.045762012091076E7,
									       -2.896814088567216E0,  5.868838238458174E0,  5.925130485071195E-2,
											86.8103e24,           5.7939399e15,         24973));
	bodies.emplace_back(Body(9, "Neptune",  4.260892227618985E9, -1.383625744199709E9, -6.970338029718751E7,
											1.643322631738114E0,  5.202138396781843E0, -1.453302666708880E-1,
											102.41e24,            6.8365299e15,         24342));
	camera = Camera();
	for (int i = 0; i < 10; ++i)
	{
		init_texture(bodies[i].getName(), i);
	}
	init_texture("stars", 10);
	//Main loop
	while (!handleInput())
	{
		//Draw the planets, apply lighting, draw skysphere
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		for (auto& body : bodies)
		{
			body.draw(rotate += 0.05f);
			lighting();
		}
		if (camera.camFollow)
			bodies[currBody].setCam();
		drawStars();
		ImGui_ImplSdlGL3_NewFrame(gWindow);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::SetNextWindowPos(ImVec2(0,0));
		ImGui::SetNextWindowSize(ImVec2(350,200));
		ImGui::Begin("FPS", &showWindow, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
			ImGui::Columns(2);
			ImGui::Spacing();
			bodies[currBody].print();
			ImGui::NextColumn();
			ImGui::TextWrapped("Average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
			if(i == 0) i = 49; else i--;
			arr[i] = ImGui::GetIO().Framerate;
			ImGui::PlotLines("FPS", arr, IM_ARRAYSIZE(arr));
			ImGui::SliderFloat("Step", &step, 0.f, 86400.0f*2);
		ImGui::End();
		ImGui::SetNextWindowSize(ImVec2(250,150));
		ImGui::SetNextWindowPos(ImVec2(w-250,0));
		ImGui::Begin("Help", &showWindow, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar);
			ImGui::TextWrapped("Camera movement:\n W/S - Y axis\n A/D - X axis\n Q/E - Z axis\n 0-9 - select a planet and view it's characteristics\n C - lock camera's position on the currently selected planet\n ALT - show cursor");
		ImGui::End();
		ImGui::PopStyleVar(1);
		ImGui::Render();
		SDL_GL_SwapWindow(gWindow);
		//n-body simulation
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
		//!n-body simulation
		SDL_Delay(10);
	}
	//Close the program
	close(bodies);
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