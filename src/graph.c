/**
  ******************************************************************************
  * @file    graph.c
  * @author  Deepak Ravikumar
  * @version V1.0
  * @date    22-April-2019
  * @brief   Code for graph building and coloring
  ******************************************************************************
*/

// The graph is represented as an adjacency list
#include "graph.h"

VERTEX* networkGraph = NULL;


/**
 * @param weight of the edge
 * @param label, the label of the vertex to connect to
 * @return malloc-ed edge
 */
EDGE* createEdge(int weight, int vertexLabel) {
    EDGE* newEdge = (EDGE*)malloc(sizeof(EDGE));
    newEdge->next = NULL;
    newEdge->weight = weight;
    newEdge->vertexLabel = vertexLabel;
    return newEdge;
}

/**
 * @param label: Label of the vertex to be added
 * @param edges: Pointer to the linked list of edges from this vertex
 * @return
 */
void addVertex(int label) {
    VERTEX* node = (VERTEX*)malloc(sizeof(VERTEX));
    node->edges = NULL;
    node->label = label;
    node->color = WHITE;

    // Insert at the beginning of the list
    node->next = networkGraph;

    // Update the head of the list to the inserted node
    networkGraph = node;
}

/**
 * @param fromVertex: The label of the vertex from which the edge needs to be added
 * @param toVertex: The label of the vertex to which the edge needs to be added
 * @param weight: weight of the edge to be added
 */
void addEdge(int fromVertex, int toVertex, int weight) {
    // Allocate memory for an edge
    EDGE* newEdge = createEdge(weight, toVertex);

    VERTEX* tracer = networkGraph;

    // Traverse the list until we hit the graph vertex
    while(tracer->label != fromVertex && tracer != NULL) {
        tracer = tracer->next;
    }

    if(tracer == NULL) {
        // vertex not found
        return;
    }

    EDGE* edges = tracer->edges;

    // Insert at the beginning of the list
    newEdge->next = edges;

    // Update the head of the list to the inserted node
    tracer->edges = newEdge;
}

/**
 * @return returns the number of vertices in the graph
 */
int getNumVertices() {
    VERTEX* tracer = networkGraph;

    // Number of vertices
    int n = 0;
    while(tracer!= NULL) {
        n++;
        tracer = tracer->next;
    }

    return n;
}

VERTEX* getVertex(int label) {
    VERTEX* tracer = networkGraph;

    while(tracer->label != label && tracer != NULL) {
        tracer = tracer->next;
    }

    return tracer;
}

/**
 * @return returns the lookup index for the color
 */
int lookup(int color) {
    switch(color) {
        case BLACK: return 0;
        case RED:   return 1;
        case BLUE:  return 2;
        case GREEN: return 3;
        default: return 4;
    }
}

/**
 * @return returns the lookup index for the color
 */
int revLookup(int index) {
    switch(index) {
        case 0: return BLACK;
        case 1: return RED;
        case 2: return BLUE;
        case 3: return GREEN;
        default: return RED;
    }
}

void buildDummyGraph() {
    addVertex(1);
    addVertex(2);
    addVertex(3);
    addVertex(4);
    addVertex(5);
    addEdge(1, 2, 0);
    addEdge(2, 3, 0);
    addEdge(4, 5, 0);
    addEdge(1, 5, 0);
}

/**
 * Colors the network graph
 * @return
 */
void colorGraph() {
    // A temporary array to store the available colors. False
    // value of available[cr] would mean that the color cr is
    // assigned to one of its adjacent vertices
    int n = getNumVertices();
    bool* available = (bool*)malloc(sizeof(bool) * n);
    for(int cr = 0; cr < n; cr++)
        available[cr] = true;

    // Assign colors to remaining V-1 vertices
    VERTEX* currVertex = networkGraph->next;
    while(currVertex != NULL)
    {
        // Process all adjacent vertices and flag their colors
        // as unavailable
        EDGE* edge = currVertex->edges;
        while(edge != NULL) {
            VERTEX* v = getVertex(edge->vertexLabel);
            edge = edge->next;

            if(v == NULL) {
                break;
            }

            if (v->color != WHITE)
                // Since we support a lot of colors we have a lok up table to
                // hash the color value
                available[lookup(v->color)] = false;
        }

        // Find the first available color
        int cr;
        for (cr = 0; cr < n; cr++)
            if (available[cr] == true)
                break;

        currVertex->color = revLookup(cr);
        currVertex = currVertex->next;

        // Reset the values back to false for the next iteration
        for(int cr = 0; cr < n; cr++)
            available[cr] = true;
    }
}

