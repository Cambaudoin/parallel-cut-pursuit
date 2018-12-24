/*=============================================================================
 * Base class for cut-pursuit algorithm
 * 
 * index_t must be able to represent the numbers of vertices and of edges in 
 * the main graph;
 * comp_t must be able to represent the numbers of constant connected 
 * components and of reduced edges in the reduced graph
 *
 * L. Landrieu, L. and G. Obozinski, Cut Pursuit: Fast Algorithms to Learn 
 * Piecewise Constant Functions on General Weighted Graphs, SIAM Journal on 
 * Imaging Sciences, 2017, 10, 1724-1766
 *
 * Hugo Raguet 2018
 *============================================================================*/
#pragma once
#include <cstdlib>
#include <chrono>
#include <limits>
#include <iostream>
#include "cp_graph.hpp" /* Boykov-Kolmogorov graph class modified for CP */

/* flag an activated edge on residual capacity of its corresponding arcs */
#define ACTIVE_EDGE ((real_t) -1.0) 

/* index_t must be able to represent the numbers of vertices and of
 * (undirected) edges in the main graph; comp_t must be able to represent the
 * numbers of constant connected components in the reduced graph */
template <typename real_t, typename index_t, typename comp_t>
class Cp
{
public:
    /**  constructor, destructor  **/

    /* only creates flow graph structure */
    Cp(index_t V, size_t D, index_t E, const index_t* first_edge, 
        const index_t* adj_vertices);

    /* delegation for monodimensional setting */
    Cp(index_t V, index_t E, const index_t* first_edge, 
        const index_t* adj_vertices) : Cp(V, 1, E, first_edge, adj_vertices){};

    /* the destructor does not free pointers which are supposed to be provided 
     * by the user (forward-star graph structure given at construction, 
     * monitoring arrays, etc.); IT DOES FREE THE REST (components assignment 
     * and reduced problem elements, etc.), but this can be prevented by
     * getting the corresponding pointer member and setting it to null */
	virtual ~Cp();

    /**  manipulate private members pointers and values  **/

    void reset_active_edges(); // flag all edges as not active

    /* if 'edge_weights' is null, homogeneously equal to 'homo_edge_weight' */
    void set_edge_weights(const real_t* edge_weights = nullptr,
        real_t homo_edge_weight = 1.0);

    void set_monitoring_arrays(real_t* objective_values = nullptr,
        double* elapsed_time = nullptr, real_t* iterate_evolution = nullptr);

    /* if rV is zero or unity, comp_assign will be automatically initialized;
     * if rV is zero, arbitrary components will be assigned at initialization,
     * in an attempt to optimize parallelization along components;
     * if rV is greater than one, comp_assign must be given and initialized;
     * comp_assign is free()'d by destructor, unless set to null beforehand */
    void set_components(comp_t rV = 0, comp_t* comp_assign = nullptr);

    void set_cp_param(real_t dif_tol, int it_max, int verbose, real_t eps);
    /* overload for default eps parameter */
    void set_cp_param(real_t dif_tol = 0.0, int it_max = 10, int verbose = 1000)
    {
        set_cp_param(dif_tol, it_max, verbose,
            std::numeric_limits<real_t>::epsilon());
    }

    /* the 'get' methods takes pointers to pointers as arguments; a null means
     * that the user is not interested by the corresponding pointer; NOTA:
     * 1) if not explicitely set by the user, memory pointed by these members
     * is allocated using malloc(), and thus should be deleted with free()
     * 2) they are free()'d by destructor, unless set to null beforehand */
    comp_t get_components(comp_t** comp_assign = nullptr,
        index_t** first_vertex = nullptr, index_t** comp_list = nullptr);

    real_t* get_reduced_values();

    size_t get_reduced_graph(comp_t** reduced_edges = nullptr,
        real_t** reduced_edge_weights = nullptr);

    /**  solve the main problem  **/

    int cut_pursuit(bool init = true);

protected:

    /**  main graph  **/

    const index_t V, E; // number of vertices, of edges
    const size_t D; // dimension of the data; total size is V*D
    /* forward-star representation:
     * - edges are numeroted so that all vertices originating from a same 
     * vertex are consecutive;
     * - for each vertex, 'first_edge' indicates the first edge starting
     * from the vertex (or, if there are none, starting from the next vertex);
     * array of length V+1, the last value is the total number of edges
     * - for each edge, 'adj_vertices' indicates its ending vertex */
    const index_t *first_edge, *adj_vertices; 
    const real_t *edge_weights;
    real_t homo_edge_weight;

    /**  reduced graph  **/

    comp_t rV; // number of components (reduced vertices)
    size_t rE; // number of reduced edges
    comp_t *comp_assign; // assignment of each vertex to a component
    real_t *rX, *last_rX; // reduced iterate (values of the components)
    /* list the vertices of each components:
     * - vertices are gathered in 'comp_list' so that all vertices belonging
     * to a same components are consecutive
     * - for each component, 'first_vertex' indicates the index of its first
     * vertex in 'comp_list' */
    index_t *comp_list, *first_vertex;
    comp_t *reduced_edges; // array with pair of vertices
    real_t *reduced_edge_weights;

    /** parameters **/

    real_t dif_tol, eps; // eps gives a characteristic precision 
    int it_max, verbose;

    /* for stopping criterion or component saturation in convex problems */
    bool monitor_evolution;

    /**  methods for manipulating nodes and arcs in the flow graph  **/

    bool is_active(index_t e); // check if edge e is active

    bool is_sink(index_t v); // check if vertex v is in the sink after min cut

    /* temporary components list and assignment in the flow graph graph */
    void set_tmp_comp_assign(index_t v, comp_t rv);

    comp_t get_tmp_comp_assign(index_t v);

    void set_tmp_comp_list(index_t i, index_t v);

    index_t get_tmp_comp_list(index_t v);

    /* NOTA: saturation is flagged on the first vertex of the component, so
     * this must be reset if the component list is modified or reordered */
    void set_saturation(comp_t rv, bool saturation);

    bool is_saturated(comp_t rv);

    /* manipulate flow graph residual capacities */
    void set_edge_capacities(index_t e, real_t cap_uv, real_t cap_vu);

    void set_term_capacities(index_t v, real_t cap);

    void add_term_capacities(index_t v, real_t cap);

    /* flag an active/inactive edges */
    void set_active(index_t e);

    void set_inactive(index_t e);

    /**  methods for cut-pursuit steps  **/

    /* get a parallel copy of the flow graph */
    Cp_graph<real_t, index_t, comp_t>* get_parallel_flow_graph();

    /* solve the problem over an isolated component */
    virtual void solve_univertex_problem(real_t* uX, comp_t rv = 0) = 0;

    /* allocate and compute reduced values */
    virtual void solve_reduced_problem() = 0;

    /* split components with graph cuts, by activating edges */
    virtual index_t split() = 0; 

    /* merge neighboring components when deemed useful */
    virtual index_t merge() = 0;

    /* compute relative iterate evolution; by default, in Euclidean norm;
     * for convex problems, components saturation should be checked here;
     * monitor_evolution must be set to true for making use of last_rX */
    virtual real_t compute_evolution(const bool compute_dif,
        comp_t & saturation);

    /* compute objective functional, often on the reduced problem objects */
    virtual real_t compute_objective() = 0;

    /* allocate memory and fail with error message if not successful */
    void* malloc_check(size_t size){
        void *ptr = malloc(size);
        if (!ptr){
            std::cerr << "Cut-pursuit: not enough memory." << std::endl;
            exit(EXIT_FAILURE);
        }
        return ptr;
    }
    void* realloc_check(void* ptr, size_t size){
        ptr = realloc(ptr, size);
        if (!ptr){
            std::cerr << "Cut-pursuit: not enough memory." << std::endl;
            exit(EXIT_FAILURE);
        }
        return ptr;
    }

private:

    using arc = typename Cp_graph<real_t, index_t, comp_t>::arc;

    Cp_graph<real_t, index_t, comp_t>* G; // flow graph

    /**  monitoring  **/

    real_t *objective_values;
    double *elapsed_time;
    real_t *iterate_evolution;

    real_t monitor_time(std::chrono::steady_clock::time_point start);

    void print_progress(int it, real_t dif, comp_t saturation, double t);

    /* set components assignment and values (and allocate them if needed);
     * assumes that no edge of the graph are active when it is called */
    void initialize();

    /* initialize with only one component and reduced graph accordingly */
    void single_connected_component();

    /* initialize approximately rV arbitrary connected components */
    void arbitrary_connected_components();

    /* initialize with components specified in 'comp_assign' */
    void assign_connected_components();

    /* update connected components and count saturated ones */
    comp_t compute_connected_components();

    /* allocate and compute reduced graph structure */
    void compute_reduced_graph();
};

/***  inline methods  ***/
template <typename real_t, typename index_t, typename comp_t> 
inline bool Cp<real_t, index_t, comp_t>::is_active(index_t e)
{ return G->arcs[(size_t) 2*e].r_cap == ACTIVE_EDGE; }

template <typename real_t, typename index_t, typename comp_t> 
inline bool Cp<real_t, index_t, comp_t>::is_sink(index_t v)
{ return !G->nodes[v].parent || G->nodes[v].is_sink; }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_tmp_comp_assign(index_t v,
    comp_t rv)
{ G->nodes[v].comp = rv; }

template <typename real_t, typename index_t, typename comp_t> 
inline comp_t Cp<real_t, index_t, comp_t>::get_tmp_comp_assign(index_t v)
{ return G->nodes[v].comp; }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_tmp_comp_list(index_t i, index_t v)
{ G->nodes[i].vertex = v; }

template <typename real_t, typename index_t, typename comp_t> 
inline index_t Cp<real_t, index_t, comp_t>::get_tmp_comp_list(index_t v)
{ return G->nodes[v].vertex; }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_edge_capacities(index_t e,
    real_t cap_uv, real_t cap_vu)
{
    size_t a = (size_t) 2*e; // cast as size_t to avoid overflow
    G->arcs[a].r_cap = cap_uv;
    G->arcs[a + 1].r_cap = cap_vu;
}

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_active(index_t e)
{ set_edge_capacities(e, ACTIVE_EDGE, ACTIVE_EDGE); }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_inactive(index_t e)
{ set_edge_capacities(e, (real_t) 0.0, (real_t) 0.0); }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_term_capacities(index_t v,
    real_t cap)
{ G->nodes[v].tr_cap = cap; }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::add_term_capacities(index_t v,
    real_t cap)
{ G->nodes[v].tr_cap += cap; }

template <typename real_t, typename index_t, typename comp_t> 
inline Cp_graph<real_t, index_t, comp_t>*
    Cp<real_t, index_t, comp_t>::get_parallel_flow_graph()
{ return new Cp_graph<real_t, index_t, comp_t>(*G); }

template <typename real_t, typename index_t, typename comp_t> 
inline void Cp<real_t, index_t, comp_t>::set_saturation(comp_t rv,
    bool saturation)
{ G->nodes[comp_list[first_vertex[rv]]].saturation = saturation; }

template <typename real_t, typename index_t, typename comp_t> 
inline bool Cp<real_t, index_t, comp_t>::is_saturated(comp_t rv)
{ return G->nodes[comp_list[first_vertex[rv]]].saturation; }