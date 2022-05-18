#pragma once

#include <vector>

struct Polygon{
    int seed_edge; //Edge that generate the polygon
    std::vector<int> vertices; //Vertices of the polygon
    //std::vector<int> neighbors; //Neighbors of the polygon WIP
};