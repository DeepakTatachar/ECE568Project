/**
  ******************************************************************************
  * @file    graph.h
  * @author  Deepak Ravikumar
  * @version V1.0
  * @date    22-April-2019
  * @brief   See graph.c
  ******************************************************************************
*/

#include <stdlib.h>
#include <stdbool.h>
#include "stm32f0xx.h"
#include "stm32f0_discovery.h"


#define WHITE 0x00ffffff
#define BLACK 0x00000000
#define RED   0x00250000
#define GREEN 0x00002500
#define BLUE  0x00000025
#define CYAN  0x00002525
#define YELL  0x00252500
#define PINK  0x00250025

// sRGB format, s not used, B is the lower byte, and so on ..
typedef uint32_t COLOR;

typedef struct edge {
    struct edge* next;
    int vertexLabel;
    int weight;
} EDGE;

typedef struct vertex {
    struct vertex* next;
    EDGE* edges;
    COLOR color;
    int label;
} VERTEX;

// See graph.c for description
void addVertex(int label);

// See graph.c for description
void addEdge(int fromVertex, int toVertex, int weight);

// See graph.c for description
void colorGraph();
