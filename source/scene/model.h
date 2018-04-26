#ifndef MODEL_H
#define MODEL_H
// #include "../declarations.h"
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../shapes/triangle.h"
#include "../materials/texture.h"
#include <omp.h>

using glm::ivec3;
using glm::ivec4;

vector<vec4> normals;

static vec4 parse_vec3(FILE *file) {
    vec4 normal;
    fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z);
    normal.w = 1.0f;

    return normal;
}

static vector<ivec3> parse_index(FILE *file) {
    vec4 normal;
    vector<ivec3> index;
    char mystring [40] = {0};
    if ( fgets (mystring , 40, file) != NULL ) {
        int nums = 0;
        for(int i = 0; i < 40; i++) {
            if (mystring[i] == '/') {
                nums++;
            }
        }

        // Kinda hacky
        if (nums == 0) {

            ivec3 i;
            sscanf(mystring, "%d %d %d", &i.x, &i.y, &i.z);
            index.push_back(i);

        } else if (nums == 3) {

            ivec3 i;
            sscanf(mystring, "%d/%d %d/%d %d/%d", &i.x, &normal.x, &i.y, &normal.y, &i.z, &normal.z);
            index.push_back(i);
            normals.push_back(normal);

        } else if (nums == 4) {

            ivec4 i;
            sscanf(mystring, "%d/%d %d/%d %d/%d %d/%d", &i.x, &normal.x, &i.y, &normal.y, &i.z, &normal.z, &normal.w, &i.w);
            index.push_back(ivec3(i.x, i.y, i.z));
            index.push_back(ivec3(i.x, i.w, i.y));
            normals.push_back(normal);
        }

    }


    return index;
}

static vec3 parse_texture(FILE *file) {
    vec3 tex;
    fscanf(file, "%f %f %f\n", &tex.x, &tex.y, &tex.z);
    return tex;
}


static vec4 scale(vec4 x, vec4 min, vec4 max, vec4 a, vec4 b) {
    vec4 result;
    result.x = ((b.x-a.x)*(x.x-min.x) / (max.x-min.x)) + a.x;
    result.y = ((b.y-a.y)*(x.y-min.y) / (max.y-min.y)) + a.y;
    result.z = ((b.z-a.z)*(x.z-min.z) / (max.z-min.z)) + a.z;
    result.w = 1;
    return result;
}

static vec3 pixel(vec3 pos, Texture* tex) {
    float x = pos.x * tex->width;
    float y = pos.y * tex->height;
    return tex->get_pixel(x, y) / vec3(255, 255, 255);
}

static std::vector<Shape2D*> uploadModel(std::string obj_file_path, std::string texture_path, vec4 min, vec4 max){
    vector<Shape2D*> triangles;
    vector<vec4> vertices;
    vector<ivec3> indexes;
    vector<vec3> textures;
    FILE *file = fopen(obj_file_path.c_str(), "r");
    if (file == NULL) {
        std::cout << "Failed to open file " << obj_file_path << std::endl;
        exit(1);
    }


    vec4 minV = vec4(10000, 10000, 10000, 1);
    vec4 maxV = vec4(-10000, -10000, -10000, 1);

    const size_t line_size = 512;
    char line[line_size];

    Texture* tex = new Texture(texture_path.c_str());

    while (true) {

        bool stop = false;

        int read_result = fscanf(file, "%s", line);
        if (read_result == EOF) {
            break;
        }

        if (strcmp(line, "v") == 0) {
            vec4 vert = parse_vec3(file);
            vert.x = -vert.x;
            vert.z = -vert.z;
            if (vert.x < minV.x) {
                minV.x = vert.x;
            }
            if (vert.x > maxV.x) {
                maxV.x = vert.x;
            }
            if (vert.y < minV.y) {
                minV.y = vert.y;
            }
            if (vert.y > maxV.y) {
                maxV.y = vert.y;
            }
            if (vert.z < minV.z) {
                minV.z = vert.z;
            }
            if (vert.z > maxV.z) {
                maxV.z = vert.z;
            }

            vertices.push_back(vert);
        }

        if (strcmp(line, "f") == 0) {
            vector<ivec3> index = parse_index(file);
            for (int i = 0; i < index.size(); i++) {
                indexes.push_back(index[i]);
            }
        }

        if (strcmp(line, "vt") == 0) {
            textures.push_back(parse_texture(file));
        }

    }

    fclose(file);

    Triangle** tris = (Triangle**)malloc(sizeof(Triangle) * indexes.size()/3);

    for (int i=0; i<indexes.size(); i++) {

        vec3 pix = vec3(0.6, 0.6, 0.8);
        triangles.push_back(new Triangle(
                    scale(vertices[indexes[i].x-1], minV, maxV, min, max),
                    scale(vertices[indexes[i].y-1], minV, maxV, min, max),
                    scale(vertices[indexes[i].z-1], minV, maxV, min, max),
                    pix, nullptr));

    }

    return triangles;
}
#endif
