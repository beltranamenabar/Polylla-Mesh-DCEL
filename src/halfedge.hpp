#pragma once

struct halfEdge {
    int origin; //tail of edge
    int target; //head of edge
    int twin; //opposite halfedge
    int next; //next halfedge of the same face
    int prev; //previous halfedge of the same face
    int face = -1; //face index incident to the halfedge
    int is_border; //1 if the halfedge is on the boundary, 0 otherwise
};