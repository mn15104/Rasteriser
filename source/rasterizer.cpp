#include <stdio.h>
#include <omp.h>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <fstream>
#include <vector>
#include <iostream>
#include <cassert>
#include <chrono>

#include <glm/gtx/norm.hpp>
#include <glm/gtx/normal.hpp>
#include <glm/glm.hpp>
#include <SDL2/SDL.h>

#include "scene/SDLauxiliary.h"
#include "scene/scene.h"
#include "light/ray.h"
#include "scene/camera.h"
#include "scene/keyboard.h"
#include "scene/kd-tree.h"
#include "shapes/cuboid.h"
#include "shapes/triangle.h"
#include "shapes/sphere.h"
#include "light/pointlight.h"
#include "light/spotlight.h"

using namespace std;

using glm::vec3;
using glm::mat3;
using glm::vec4;
using glm::mat4;
using glm::ivec2;
using glm::vec2;

//#define SCREEN_WIDTH 600
//#define SCREEN_HEIGHT 600
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 256
#define FULLSCREEN_MODE false
#define INDIRECT_LIGHT  vec3(0.3,0.2,0.18)
#define ANG 0.1
#define NUM_THREADS 16

bool LCTRL = false;

/* ----------------------------------------------------------------------------*/
/* FUNCTIONS                                                                   */
void Update(screen* screen, SDL_Event& event, Camera& camera, LightSource* lightSource, Keyboard& keyboard, vector<Shape2D*>& shapes, int& runProgram);
void Draw(screen* screen, const Camera& camera, LightSource* lightSource, const vector<Shape2D*>& shapes);
void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result);
void DrawLineSDL(screen* screen, ivec2 a, ivec2 b, vec3 color);
void ComputePolygonRows( const vector<ivec2>& vertexPixels, vector<ivec2>& leftPixels, vector<ivec2>& rightPixels );
void DrawRows(screen *screen, const vector<ivec2>& leftPixels, const vector<ivec2>& rightPixels, const vec3 color);
void DrawPolygon( screen *screen, vector<vec4>& verticies, const vec3 color);

void VertexShader( const vec4& v, ivec2& p) {
    vec4 camPos(0, 0, -2.3, 1);
    const float focal_length = SCREEN_WIDTH / 2;

    vec4 v_prime = v - camPos;

    p.x = focal_length * (v_prime.x / v_prime.z)+ (SCREEN_WIDTH / 2);
    p.y = focal_length * (v_prime.y / v_prime.z) + (SCREEN_HEIGHT / 2);
    //fprintf(stderr, "%f\n", v.x);
    //fprintf(stderr, "%f\n", v.y);
    //fprintf(stderr, "%f\n", v.z);
    //fprintf(stderr, "%d\n", p.x);
    //fprintf(stderr, "%d\n", p.y);

    //vec4 P_prime = v - camPos;
}

void Draw(screen* screen, const Camera& camera, LightSource* lightSource, const vector<Shape2D*>& shapes) {
    memset(screen->buffer, 0, screen->height*screen->width*sizeof(uint32_t));

    //omp_set_num_threads(NUM_THREADS);

    for ( uint32_t i = 0; i < shapes.size(); i++) {
        vector<vec4> verticies = shapes[i]->verticies();

        //verticies[0] = shapes[i]->v0;
        //verticies[1] = shapes[i]->v1;
        //verticies[2] = shapes[i]->v2;

        for ( int v = 0; v  < verticies.size(); v++) {
            //ivec2 projA;
            //ivec2 projB;
            //VertexShader(verticies[v], projA);
            //VertexShader(verticies[(v + 1) % verticies.size()], projB);
            //DrawLineSDL(screen, projA, projB, vec3(1, 1, 1));
            //fprintf(stderr, "HERE\n");
            //fprintf(stderr, ">>%d\n", shapes[i]->color.x);
            DrawPolygon(screen, verticies, shapes[i]->color);
        }

        //for (int v = 0; v < 3; ++v) {
        //    ivec2 projPos;
        //    VertexShader(verticies[v], projPos);
        //    vec3 color(1, 1, 1);
        //    PutPixelSDL(screen, projPos.x, projPos.y, color);
        //}
    }

    //#pragma omp parallel for
    //}
}

void DrawPolygon( screen *screen, vector<vec4>& vertices, const vec3 color) {
    int V = vertices.size();
    vector<ivec2> vertexPixels( V );
    for( int i=0; i<V; ++i )
    VertexShader( vertices[i], vertexPixels[i] );
    vector<ivec2> leftPixels;
    vector<ivec2> rightPixels;
    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
    DrawRows( screen, leftPixels, rightPixels, color );
}

void DrawLineSDL(screen* screen, ivec2 a, ivec2 b, vec3 color) {
    ivec2 delta = abs(a - b);
    int pixels = max(delta.x, delta.y) + 1;

    vector<ivec2> line(pixels);
    Interpolate(a, b, line);

    for (int i = 0; i < line.size(); i++) {
        PutPixelSDL(screen, line[i].x, line[i].y, color);
    }
}

void ComputePolygonRows( const vector<ivec2>& vertexPixels, vector<ivec2>& leftPixels, vector<ivec2>& rightPixels ) {
    int minY = numeric_limits<int>::max();
    int maxY = -numeric_limits<int>::max();

    for (int i = 0; i < vertexPixels.size(); i++) {
        if (minY > vertexPixels[i].y) {
            minY = vertexPixels[i].y;
        }
        if (maxY < vertexPixels[i].y) {
            maxY = vertexPixels[i].y;
        }
    }

    int rows = maxY - minY + 1;
    leftPixels = vector<ivec2>(rows);
    rightPixels = vector<ivec2>(rows);

    for (int i = 0; i < rows; i++) {
        leftPixels[i].x = numeric_limits<int>::max();
        rightPixels[i].x = -numeric_limits<int>::max();
    }

    for (int i = 0; i < vertexPixels.size(); i++) {
        vector<ivec2> line = vector<ivec2>(rows);
        Interpolate(vertexPixels[i], vertexPixels[(i+1) % vertexPixels.size()], line);

        for (int j = 0; j < rows; j++) {
            int y = line[j].y - minY;
            if (leftPixels[y].x > line[j].x) {
                leftPixels[y] = line[j];
            }
            if (rightPixels[y].x < line[j].x) {
                rightPixels[y] = line[j];
            }

        }
    }
}

void DrawRows(screen *screen, const vector<ivec2>& leftPixels, const vector<ivec2>& rightPixels, const vec3 color) {
    for (int i = 0; i < leftPixels.size(); i++) {
        for (int x = leftPixels[i].x; x < rightPixels[i].x; x++) {
            for (int y = leftPixels[i].y; y < leftPixels[i].y; y++) {
                PutPixelSDL(screen, x, y, color);
            }
        }
    }
}

void Update(screen* screen, SDL_Event& event, Camera& camera, LightSource* lightSource, Keyboard& keyboard, vector<Shape2D*>& shapes, int& runProgram){
    switch(event.type ){
        case SDL_KEYDOWN:
            keyboard.ProcessKeyDown(event.key, lightSource, camera, runProgram);
            if(runProgram == 1){
                auto started = std::chrono::high_resolution_clock::now();
                Draw(screen, camera, lightSource, shapes);
                auto done = std::chrono::high_resolution_clock::now();
                cout << "Render time: ";
                cout << chrono::duration_cast<chrono::milliseconds>(done-started).count();
                cout << " ms \n";
                SDL_Renderframe(screen);
            }
            break;
        case SDL_KEYUP:
            keyboard.ProcessKeyUp(event.key);
            break;
        default:
            break;
    }
}

void Interpolate( ivec2 a, ivec2 b, vector<ivec2>& result) {
    int N = result.size();
    vec2 step = vec2(b - a) / float(max(N - 1, 1));
    vec2 current(a);
    for (int i = 0; i < N; ++i) {
        result[i] = current;
        current += step;
    }
}

int main( int argc, char* argv[] ) {
    screen *screen = InitializeSDL( SCREEN_WIDTH, SCREEN_HEIGHT, FULLSCREEN_MODE );

    LightSource* spotLight = new SpotLight();
    Camera camera(vec4(0, 0, -2.25, 1), SCREEN_WIDTH/2);
    Keyboard keyboard;
    vector<Shape2D*> shapes;
    LoadTestModel(shapes);

    vector<ivec2> vertexPixels(3);
    vertexPixels[0] = ivec2(10, 5);
    vertexPixels[1] = ivec2( 5,10);
    vertexPixels[2] = ivec2(15,15);
    vector<ivec2> leftPixels;
    vector<ivec2> rightPixels;
    ComputePolygonRows( vertexPixels, leftPixels, rightPixels );
    for( int row=0; row<leftPixels.size(); ++row ) {
    cout << "Start: ("
    << leftPixels[row].x << ","
    << leftPixels[row].y << "). "
    << "End: ("
    << rightPixels[row].x << ","
    << rightPixels[row].y << "). " << endl;
    }

    exit(1);


    SDL_Event event;
    int runProgram = 0;

    //KDNode tree = *KDNode().buildTree(shapes, 0);

    auto started = std::chrono::high_resolution_clock::now();

    Draw(screen, camera, spotLight, shapes);

    auto done = std::chrono::high_resolution_clock::now();
    cout << "Render time: ";
    cout << chrono::duration_cast<chrono::milliseconds>(done-started).count();
    cout << " ms \n";

    SDL_Renderframe(screen);

    while(runProgram != -1){
        while( SDL_PollEvent( &event ) ){
            Update(screen, event, camera, spotLight, keyboard, shapes, runProgram);
        }
    }
    SDL_SaveImage( screen, "screenshot.bmp" );

    KillSDL(screen);
}