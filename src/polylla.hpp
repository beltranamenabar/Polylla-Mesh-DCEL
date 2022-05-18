/* Polygon mesh generator
//POSIBLE BUG: el algoritmo no viaja por todos los halfedges dentro de un poligono, 
    //por lo que pueden haber semillas que no se borren y tener poligonos repetidos de output
*/

#ifndef POLYLLA_HPP
#define POLYLLA_HPP


#include <array>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cmath>
#include <chrono>
#include <iomanip>

#include "polygon.hpp"
#include "triangulation.hpp"

#define print_e(eddddge) eddddge<<" ( "<<tr->origin(eddddge)<<" - "<<tr->target(eddddge)<<") "



class Polylla
{
private:
    typedef std::vector<int> _polygon; 
    typedef std::vector<bool> bit_vector; 


    Triangulation *tr; // Halfedge triangulation
    std::vector<Polygon> polygonal_mesh; //Vector of polygons generated by polygon
    std::vector<int> triangles; //True if the edge generated a triangle CHANGE!!!!

    bit_vector max_edges; //True if the edge i is a max edge
    bit_vector frontier_edges; //True if the edge i is a frontier edge
    bit_vector terminal_edges; //True if the edge i is a terminal edge
    std::vector<int> seed_edges; //Seed edges that generate polygon simple and non-simple

    int m_polygons = 0; //Number of polygons
    int n_frontier_edges = 0; //Number of frontier edges
    int n_barrier_edge_tips = 0; //Number of barrier edge tips
public:

    Polylla() {}; //Default constructor

    //Constructor from a OFF file
    Polylla(std::string off_file){
        //std::cout<<"Generating Triangulization..."<<std::endl;
        auto t_start = std::chrono::high_resolution_clock::now();
        this->tr = new Triangulation(off_file);
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout<<"Triangulation generated "<<elapsed_time_ms<<" ms"<<std::endl;

        construct_Polylla();
    }

    //Constructor from a node_file, ele_file and neigh_file
    Polylla(std::string node_file, std::string ele_file, std::string neigh_file){
        //std::cout<<"Generating Triangulization..."<<std::endl;
        auto t_start = std::chrono::high_resolution_clock::now();
        this->tr = new Triangulation(node_file, ele_file, neigh_file);
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout<<"Triangulation generated "<<elapsed_time_ms<<" ms"<<std::endl;

        construct_Polylla();
    }

    ~Polylla() {
        delete tr;
    }

    void construct_Polylla(){
        max_edges = bit_vector(tr->halfEdges(), false);
        frontier_edges = bit_vector(tr->halfEdges(), false);
        terminal_edges = bit_vector(tr->halfEdges(), false);
        //seed_edges = bit_vector(tr->halfEdges(), false);
        triangles = tr->get_Triangles(); //Change by triangle list

        //Label max edges of each triangle
        //for (size_t t = 0; t < tr->faces(); t++){
        auto t_start = std::chrono::high_resolution_clock::now();
        for(auto &t : triangles)
            max_edges[label_max_edge(t)] = true;   
        auto t_end = std::chrono::high_resolution_clock::now();
        double elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout<<"Labered max edges in "<<elapsed_time_ms<<" ms"<<std::endl;

        t_start = std::chrono::high_resolution_clock::now();
        //Label frontier edges
        for (std::size_t e = 0; e < tr->halfEdges(); e++){
            if(is_frontier_edge(e)){
                frontier_edges[e] = true;
                n_frontier_edges++;
            }
        }
        t_end = std::chrono::high_resolution_clock::now();
        elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout<<"Labeled frontier edges in "<<elapsed_time_ms<<" ms"<<std::endl;
        
        t_start = std::chrono::high_resolution_clock::now();
        //label seeds edges,
        for (std::size_t e = 0; e < tr->halfEdges(); e++)
            if(tr->is_interior_face(e) && is_seed_edge(e))
                seed_edges.push_back(e);
        t_end = std::chrono::high_resolution_clock::now();
        elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout<<"Labeled seed edges in "<<elapsed_time_ms<<" ms"<<std::endl;


        //Travel phase: Generate polygon mesh
        _polygon poly;
        //Foreach seed edge generate polygon
        t_start = std::chrono::high_resolution_clock::now();
        for(auto &e : seed_edges){
            poly = travel_triangles(e);
            if(!has_BarrierEdgeTip(poly)){ //If the polygon is a simple polygon then is part of the mesh
                polygonal_mesh.push_back({e, poly});
            }else{ //Else, the polygon is send to reparation phase
                barrieredge_tip_reparation(e, poly);
            }         
        }    
        t_end = std::chrono::high_resolution_clock::now();
        elapsed_time_ms = std::chrono::duration<double, std::milli>(t_end-t_start).count();
        std::cout<<"Polygons generated/repaired in "<<elapsed_time_ms<<" ms"<<std::endl;
        
        this->m_polygons = polygonal_mesh.size();

        std::cout<<"Mesh with "<<m_polygons<<" polygons "<<n_frontier_edges/2<<" edges and "<<n_barrier_edge_tips<<" barrier-edge tips."<<std::endl;
        //tr->print_pg(std::to_string(tr->vertices()) + ".pg");             
    }

    //function whose input is a vector and print the elements of the vector
    void print_vector(std::vector<int> &vec){
        std::cout<<vec.size()<<" ";
        for (auto &v : vec){
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }

    //Print ale file of the polylla mesh
    void print_ALE(std::string filename){
        std::ofstream out(filename);
        _polygon poly;
        out<<"# domain type\nCustom\n";
        out<<"# nodal coordinates: number of nodes followed by the coordinates \n";
        out<<tr->vertices()<<std::endl;
        //print nodes
        for(std::size_t v = 0; v < tr->vertices(); v++)
            out<<std::setprecision(15)<<tr->get_PointX(v)<<" "<<tr->get_PointY(v)<<std::endl; 
        out<<"# element connectivity: number of elements followed by the elements\n";
        out<<this->m_polygons<<std::endl;
        //print polygons
        for(auto &i : this->polygonal_mesh){
            out<<i.vertices.size()<<" ";
            for(auto &v : i.vertices){
                out<<v + 1<<" ";
            }
            out<<std::endl; 
        }
        //Print borderedges
        out<<"# indices of nodes located on the Dirichlet boundary\n";
        ///Find borderedges
        int b_curr, b_init = 0;
        for(std::size_t i = tr->halfEdges()-1; i != 0; i--){
            if(tr->is_border_face(i)){
                b_init = i;
                break;
            }
        }
        out<<tr->origin(b_init) + 1<<" ";
        b_curr = tr->prev(b_init);
        while(b_init != b_curr){
            out<<tr->origin(b_curr) + 1<<" ";
            b_curr = tr->prev(b_curr);
        }
        out<<std::endl;
        out<<"# indices of nodes located on the Neumann boundary\n0\n";
        out<<"# xmin, xmax, ymin, ymax of the bounding box\n";
        double xmax = tr->get_PointX(0);
        double xmin = tr->get_PointX(0);
        double ymax = tr->get_PointY(0);
        double ymin = tr->get_PointY(0);
        //Search min and max coordinates
        for(std::size_t v = 0; v < tr->vertices(); v++){
            //search range x
            if(tr->get_PointX(v) > xmax )
                xmax = tr->get_PointX(v);
            if(tr->get_PointX(v) < xmin )
                xmin = tr->get_PointX(v);
            //search range y
            if(tr->get_PointY(v) > ymax )
                ymax = tr->get_PointY(v);
            if(tr->get_PointY(v) < ymin )
                ymin = tr->get_PointY(v);
        }
        out<<xmin<<" "<<xmax<<" "<<ymin<<" "<<ymax<<std::endl;
        out.close();
    }

    //Print off file of the polylla mesh
    void print_OFF(std::string filename){
        std::ofstream out(filename);
        _polygon poly;
        out<<"{ appearance  {+edge +face linewidth 2} LIST\n";
        out<<"OFF"<<std::endl;
        //num_vertices num_polygons 0
        out<<std::setprecision(15)<<tr->vertices()<<" "<<m_polygons<<" 0"<<std::endl;
        //print nodes
        for(std::size_t v = 0; v < tr->vertices(); v++)
            out<<tr->get_PointX(v)<<" "<<tr->get_PointY(v)<<" 0"<<std::endl; 
        //print polygons
        for(auto &i : polygonal_mesh){
            out<<i.vertices.size()<<" ";
            for(auto &v : i.vertices){
                out<<v<<" ";
            }
            out<<std::endl; 
        }
        out<<"}"<<std::endl;
        out.close();
    }

    //Print a halfedge file
    //The first line of the file is the number of halfedges
    //The rest of the lines are the halfedges with the following format:
    //origin target
    void print_hedge(std::string file_name){
        std::cout<<"Print halfedges"<<std::endl;
        std::ofstream file;
        file.open(file_name);
        int n_frontier_edges = 0;
        for(std::size_t i = 0; i < frontier_edges.size(); i++){
            if(frontier_edges[i] == true){
                n_frontier_edges++;
            }
        }
        file<<n_frontier_edges<<std::endl;
        for(std::size_t i = 0; i < tr->halfEdges(); i++){
            if(frontier_edges[i] == true){
                file<<tr->origin(i)<<" "<<tr->target(i)<<"\n";
            }
        }
        file.close();
    }

    //Return a polygon generated from a seed edge
    _polygon generate_polygon(int e)
    {   
        _polygon poly;
        //search next frontier-edge
        int e_init = search_frontier_edge(e);
        int v_init = tr->origin(e_init);
        int e_curr = tr->next(e_init);
        int v_curr = tr->origin(e_curr);
        poly.push_back(v_curr);
        while(e_curr != e_init && v_curr != v_init)
        {   
            e_curr = search_frontier_edge(e_curr);  
            //select triangle that contains v_curr as origin
            e_curr = tr->next(e_curr);
            v_curr = tr->origin(e_curr);
            poly.push_back(v_curr);
        }
        return poly;
    }


private:

    //Return true is the edge is terminal-edge or terminal border edge, 
    //but it only selects one halfedge as terminal-edge, the halfedge with lowest index is selected
    bool is_seed_edge(int e){
        int twin = tr->twin(e);

        bool is_terminal_edge = (tr->is_interior_face(twin) &&  (max_edges[e] && max_edges[twin]) );
        bool is_terminal_border_edge = (tr->is_border_face(twin) && max_edges[e]);

        if( (is_terminal_edge && e < twin ) || is_terminal_border_edge){
            return true;
        }

        return false;
    }




    //Label max edges of all triangles in the triangulation
    //input: edge e indicent to a triangle t
    //output: position of edge e in max_edges[e] is labeled as true
    int label_max_edge(const int e)
    {
        //Calculates the size of each edge of a triangle 

        double dist0 = tr->distance(e);
        double dist1 = tr->distance(tr->next(e));
        double dist2 = tr->distance(tr->next(tr->next(e)));

        short max;
        //Find the longest edge of the triangle
        if((dist0 >= dist1 && dist1 >= dist2) || (dist0 >= dist2 && dist2 >= dist1)){
            max = 0; //edge face[0]-face[1] is max
        }else if( (dist1 >= dist0 && dist0 >= dist2) || (dist1 >= dist2 && dist2 >= dist0)){
            max = 1; //edge face[1]-face[2] is max
        }else if( (dist2 >= dist1 && dist1 >= dist0) || (dist2 >= dist0 && dist0 >= dist1)){
            max = 2; //edge face[2]-face[0] is max
        }else{
            std::cout<<"ERROR: max edge not found"<<std::endl;
            exit(0);
        }
        int init_vertex = tr->origin(e);
        int curr_vertex = -1;
        int nxt = e;
        // Return the index of the edge with the longest edge
        while (curr_vertex != init_vertex){
            nxt = tr->next(nxt);
            curr_vertex = tr->origin(nxt);
            if(max == 0 && curr_vertex == tr->origin(e)){
                return nxt;
            }else if(max == 1 && curr_vertex == tr->origin(tr->next(e))){
                return nxt;
            }else if(max == 2 && curr_vertex == tr->origin(tr->next(tr->next(e)))){
                return nxt;
            }          
        }
        return -1;
    }

 
    //Return true if the edge e is the lowest edge both triangles incident to e
    //in case of border edges, they are always labeled as frontier-edge
    bool is_frontier_edge(const int e)
    {
        int twin = tr->twin(e);
        bool is_border_edge = tr->is_border_face(e) || tr->is_border_face(twin);
        bool is_not_max_edge = !(max_edges[e] || max_edges[twin]);
        if(is_border_edge || is_not_max_edge)
            return true;
        else
            return false;
    }

    //Travel in CCW order around the edges of vertex v from the edge e looking for the next frontier edge
    int search_frontier_edge(const int e)
    {
        int nxt = e;
        while(!frontier_edges[nxt])
        {
            nxt = tr->CW_edge_to_vertex(nxt);
        }  
        return nxt;
    }

    //return true if the polygon is not simple
    bool has_BarrierEdgeTip(std::vector<int> poly){
        int length_poly = poly.size();
        int x, y, i;
        for (i = 0; i < length_poly; i++)
        {
            x = i % length_poly;
            y = (i+2) % length_poly;
            if (poly[x] == poly[y])
                return true;
        }
        return false;
    }   

    //generate a polygon from a seed edge
    _polygon travel_triangles(const int e)
    {   
        _polygon poly;
        //search next frontier-edge
        int e_init = search_frontier_edge(e);
        int v_init = tr->origin(e_init);
        int e_curr = tr->next(e_init);
        int v_curr = tr->origin(e_curr);
        poly.push_back(v_curr);
        //travel inside frontier-edges of polygon
        while(e_curr != e_init && v_curr != v_init)
        {   
            e_curr = search_frontier_edge(e_curr);  
            //select edge that contains v_curr as origin
            e_curr = tr->next(e_curr);
            v_curr = tr->origin(e_curr);
            //v_curr is part of the polygon
            poly.push_back(v_curr);
        }
        return poly;
    }
    
    //Given a barrier-edge tip v, return the middle edge incident to v
    //The function first calculate the degree of v - 1 and then divide it by 2, after travel to until the middle-edge
    //input: vertex v
    //output: edge incident to v
    int search_middle_edge(const int v_bet)
    {
        //select frontier-edge of barrier-edge tip
        int frontieredge_with_bet = this->search_frontier_edge(tr->edge_of_vertex(v_bet));
        int nxt = tr->CW_edge_to_vertex(frontieredge_with_bet);
        int adv = 1; 
        //calculates the degree of v_bet
        while (nxt != frontieredge_with_bet)
        {
            nxt = tr->CW_edge_to_vertex(nxt);
            adv++;
        }
        adv--; //last edge visited is the same with the frontier-edge so it is not counted
        if(adv%2 == 0){ //if the triangles surrounding the BET are even 
            adv = adv/2 - 1;
        }else{   
            //if the triangles surrounding the BET are odd, edges are even
            //Choose any edge of the triangle in the middle; prov is choose due to this always exists
            adv = adv/2;
        }   
        //back to traversing the edges of v_bet until select the middle-edge
        nxt = tr->CW_edge_to_vertex(frontieredge_with_bet);
        //adv--;
        while (adv != 0)
        {
            nxt = tr->CW_edge_to_vertex(nxt);
            adv--;
        }
        return nxt;
    }

    //Given a seed edge e and a polygon poly generated by e, split the polygon until remove al barrier-edge tips
    //input: seed edge e, polygon poly
    //output: polygon without barrier-edge tips
    void barrieredge_tip_reparation(const int e, std::vector<int> &poly)
    {
        int x, y, i;
        int t1, t2;
        int middle_edge, v_bet;

        //list is initialize
        std::vector<int> triangle_list;
        bit_vector seed_bet_mark(this->tr->halfEdges(), false);
        //look for barrier-edge tips
        for (i = 0; i < poly.size(); i++)
        {
            x = i;
            y = (i+2) % poly.size();
            if (poly[x] == poly[y]){
                n_barrier_edge_tips++;
                n_frontier_edges+=2;
                //select edge with bet
                v_bet= poly[(i+1) % poly.size()];
                //middle edge that contains v_bet
                middle_edge = search_middle_edge(v_bet);
                t1 = middle_edge;
                t2 = tr->twin(middle_edge);
                //edges of middle-edge are labeled as frontier-edge
                this->frontier_edges[t1] = true;
                this->frontier_edges[t2] = true;
                //edges are use as seed edges and saves in a list
                triangle_list.push_back(t1);
                triangle_list.push_back(t2);

                seed_bet_mark[t1] = true;
                seed_bet_mark[t2] = true;
            }
        }
        int t_curr;
        _polygon poly_curr;
        //generate polygons from seeds,
        //two seeds can generate the same polygon
        //so the bit_vector seed_bet_mark is used to label as false the edges that are already used
        while (!triangle_list.empty())
        {
            t_curr = triangle_list.back();
            triangle_list.pop_back();
            if(seed_bet_mark[t_curr]){
                seed_bet_mark[t_curr] = false;
                poly_curr = generate_repaired_polygon(t_curr, seed_bet_mark);
                //Store the polygon in the as part of the mesh
                this->polygonal_mesh.push_back({t_curr, poly_curr});
            }
        }
    }


    //Generate a polygon from a seed-edge and remove repeated seed from seed_list
    //POSIBLE BUG: el algoritmo no viaja por todos los halfedges dentro de un poligono, 
    //por lo que pueden haber semillas que no se borren y tener poligonos repetidos de output
    _polygon generate_repaired_polygon(const int e, bit_vector &seed_list)
    {   
        _polygon poly;
        int e_init = e;
        //search next frontier-edge
        while(!frontier_edges[e_init])
        {
            e_init = tr->CW_edge_to_vertex(e_init);
            seed_list[e_init] = false; 
            //seed_list[tr->twin(e_init)] = false;
        }        
        int v_init = tr->origin(e_init);
        int e_curr = tr->next(e_init);
        int v_curr = tr->origin(e_curr);
        poly.push_back(v_curr);
        seed_list[e_curr] = false;
        //seed_list[tr->twin(e_curr)] = false;
        while(e_curr != e_init && v_curr != v_init)
        {   
            while(!frontier_edges[e_curr])
            {
                e_curr = tr->CW_edge_to_vertex(e_curr);
                seed_list[e_curr] = false;
          //      seed_list[tr->twin(e_curr)] = false;
            } 
            seed_list[e_curr] = false;
            //seed_list[tr->twin(e_curr)] = false;
            e_curr = tr->next(e_curr);
            v_curr = tr->origin(e_curr);
            poly.push_back(v_curr);
            seed_list[e_curr] = false;
            //seed_list[tr->twin(e_curr)] = false;
        }
        return poly;
    }
};

#endif