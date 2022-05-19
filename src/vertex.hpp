#pragma once

struct vertex {
    double x;
    double y;
    bool   is_border = false; // if the vertex is on the boundary
    int    incident_halfedge; // halfedge incident to the vertex, vertex is the origin of the halfedge
};