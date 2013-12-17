#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>
#define GL_GLEXT_PROTOTYPES
#include <SFML/OpenGL.hpp>
#include <GL/glext.h>
#include <iostream>
#include <fstream>
#include <cmath>

#define WINDOW_WIDTH    800
#define WINDOW_HEIGHT   600

#define LOG_MAX_LEN     1023

#define FPS             60

struct vec2
{
    float x;
    float y;
};
struct vec3
{
    float x;
    float y;
    float z;
};
struct vec4
{
    float x;
    float y;
    float z;
    float w;
};


/**********/
/* GLOBAL */
/**********/

unsigned width = WINDOW_WIDTH;
unsigned height = WINDOW_HEIGHT;

/********************/
/* USEFUL FUNCTIONS */
/********************/

float norm(const vec3& v)
{
    return sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}
vec3 normalize(const vec3& v)
{
    vec3 r;
    float n = norm(v);
    r.x = v.x / n;
    r.y = v.y / n;
    r.z = v.z / n;
    return r;
}
vec3 operator-(const vec3& u, const vec3& v)
{
    vec3 w;
    w.x = u.x - v.x;
    w.y = u.y - v.y;
    w.z = u.z - v.z;
    return w;
}
vec3 cross(const vec3& u, const vec3& v)
{
    vec3 w;
    w.x = u.y * v.z - u.z * v.y;
    w.y = u.z * v.x - u.x * v.z;
    w.z = u.x * v.y - u.y * v.x;
    return w;
}

const char* sourceFromFile(const char* filename)
{
    std::ifstream file(filename, std::ios::in|std::ios::binary|std::ios::ate);
    file.seekg(0, std::ios::end);
    unsigned size = file.tellg();
    char *src = new char[size];
    file.seekg(0, std::ios::beg);
    file.read(src, size);
    file.close();
    return src;
}

void getCamera(const vec3& origin, const vec3& target, vec3& n, vec3& u, vec3& v, float& focal)
{
    n = normalize(target - origin);
    float b = n.y;
    float c = n.z;

    vec3 a;
    a.z = 0;
    if (fabs(b / c) > 1.0)
    {
        a.x = 1;
        a.y = 0;
        v = normalize(cross(a, n));
        u = normalize(cross(n, v));
    }
    else
    {
        a.x = 0;
        a.y = 1;
        u = normalize(cross(n, a));
        v = normalize(cross(u, n));
    }

    n = normalize(cross(v, u));

    // fovy = 45 degrees
    focal = fabs(width / (2.0 * 0.41421356237309503));
}

/***********/
/* PROGRAM */
/***********/

int main()
{
    // create the window
    sf::Window window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "OpenGL", sf::Style::Default, sf::ContextSettings(32));
    window.setVerticalSyncEnabled(false);

    // load resources, initialize the OpenGL states, ...

    std::cout << (const char*) glGetString(GL_VERSION) << "\n";
    std::cout << (const char*) glGetString(GL_SHADING_LANGUAGE_VERSION) << "\n";

    /*************************/
    /* SHADER INITIALISATION */
    /*************************/

    const char* vsSrc = sourceFromFile("vertex.glsl");
    const char* fsSrc = sourceFromFile("fragment.glsl");

    int status, len;
    char* log = new char[LOG_MAX_LEN + 1];

    // compiling vertex shader
    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vsSrc, NULL);
    glCompileShader(v);
    glGetShaderiv(v, GL_COMPILE_STATUS, &status);
    std::cout << "vertex shader compilation: " << status << "\n";
    if (!status)
    {
        std::cout << "===============================\n";
        glGetShaderInfoLog(v, LOG_MAX_LEN, &len, log);
        std::cout.write(log, len);
        std::cout << "===============================\n";
    }

    // compiling fragment shader
    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fsSrc, NULL);
    glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &status);
    std::cout << "fragment shader compilation: " << status << "\n";
    if (!status)
    {
        std::cout << "===============================\n";
        glGetShaderInfoLog(f, LOG_MAX_LEN, &len, log);
        std::cout.write(log, len);
        std::cout << "===============================\n";
    }

    // creating and linking shader program
    GLuint p = glCreateProgram();
    glAttachShader(p, v);
    glAttachShader(p, f);
    glLinkProgram(p);
    glGetShaderiv(f, GL_LINK_STATUS, &status);
    std::cout << "program linking: " << status << "\n";
    if (!status)
        exit(0);

    // using program from now
    glUseProgram(p);


    // settings uniform constants
    glUniform2f(glGetUniformLocation(p, "resolution"), WINDOW_WIDTH, WINDOW_HEIGHT);

    /******************/
    /* MUSIC MAESTRO! */
    /******************/

    sf::Music music;
    music.openFromFile("music.ogg");
    music.setPitch(1.0);
    // music.play();

    // tempo
    int BPM = 129;

    // and time
    sf::Clock clock;
    sf::Time lastTime = clock.getElapsedTime();

    /******************/
    /* USED VARIABLES */
    /******************/

    // camera
    vec3 cameraOrigin = {0, 100, -200};
    vec3 cameraTarget = {0, 0, 0};
    vec3 U, V, cameraNormal;
    float focal;

    GLuint normalLoc = glGetUniformLocation(p, "normal");
    GLuint originLoc = glGetUniformLocation(p, "origin");
    GLuint uLoc = glGetUniformLocation(p, "u");
    GLuint vLoc = glGetUniformLocation(p, "v");
    GLuint focalLoc = glGetUniformLocation(p, "focal");

    // objects
    unsigned    objectsNb = 0;
    vec4        spheres[100];
    vec3        colors[100];
    vec3        attributes[100];

    GLuint objectsNbLoc = glGetUniformLocation(p, "objNb");
    GLuint spheresLoc = glGetUniformLocation(p, "spheres");
    GLuint colorsLoc = glGetUniformLocation(p, "colors");
    GLuint attrLoc = glGetUniformLocation(p, "attr");

    // lights
    float       ambientLight = .5;
    unsigned    lightsNb = 0;
    vec4        lights[10];

    GLuint ambientLoc = glGetUniformLocation(p, "ambientLight");
    GLuint lightsNbLoc = glGetUniformLocation(p, "lNb");
    GLuint lightsLoc = glGetUniformLocation(p, "lights");

    /************************/
    /* SCENE INITIALISATION */
    /************************/

    // objects
    objectsNb = 4;

    spheres[0] = {-100,0,0, 60};
    colors[0] = {0,1,0};
    attributes[0] = {.5, .5, 8.0};

    spheres[1] = {100,0,0, 60};
    colors[1] = {1,0,0};
    attributes[1] = {.3, .7, 16.0};

    spheres[2] = {0,0,100, 60};
    colors[2] = {1,1,0};
    attributes[2] = {0, 1.0, 8.0};

    spheres[3] = {0,0,-100, 60};
    colors[3] = {0,0,1};
    attributes[3] = {0, 1.0, 8.0};

    // spheres[4] = {0,0,-180, 20};
    // colors[4] = {0,1,1};
    // attributes[4] = {0, 1.0, 8.0};


    // lights
    lightsNb = 1;
    lights[0] = {0,200,0, 1.0};
    // lights[0] = {0,0,-200, 1.0};
    // lights[1] = {0,0,-200, 1.0};
    // lights[2] = {-200,0,0, 1.0};
    // lights[3] = {200,0,0, 1.0};

    /*************/
    /* MAIN LOOP */
    /*************/

    // deleteme
    focal = width / 45.0;
    focal = -.1;

    unsigned tic = 0; // tempo indicator
    double t, t_; // bpm indicator (and previous)
    int u = (60000 / (BPM * 2)); // bpm factor
    int T = 0; // real time

    bool firstTime = true;
    bool running = true;

    while (running)
    {
        sf::Time time = clock.getElapsedTime();

        if ((time - lastTime).asMilliseconds() < (1000 / FPS))
            continue;

        // double u = (time - lastTime).asMilliseconds();
        // if (u > 0)
        //     std::cout << 1000 / u << "\n";

        lastTime = time;

        // handle events
        // sf::Event event;
        // while (window.pollEvent(event))
        // {
        //     if (event.type == sf::Event::Closed)
        //     {
        //         // end the program
        //         running = false;
        //     }
        //     else if (event.type == sf::Event::Resized)
        //     {
        //         // adjust the viewport when the window is resized
        //         glViewport(0, 0, event.size.width, event.size.height);
        //         glUniform2f(glGetUniformLocation(p, "resolution"),
        //                     event.size.width, event.size.height);
        //         width = event.size.width;
        //         height = event.size.height;
        //     }
        // }

        // clear the buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        bool updateCamera = true;
        bool updateScene = false;
        bool updateLights = true;

        T = time.asMilliseconds();
        t = 2 * (double(T % u) / u) - 1.0;
        if (t_ > 0 && t < 0)
            ++tic;
        t_ = t;

        cameraOrigin.z -= 10;

        lights[0].x = cameraOrigin.x;
        lights[0].y = cameraOrigin.y;
        lights[0].z = cameraOrigin.z;

        if (updateCamera || firstTime)
        {
            getCamera(cameraOrigin, cameraTarget, cameraNormal, U, V, focal);
            glUniform3f(originLoc, cameraOrigin.x, cameraOrigin.y, cameraOrigin.z);
            glUniform3f(normalLoc, cameraNormal.x, cameraNormal.y, cameraNormal.z);
            glUniform3f(uLoc, U.x, U.y, U.z);
            glUniform3f(vLoc, V.x, V.y, V.z);
            glUniform1f(focalLoc, focal);
        }

        if (updateScene || firstTime)
        {
            glUniform1i(objectsNbLoc, objectsNb);
            glUniform4fv(spheresLoc, objectsNb, (float*)spheres);
            glUniform3fv(colorsLoc, objectsNb, (float*)colors);
            glUniform3fv(attrLoc, objectsNb, (float*)attributes);
        }

        if (updateLights || firstTime)
        {
            glUniform1f(ambientLoc, ambientLight);
            glUniform1i(lightsNbLoc, lightsNb);
            glUniform4fv(lightsLoc, lightsNb, (float*)lights);
        }

        glBegin(GL_TRIANGLE_STRIP);
            glVertex3f(-1, 1, 1);
            glVertex3f(1, 1, 1);
            glVertex3f(-1, -1, 1);
            glVertex3f(1, -1, 1);
        glEnd();


        // end the current frame (internally swaps the front and back buffers)
        window.display();

        if (firstTime)
            firstTime = false;
    }

    // release resources...

    return 0;
}
