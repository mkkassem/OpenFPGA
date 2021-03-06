/************************************************************************
 * Member Functions of RRGraph
 * include mutators, accessors and utility functions 
 ***********************************************************************/
#include <cmath>
#include <algorithm>
#include <map>
#include <limits>

#include "vtr_geometry.h"
#include "vtr_vector_map.h"
#include "vtr_log.h"
#include "vtr_util.h"
#include "vtr_assert.h"
#include "rr_graph_obj.h"
#include "rr_graph_obj_utils.h"

/********************************************************************
 * Constructors
 *******************************************************************/
RRGraph::RRGraph()
    : num_nodes_(0)
    , num_edges_(0) {
    //Pass
}

/********************************************************************
 * Accessors
 *******************************************************************/
RRGraph::lazy_node_range RRGraph::nodes() const {
    return vtr::make_range(lazy_node_iterator(RRNodeId(0), invalid_node_ids_),
                           lazy_node_iterator(RRNodeId(num_nodes_), invalid_node_ids_));
}

RRGraph::lazy_edge_range RRGraph::edges() const {
    return vtr::make_range(lazy_edge_iterator(RREdgeId(0), invalid_edge_ids_),
                           lazy_edge_iterator(RREdgeId(num_edges_), invalid_edge_ids_));
}

RRGraph::switch_range RRGraph::switches() const {
    return vtr::make_range(switch_ids_.begin(), switch_ids_.end());
}

RRGraph::segment_range RRGraph::segments() const {
    return vtr::make_range(segment_ids_.begin(), segment_ids_.end());
}

//Node attributes
t_rr_type RRGraph::node_type(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_types_[node];
}

size_t RRGraph::node_index(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return size_t(node);
}

short RRGraph::node_xlow(const RRNodeId& node) const {
    return node_bounding_box(node).xmin();
}

short RRGraph::node_ylow(const RRNodeId& node) const {
    return node_bounding_box(node).ymin();
}

short RRGraph::node_xhigh(const RRNodeId& node) const {
    return node_bounding_box(node).xmax();
}

short RRGraph::node_yhigh(const RRNodeId& node) const {
    return node_bounding_box(node).ymax();
}

short RRGraph::node_length(const RRNodeId& node) const {
    return std::max(node_xhigh(node) - node_xlow(node), node_yhigh(node) - node_ylow(node));
}

vtr::Rect<short> RRGraph::node_bounding_box(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_bounding_boxes_[node];
}

/* Node starting and ending points */
/************************************************************************
 * Get the coordinator of a starting point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xlow, ylow) should be the starting point 
 *
 * For routing tracks in DEC_DIRECTION
 * (xhigh, yhigh) should be the starting point 
 *
 * For routing tracks in BI_DIRECTION
 * we always use (xhigh, yhigh)
 ***********************************************************************/
vtr::Point<short> RRGraph::node_start_coordinate(const RRNodeId& node) const {
    /* Make sure we have CHANX or CHANY */
    VTR_ASSERT((CHANX == node_type(node)) || (CHANY == node_type(node)));

    vtr::Point<short> start_coordinate(node_xlow(node), node_ylow(node));

    if (DEC_DIRECTION == node_direction(node)) {
        start_coordinate.set(node_xhigh(node), node_yhigh(node));
    }

    return start_coordinate;
}

/************************************************************************
 * Get the coordinator of a end point of a routing track 
 * For routing tracks in INC_DIRECTION
 * (xhigh, yhigh) should be the ending point 
 *
 * For routing tracks in DEC_DIRECTION
 * (xlow, ylow) should be the ending point 
 *
 * For routing tracks in BI_DIRECTION
 * we always use (xhigh, yhigh)
 ***********************************************************************/
vtr::Point<short> RRGraph::node_end_coordinate(const RRNodeId& node) const {
    /* Make sure we have CHANX or CHANY */
    VTR_ASSERT((CHANX == node_type(node)) || (CHANY == node_type(node)));

    vtr::Point<short> end_coordinate(node_xhigh(node), node_yhigh(node));

    if (DEC_DIRECTION == node_direction(node)) {
        end_coordinate.set(node_xlow(node), node_ylow(node));
    }

    return end_coordinate;
}

short RRGraph::node_fan_in(const RRNodeId& node) const {
    return node_num_in_edges_[node];
}

short RRGraph::node_fan_out(const RRNodeId& node) const {
    return node_num_out_edges_[node];
}

short RRGraph::node_capacity(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_capacities_[node];
}

short RRGraph::node_ptc_num(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_ptc_nums_[node][0];
}

short RRGraph::node_pin_num(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN,
                   "Pin number valid only for IPIN/OPIN RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_track_num(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY,
                   "Track number valid only for CHANX/CHANY RR nodes");
    return node_ptc_num(node);
}

short RRGraph::node_class_num(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");
    return node_ptc_num(node);
}

std::vector<short> RRGraph::node_track_ids(const RRNodeId& node) const {
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY,
                   "Track number valid only for CHANX/CHANY RR nodes");
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_ptc_nums_[node];
}

short RRGraph::node_cost_index(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_cost_indices_[node];
}

e_direction RRGraph::node_direction(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direction valid only for CHANX/CHANY RR nodes");
    return node_directions_[node];
}

e_side RRGraph::node_side(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Side valid only for IPIN/OPIN RR nodes");
    return node_sides_[node];
}

/* Get the resistance of a node */
float RRGraph::node_R(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_Rs_[node];
}

/* Get the capacitance of a node */
float RRGraph::node_C(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_Cs_[node];
}

short RRGraph::node_rc_data_index(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    return node_rc_data_indices_[node];
}

/*
 * Get a segment id of a node in rr_graph 
 */
RRSegmentId RRGraph::node_segment(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return node_segments_[node];
}

RRGraph::edge_range RRGraph::node_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range(node_edges_[node].get(),
                           node_edges_[node].get() + node_num_in_edges_[node] + node_num_out_edges_[node]);
}

RRGraph::edge_range RRGraph::node_in_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range(node_edges_[node].get(),
                           node_edges_[node].get() + node_num_in_edges_[node]);
}

RRGraph::edge_range RRGraph::node_out_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range((node_edges_[node].get() + node_num_in_edges_[node]),
                           (node_edges_[node].get() + node_num_in_edges_[node]) + node_num_out_edges_[node]);
}

/* Get the list of configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_configurable_in_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range(node_edges_[node].get(),
                           node_edges_[node].get() + node_num_in_edges_[node] - node_num_non_configurable_in_edges_[node]);
}

/* Get the list of non configurable edges from the input edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_non_configurable_in_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range(node_edges_[node].get() + node_num_in_edges_[node] - node_num_non_configurable_in_edges_[node],
                           node_edges_[node].get() + node_num_in_edges_[node]);
}

/* Get the list of configurable edges from the output edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_configurable_out_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range((node_edges_[node].get() + node_num_in_edges_[node]),
                           (node_edges_[node].get() + node_num_in_edges_[node]) + node_num_out_edges_[node] - node_num_non_configurable_out_edges_[node]);
}

/* Get the list of non configurable edges from the output edges of a given node 
 * And return the range(iterators) of the list 
 */
RRGraph::edge_range RRGraph::node_non_configurable_out_edges(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));

    return vtr::make_range((node_edges_[node].get() + node_num_in_edges_[node]) + node_num_out_edges_[node] - node_num_non_configurable_out_edges_[node],
                           (node_edges_[node].get() + node_num_in_edges_[node]) + node_num_out_edges_[node]);
}

//Edge attributes
size_t RRGraph::edge_index(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return size_t(edge);
}

RRNodeId RRGraph::edge_src_node(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return edge_src_nodes_[edge];
}
RRNodeId RRGraph::edge_sink_node(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return edge_sink_nodes_[edge];
}

RRSwitchId RRGraph::edge_switch(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return edge_switches_[edge];
}

/* Check if the edge is a configurable edge (programmble) */
bool RRGraph::edge_is_configurable(const RREdgeId& edge) const {
    /* Make sure we have a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));

    auto iswitch = edge_switch(edge);

    return switches_[iswitch].configurable();
}

/* Check if the edge is a non-configurable edge (hardwired) */
bool RRGraph::edge_is_non_configurable(const RREdgeId& edge) const {
    /* Make sure we have a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return !edge_is_configurable(edge);
}

size_t RRGraph::switch_index(const RRSwitchId& switch_id) const {
    VTR_ASSERT_SAFE(valid_switch_id(switch_id));
    return size_t(switch_id);
}

/*
 * Get a switch from the rr_switch list with a given id  
 */
const t_rr_switch_inf& RRGraph::get_switch(const RRSwitchId& switch_id) const {
    VTR_ASSERT_SAFE(valid_switch_id(switch_id));
    return switches_[switch_id];
}

size_t RRGraph::segment_index(const RRSegmentId& segment_id) const {
    VTR_ASSERT_SAFE(valid_segment_id(segment_id));
    return size_t(segment_id);
}

/*
 * Get a segment from the segment list with a given id  
 */
const t_segment_inf& RRGraph::get_segment(const RRSegmentId& segment_id) const {
    VTR_ASSERT_SAFE(valid_segment_id(segment_id));
    return segments_[segment_id];
}

/************************************************************************
 * Find all the edges interconnecting two nodes
 * Return a vector of the edge ids
 ***********************************************************************/
std::vector<RREdgeId> RRGraph::find_edges(const RRNodeId& src_node, const RRNodeId& sink_node) const {
    std::vector<RREdgeId> matching_edges;

    /* Iterate over the outgoing edges of the source node */
    for (auto edge : node_out_edges(src_node)) {
        if (edge_sink_node(edge) == sink_node) {
            /* Update edge list to return */
            matching_edges.push_back(edge);
        }
    }

    return matching_edges;
}

RRNodeId RRGraph::find_node(const short& x, const short& y, const t_rr_type& type, const int& ptc, const e_side& side) const {
    initialize_fast_node_lookup();

    size_t itype = type;
    size_t iside = side;

    /* Check if x, y, type and ptc, side is valid */
    if ((x < 0)                                          /* See if x is smaller than the index of first element */
        || (size_t(x) > node_lookup_.dim_size(0) - 1) /* See if x is large than the index of last element */
        || (0 == node_lookup_.dim_size(0))) { /* See if x is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    if ((y < 0)                                          /* See if y is smaller than the index of first element */
        || (size_t(y) > node_lookup_.dim_size(1) - 1)  /* See if y is large than the index of last element */
        || (0 == node_lookup_.dim_size(1))) { /* See if y is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    /* itype is always larger than -1, we can skip checking */
    if ( (itype > node_lookup_.dim_size(2) - 1)  /* See if type is large than the index of last element */
      || (0 == node_lookup_.dim_size(2))) { /* See if type is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    if ((ptc < 0)                                                 /* See if ptc is smaller than the index of first element */
        || (size_t(ptc) > node_lookup_[x][y][type].size() - 1) /* See if ptc is large than the index of last element */
        || (0 == node_lookup_[x][y][type].size())) { /* See if ptc is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    /* Check if x, y, type and ptc, side is valid */
    /* iside is always larger than -1, we can skip checking */
    if ((iside > node_lookup_[x][y][type][ptc].size() - 1) /* See if side is large than the index of last element */
       || (0 == node_lookup_[x][y][type][ptc].size()) ) { /* See if side is large than the index of last element */
        /* Return a zero range! */
        return RRNodeId::INVALID();
    }

    return node_lookup_[x][y][itype][ptc][iside];
}

/* Find the channel width (number of tracks) of a channel [x][y] */
short RRGraph::chan_num_tracks(const short& x, const short& y, const t_rr_type& type) const {
    /* Must be CHANX or CHANY */
    VTR_ASSERT_MSG(CHANX == type || CHANY == type,
                   "Required node_type to be CHANX or CHANY!");
    initialize_fast_node_lookup();

    /* Check if x, y, type and ptc is valid */
    if ((x < 0)                                          /* See if x is smaller than the index of first element */
        || (size_t(x) > node_lookup_.dim_size(0) - 1)) { /* See if x is large than the index of last element */
        /* Return a zero range! */
        return 0;
    }

    /* Check if x, y, type and ptc is valid */
    if ((y < 0)                                          /* See if y is smaller than the index of first element */
        || (size_t(y) > node_lookup_.dim_size(1) - 1)) { /* See if y is large than the index of last element */
        /* Return a zero range! */
        return 0;
    }

    /* Check if x, y, type and ptc is valid */
    if ((size_t(type) > node_lookup_.dim_size(2) - 1)) { /* See if type is large than the index of last element */
        /* Return a zero range! */
        return 0;
    }

    const auto& matching_nodes = node_lookup_[x][y][type];

    return vtr::make_range(matching_nodes.begin(), matching_nodes.end()).size();
}

/* This function aims to print basic information about a node */
void RRGraph::print_node(const RRNodeId& node) const {
    VTR_LOG("Node id: %d\n", node_index(node));
    VTR_LOG("Node type: %s\n", rr_node_typename[node_type(node)]);
    VTR_LOG("Node xlow: %d\n", node_xlow(node));
    VTR_LOG("Node ylow: %d\n", node_ylow(node));
    VTR_LOG("Node xhigh: %d\n", node_xhigh(node));
    VTR_LOG("Node yhigh: %d\n", node_yhigh(node));
    VTR_LOG("Node ptc: %d\n", node_ptc_num(node));
    VTR_LOG("Node num in_edges: %d\n", node_in_edges(node).size());
    VTR_LOG("Node num out_edges: %d\n", node_out_edges(node).size());
}

/* Check if the segment id of a node is in range */
bool RRGraph::validate_node_segment(const RRNodeId& node) const {
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* Only CHANX and CHANY requires a valid segment id */
    if ((CHANX == node_type(node))
        || (CHANY == node_type(node))) {
        return valid_segment_id(node_segments_[node]);
    } else {
        return true;
    }
}

/* Check if the segment id of every node is in range */
bool RRGraph::validate_node_segments() const {
    bool all_valid = true;
    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Skip this id */
            continue;
        }
        if (true == validate_node_segment(RRNodeId(id))) {
            continue;
        }
        /* Reach here it means we find an invalid segment id */
        all_valid = false;
        /* Print a warning! */
        VTR_LOG_WARN("Node %d has an invalid segment id (%d)!\n",
                     id, size_t(node_segment(RRNodeId(id))));
    }
    return all_valid;
}

/* Check if the switch id of a edge is in range */
bool RRGraph::validate_edge_switch(const RREdgeId& edge) const {
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    return valid_switch_id(edge_switches_[edge]);
}

/* Check if the switch id of every edge is in range */
bool RRGraph::validate_edge_switches() const {
    bool all_valid = true;
    for (size_t id = 0; id < num_edges_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_edge_id(RREdgeId(id))) {
            /* Skip this id */
            continue;
        }
        if (true == validate_edge_switch(RREdgeId(id))) {
            continue;
        }
        /* Reach here it means we find an invalid segment id */
        all_valid = false;
        /* Print a warning! */
        VTR_LOG_WARN("Edge %d has an invalid switch id (%d)!\n",
                     id, size_t(edge_switch(RREdgeId(id))));
    }
    return all_valid;
}

/* Check if a node is in the list of source_nodes of a edge */
bool RRGraph::validate_node_is_edge_src(const RRNodeId& node, const RREdgeId& edge) const {
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* assure a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    /* find if the node is the src */
    if (node == edge_src_node(edge)) {
        return true; /* confirmed source node*/
    } else {
        return false; /* not a source */
    }
}

/* Check if a node is in the list of sink_nodes of a edge */
bool RRGraph::validate_node_is_edge_sink(const RRNodeId& node, const RREdgeId& edge) const {
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* assure a valid edge id */
    VTR_ASSERT_SAFE(valid_edge_id(edge));
    /* find if the node is the sink */
    if (node == edge_sink_node(edge)) {
        return true; /* confirmed source node*/
    } else {
        return false; /* not a source */
    }
}

/* This function will check if a node has valid input edges
 * 1. Check the edge ids are valid
 * 2. Check the node is in the list of edge_sink_node
 */
bool RRGraph::validate_node_in_edges(const RRNodeId& node) const {
    bool all_valid = true;
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* Check each edge */
    for (auto edge : node_in_edges(node)) {
        /* assure a valid edge id */
        VTR_ASSERT_SAFE(valid_edge_id(edge));
        /* check the node is in the list of edge_sink_node */
        if (true == validate_node_is_edge_sink(node, edge)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d is in the input edge list of node %d while the node is not in edge's sink node list!\n",
                     size_t(edge), size_t(node));
        all_valid = false;
    }

    return all_valid;
}

/* This function will check if a node has valid output edges
 * 1. Check the edge ids are valid
 * 2. Check the node is in the list of edge_source_node
 */
bool RRGraph::validate_node_out_edges(const RRNodeId& node) const {
    bool all_valid = true;
    /* Assure a valid node id */
    VTR_ASSERT_SAFE(valid_node_id(node));
    /* Check each edge */
    for (auto edge : node_out_edges(node)) {
        /* assure a valid edge id */
        VTR_ASSERT_SAFE(valid_edge_id(edge));
        /* check the node is in the list of edge_sink_node */
        if (true == validate_node_is_edge_src(node, edge)) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d is in the output edge list of node %d while the node is not in edge's source node list!\n",
                     size_t(edge), size_t(node));
        all_valid = false;
    }

    return all_valid;
}

/* check all the edges of a node */
bool RRGraph::validate_node_edges(const RRNodeId& node) const {
    bool all_valid = true;

    if (false == validate_node_in_edges(node)) {
        all_valid = false;
    }
    if (false == validate_node_out_edges(node)) {
        all_valid = false;
    }

    return all_valid;
}

/* check if all the nodes' input edges are valid */
bool RRGraph::validate_nodes_in_edges() const {
    bool all_valid = true;
    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Skip this id */
            continue;
        }
        if (true == validate_node_in_edges(RRNodeId(id))) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        all_valid = false;
    }
    return all_valid;
}

/* check if all the nodes' output edges are valid */
bool RRGraph::validate_nodes_out_edges() const {
    bool all_valid = true;
    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Skip this id */
            continue;
        }
        if (true == validate_node_out_edges(RRNodeId(id))) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        all_valid = false;
    }
    return all_valid;
}

/* check all the edges of every node */
bool RRGraph::validate_nodes_edges() const {
    bool all_valid = true;

    if (false == validate_nodes_in_edges()) {
        all_valid = false;
    }
    if (false == validate_nodes_out_edges()) {
        all_valid = false;
    }

    return all_valid;
}

/* Check if source node of a edge is valid */
bool RRGraph::validate_edge_src_node(const RREdgeId& edge) const {
    return valid_node_id(edge_src_node(edge));
}

/* Check if sink node of a edge is valid */
bool RRGraph::validate_edge_sink_node(const RREdgeId& edge) const {
    return valid_node_id(edge_sink_node(edge));
}

/* Check if source nodes of a edge are all valid */
bool RRGraph::validate_edge_src_nodes() const {
    bool all_valid = true;
    for (size_t id = 0; id < num_edges_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_edge_id(RREdgeId(id))) {
            /* Skip this id */
            continue;
        }
        if (true == validate_edge_src_node(RREdgeId(id))) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d has a invalid source node %d!\n",
                     id, size_t(edge_src_node(RREdgeId(id))));
        all_valid = false;
    }
    return all_valid;
}

/* Check if source nodes of a edge are all valid */
bool RRGraph::validate_edge_sink_nodes() const {
    bool all_valid = true;
    for (size_t id = 0; id < num_edges_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_edge_id(RREdgeId(id))) {
            /* Skip this id */
            continue;
        }
        if (true == validate_edge_sink_node(RREdgeId(id))) {
            continue;
        }
        /* Reach here, it means there is something wrong! 
         * Print a warning  
         */
        VTR_LOG_WARN("Edge %d has a invalid sink node %d!\n",
                     id, size_t(edge_sink_node(RREdgeId(id))));
        all_valid = false;
    }
    return all_valid;
}

/* This function should be used when nodes, edges, switches and segments have been used after 
 * We will build the fast_lookup, partition edges and check 
 * This function run fundamental and optional checks on internal data 
 * Errors are thrown if fundamental checking fails
 * Warnings are thrown if optional checking fails
 */
bool RRGraph::validate() const {
    size_t num_err = 0;

    initialize_fast_node_lookup();

    /* Validate the sizes of nodes and node-related vectors 
     * Validate the sizes of edges and edge-related vectors 
     */
    if (false == validate_sizes()) {
        VTR_LOG_WARN("Fail in validating node- and edge-related vector sizes!\n");
        num_err++;
    }

    /* Fundamental check */
    if (false == validate_nodes_edges()) {
        VTR_LOG_WARN("Fail in validating edges connected to each node!\n");
        num_err++;
    }

    if (false == validate_node_segments()) {
        VTR_LOG_WARN("Fail in validating segment IDs of nodes !\n");
        num_err++;
    }

    if (false == validate_edge_switches()) {
        VTR_LOG_WARN("Fail in validating switch IDs of edges !\n");
        num_err++;
    }

    /* Error out if there is any fatal errors found */
    if (0 < num_err) {
        VTR_LOG_ERROR("Routing Resource graph is not valid due to %d fatal errors !\n",
                      num_err);
    }

    return (0 == num_err);
}

bool RRGraph::is_dirty() const {
    return dirty_;
}

void RRGraph::set_dirty() {
    dirty_ = true;
}

void RRGraph::clear_dirty() {
    dirty_ = false;
}

/* Reserve a list of nodes */
void RRGraph::reserve_nodes(const unsigned long& num_nodes) {
    /* Reserve the full set of vectors related to nodes */
    /* Basic information */
    this->node_types_.reserve(num_nodes);

    this->node_bounding_boxes_.reserve(num_nodes);

    this->node_capacities_.reserve(num_nodes);
    this->node_ptc_nums_.reserve(num_nodes);
    this->node_cost_indices_.reserve(num_nodes);
    this->node_directions_.reserve(num_nodes);
    this->node_sides_.reserve(num_nodes);
    this->node_Rs_.reserve(num_nodes);
    this->node_Cs_.reserve(num_nodes);
    this->node_rc_data_indices_.reserve(num_nodes);
    this->node_segments_.reserve(num_nodes);

    /* Edge-related vectors */
    this->node_num_in_edges_.reserve(num_nodes);
    this->node_num_out_edges_.reserve(num_nodes);
    this->node_num_non_configurable_in_edges_.reserve(num_nodes);
    this->node_num_non_configurable_out_edges_.reserve(num_nodes);
    this->node_edges_.reserve(num_nodes);
}

/* Reserve a list of edges */
void RRGraph::reserve_edges(const unsigned long& num_edges) {
    /* Reserve the full set of vectors related to edges */
    this->edge_src_nodes_.reserve(num_edges);
    this->edge_sink_nodes_.reserve(num_edges);
    this->edge_switches_.reserve(num_edges);
}

/* Reserve a list of switches */
void RRGraph::reserve_switches(const size_t& num_switches) {
    this->switch_ids_.reserve(num_switches);
    this->switches_.reserve(num_switches);
}

/* Reserve a list of segments */
void RRGraph::reserve_segments(const size_t& num_segments) {
    this->segment_ids_.reserve(num_segments);
    this->segments_.reserve(num_segments);
}

/* Mutators */
RRNodeId RRGraph::create_node(const t_rr_type& type) {
    /* Allocate an ID */
    RRNodeId node_id = RRNodeId(num_nodes_);
    /* Expand range of node ids */
    num_nodes_++;

    /* Initialize the attributes */
    node_types_.push_back(type);

    node_bounding_boxes_.emplace_back(-1, -1, -1, -1);

    node_capacities_.push_back(-1);
    node_ptc_nums_.push_back(std::vector<short>(1, -1));
    node_cost_indices_.push_back(-1);
    node_directions_.push_back(NO_DIRECTION);
    node_sides_.push_back(NUM_SIDES);
    node_Rs_.push_back(0.);
    node_Cs_.push_back(0.);
    node_rc_data_indices_.push_back(-1);
    node_segments_.push_back(RRSegmentId::INVALID());

    node_edges_.emplace_back(); //Initially empty

    node_num_in_edges_.emplace_back(0);
    node_num_out_edges_.emplace_back(0);
    node_num_non_configurable_in_edges_.emplace_back(0);
    node_num_non_configurable_out_edges_.emplace_back(0);

    invalidate_fast_node_lookup();

    VTR_ASSERT(validate_sizes());

    return node_id;
}

RREdgeId RRGraph::create_edge(const RRNodeId& source, const RRNodeId& sink, const RRSwitchId& switch_id, const bool& fake_switch) {
    VTR_ASSERT(valid_node_id(source));
    VTR_ASSERT(valid_node_id(sink));
    if (false == fake_switch) {
      VTR_ASSERT(valid_switch_id(switch_id));
    }

    /* Allocate an ID */
    RREdgeId edge_id = RREdgeId(num_edges_);
    /* Expand range of edge ids */
    num_edges_++;

    /* Initialize the attributes */
    edge_src_nodes_.push_back(source);
    edge_sink_nodes_.push_back(sink);
    edge_switches_.push_back(switch_id);

    //We do not create the entry in node_edges_ here!
    //For memory efficiency this is done when
    //rebuild_node_edges() is called (i.e. after all
    //edges have been created).

    VTR_ASSERT(validate_sizes());

    return edge_id;
}

void RRGraph::set_edge_switch(const RREdgeId& edge, const RRSwitchId& switch_id) {
    VTR_ASSERT(valid_edge_id(edge));
    VTR_ASSERT(valid_switch_id(switch_id));
    edge_switches_[edge] = switch_id;
}

RRSwitchId RRGraph::create_switch(const t_rr_switch_inf& switch_info) {
    //Allocate an ID
    RRSwitchId switch_id = RRSwitchId(switch_ids_.size());
    switch_ids_.push_back(switch_id);

    switches_.push_back(switch_info);

    return switch_id;
}

/* Create segment */
RRSegmentId RRGraph::create_segment(const t_segment_inf& segment_info) {
    //Allocate an ID
    RRSegmentId segment_id = RRSegmentId(segment_ids_.size());
    segment_ids_.push_back(segment_id);

    segments_.push_back(segment_info);

    return segment_id;
}

/* This function just marks the node to be removed with an INVALID id 
 * It also disconnects and mark the incoming and outcoming edges to be INVALID()
 * And then set the RRGraph as polluted (dirty_flag = true)
 * The compress() function should be called to physically remove the node 
 */
void RRGraph::remove_node(const RRNodeId& node) {
    //Invalidate all connected edges
    // TODO: consider removal of self-loop edges?
    for (auto edge : node_in_edges(node)) {
        remove_edge(edge);
    }
    for (auto edge : node_out_edges(node)) {
        remove_edge(edge);
    }

    //Mark node invalid
    invalid_node_ids_.insert(node);

    //Invalidate the node look-up
    invalidate_fast_node_lookup();

    set_dirty();
}

/* This function just marks the edge to be removed with an INVALID id 
 * And then set the RRGraph as polluted (dirty_flag = true)
 * The compress() function should be called to physically remove the edge 
 */
void RRGraph::remove_edge(const RREdgeId& edge) {
    RRNodeId src_node = edge_src_node(edge);
    RRNodeId sink_node = edge_sink_node(edge);

    /* Invalidate node to edge references
     * TODO: consider making this optional (e.g. if called from remove_node)
     */
    for (size_t i = 0; i < node_num_in_edges_[src_node]; ++i) {
        if (node_edges_[src_node][i] == edge) {
            node_edges_[src_node][i] = RREdgeId::INVALID();
            break;
        }
    }
    for (size_t i = node_num_in_edges_[sink_node]; i < node_num_in_edges_[sink_node] + node_num_out_edges_[sink_node]; ++i) {
        if (node_edges_[sink_node][i] == edge) {
            node_edges_[sink_node][i] = RREdgeId::INVALID();
            break;
        }
    }

    /* Mark edge invalid */
    invalid_edge_ids_.insert(edge);

    set_dirty();
}

void RRGraph::set_node_type(const RRNodeId& node, const t_rr_type& type) {
    VTR_ASSERT(valid_node_id(node));

    node_types_[node] = type;
}

void RRGraph::set_node_xlow(const RRNodeId& node, const short& xlow) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_xmin(xlow);
}

void RRGraph::set_node_ylow(const RRNodeId& node, const short& ylow) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_ymin(ylow);
}

void RRGraph::set_node_xhigh(const RRNodeId& node, const short& xhigh) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_xmax(xhigh);
}

void RRGraph::set_node_yhigh(const RRNodeId& node, const short& yhigh) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node].set_ymax(yhigh);
}

void RRGraph::set_node_bounding_box(const RRNodeId& node, const vtr::Rect<short>& bb) {
    VTR_ASSERT(valid_node_id(node));

    node_bounding_boxes_[node] = bb;
}

void RRGraph::set_node_capacity(const RRNodeId& node, const short& capacity) {
    VTR_ASSERT(valid_node_id(node));

    node_capacities_[node] = capacity;
}

void RRGraph::set_node_ptc_num(const RRNodeId& node, const short& ptc) {
    VTR_ASSERT(valid_node_id(node));

    /* For CHANX and CHANY, we will resize the ptc num to length of the node
     * For other nodes, we will always assign the first element
     */
    if ((CHANX == node_type(node)) || (CHANY == node_type(node))) {
        if ((size_t)node_length(node) + 1 != node_ptc_nums_[node].size()) {
            node_ptc_nums_[node].resize((size_t)node_length(node) + 1);
        }
        std::fill(node_ptc_nums_[node].begin(), node_ptc_nums_[node].end(), ptc);
    } else {
        VTR_ASSERT(1 == node_ptc_nums_[node].size());
        node_ptc_nums_[node][0] = ptc;
    }
}

void RRGraph::set_node_pin_num(const RRNodeId& node, const short& pin_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Pin number valid only for IPIN/OPIN RR nodes");

    set_node_ptc_num(node, pin_id);
}

void RRGraph::set_node_track_num(const RRNodeId& node, const short& track_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Track number valid only for CHANX/CHANY RR nodes");

    set_node_ptc_num(node, track_id);
}

void RRGraph::set_node_class_num(const RRNodeId& node, const short& class_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == SOURCE || node_type(node) == SINK, "Class number valid only for SOURCE/SINK RR nodes");

    set_node_ptc_num(node, class_id);
}

void RRGraph::add_node_track_num(const RRNodeId& node,
                                 const vtr::Point<size_t>& node_offset,
                                 const short& track_id) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Track number valid only for CHANX/CHANY RR nodes");

    if ((size_t)node_length(node) + 1 != node_ptc_nums_[node].size()) {
        node_ptc_nums_[node].resize((size_t)node_length(node) + 1);
    }

    size_t offset = node_offset.x() - node_xlow(node) + node_offset.y() - node_ylow(node);
    VTR_ASSERT(offset < node_ptc_nums_[node].size());

    node_ptc_nums_[node][offset] = track_id;
}

void RRGraph::set_node_cost_index(const RRNodeId& node, const short& cost_index) {
    VTR_ASSERT(valid_node_id(node));
    node_cost_indices_[node] = cost_index;
}

void RRGraph::set_node_direction(const RRNodeId& node, const e_direction& direction) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == CHANX || node_type(node) == CHANY, "Direct can only be specified on CHANX/CNAY rr nodes");

    node_directions_[node] = direction;
}

void RRGraph::set_node_side(const RRNodeId& node, const e_side& side) {
    VTR_ASSERT(valid_node_id(node));
    VTR_ASSERT_MSG(node_type(node) == IPIN || node_type(node) == OPIN, "Side can only be specified on IPIN/OPIN rr nodes");

    node_sides_[node] = side;
}

void RRGraph::set_node_R(const RRNodeId& node, const float& R) {
    VTR_ASSERT(valid_node_id(node));

    node_Rs_[node] = R;
}

void RRGraph::set_node_C(const RRNodeId& node, const float& C) {
    VTR_ASSERT(valid_node_id(node));

    node_Cs_[node] = C;
}

void RRGraph::set_node_rc_data_index(const RRNodeId& node, const short& rc_data_index) {
    VTR_ASSERT(valid_node_id(node));

    node_rc_data_indices_[node] = rc_data_index;
}

/*
 * Set a segment id for a node in rr_graph 
 */
void RRGraph::set_node_segment(const RRNodeId& node, const RRSegmentId& segment_id) {
    VTR_ASSERT(valid_node_id(node));

    /* Only CHANX and CHANY requires a valid segment id */
    if ((CHANX == node_type(node))
        || (CHANY == node_type(node))) {
        VTR_ASSERT(valid_segment_id(segment_id));
    }

    node_segments_[node] = segment_id;
}
void RRGraph::rebuild_node_edges() {
    node_edges_.resize(nodes().size());
    node_num_in_edges_.resize(nodes().size(), 0);
    node_num_out_edges_.resize(nodes().size(), 0);
    node_num_non_configurable_in_edges_.resize(nodes().size(), 0);
    node_num_non_configurable_out_edges_.resize(nodes().size(), 0);

    //Count the number of edges of each type
    for (RREdgeId edge : edges()) {
        if (!edge) continue;

        RRNodeId src_node = edge_src_node(edge);
        RRNodeId sink_node = edge_sink_node(edge);
        bool config = edge_is_configurable(edge);

        ++node_num_out_edges_[src_node];
        ++node_num_in_edges_[sink_node];
        if (!config) {
            ++node_num_non_configurable_out_edges_[src_node];
            ++node_num_non_configurable_in_edges_[sink_node];
        }
    }

    //Allocate precisely the correct space for each nodes edge list
    for (RRNodeId node : nodes()) {
        if (!node) continue;

        node_edges_[node] = std::make_unique<RREdgeId[]>(node_num_in_edges_[node] + node_num_out_edges_[node]);
    }

    //Insert the edges into the node lists
    {
        vtr::vector<RRNodeId, int> inserted_edge_cnt(nodes().size(), 0);
        for (RREdgeId edge : edges()) {
            RRNodeId src_node = edge_src_node(edge);
            RRNodeId sink_node = edge_sink_node(edge);

            node_edges_[src_node][inserted_edge_cnt[src_node]++] = edge;
            node_edges_[sink_node][inserted_edge_cnt[sink_node]++] = edge;
        }
    }

#if 0
    //TODO: Sanity Check remove!
    for (RRNodeId node : nodes()) {
        for (size_t iedge = 0; iedge < node_num_in_edges_[node] + node_num_out_edges_[node]; ++iedge) {
            RREdgeId edge = node_edges_[node][iedge];
            VTR_ASSERT(edge_src_node(edge) == node || edge_sink_node(edge) == node);
        }
    }
#endif

    //Partition each node's edge lists according to their type to line up with the counts

    auto is_configurable_edge = [&](const RREdgeId edge) {
        return edge_is_configurable(edge);
    };

    for (RRNodeId node : nodes()) {
        if (!node) continue;

        //We partition the nodes first by incoming/outgoing:
        //
        // +---------------------------+-----------------------------+
        // |             in            |           out               |
        // +---------------------------+-----------------------------+
        //
        //and then the two subsets by configurability. So the final ordering is:
        //
        // +-----------+---------------+------------+----------------+
        // | in_config | in_non_config | out_config | out_non_config |
        // +-----------+---------------+------------+----------------+
        //

        //Partition first into incoming/outgoing
        auto is_incoming_edge = [&](const RREdgeId edge) {
            return edge_sink_node(edge) == node;
        };
        /* Use stable_partition to keep the relative order,
         * This is mainly for comparing the RRGraph write with rr_node writer 
         * so that it is easy to check consistency
         */
        std::stable_partition(node_edges_[node].get(),
                              node_edges_[node].get() + node_num_in_edges_[node] + node_num_out_edges_[node],
                              is_incoming_edge);

        //Partition incoming by configurable/non-configurable
        std::stable_partition(node_edges_[node].get(),
                              node_edges_[node].get() + node_num_in_edges_[node],
                              is_configurable_edge);

        //Partition outgoing by configurable/non-configurable
        std::stable_partition(node_edges_[node].get() + node_num_in_edges_[node],
                              node_edges_[node].get() + node_num_in_edges_[node] + node_num_out_edges_[node],
                              is_configurable_edge);

#if 0
        //TODO: Sanity check remove!
        size_t nedges = node_num_in_edges_[node] + node_num_out_edges_[node];
        for (size_t iedge = 0; iedge < nedges; ++iedge) {
            RREdgeId edge = node_edges_[node][iedge];
            if (iedge < node_num_in_edges_[node]) { //Incoming
                VTR_ASSERT(edge_sink_node(edge) == node);
                if (iedge < node_num_in_edges_[node] - node_num_non_configurable_in_edges_[node]) {
                    VTR_ASSERT(edge_is_configurable(edge));
                } else {
                    VTR_ASSERT(!edge_is_configurable(edge));
                }
            } else { //Outgoing
                VTR_ASSERT(edge_src_node(edge) == node);
                if (iedge < node_num_in_edges_[node] + node_num_out_edges_[node] - node_num_non_configurable_out_edges_[node]) {
                    VTR_ASSERT(edge_is_configurable(edge));
                } else {
                    VTR_ASSERT(!edge_is_configurable(edge));
                }
            }
        }
#endif
    }
}

void RRGraph::build_fast_node_lookup() const {
    /* Free the current fast node look-up, we will rebuild a new one here */
    invalidate_fast_node_lookup();

    /* Get the max (x,y) and then we can resize the ndmatrix */
    vtr::Point<short> max_coord(0, 0);
    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Skip this id */
            continue;
        }
        max_coord.set_x(std::max(max_coord.x(), std::max(node_bounding_boxes_[RRNodeId(id)].xmax(), node_bounding_boxes_[RRNodeId(id)].xmin())));
        max_coord.set_y(std::max(max_coord.y(), std::max(node_bounding_boxes_[RRNodeId(id)].ymax(), node_bounding_boxes_[RRNodeId(id)].ymin())));
    }
    node_lookup_.resize({(size_t)max_coord.x() + 1, (size_t)max_coord.y() + 1, NUM_RR_TYPES + 1});

    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Skip this id */
            continue;
        }
        RRNodeId node = RRNodeId(id);
        /* Special for CHANX and CHANY, we should annotate in the look-up 
         * for all the (x,y) upto (xhigh, yhigh)
         */
        size_t x_start = std::min(node_xlow(node), node_xhigh(node));
        size_t y_start = std::min(node_ylow(node), node_yhigh(node));
        std::vector<size_t> node_x(std::abs(node_xlow(node) - node_xhigh(node)) + 1);
        std::vector<size_t> node_y(std::abs(node_ylow(node) - node_yhigh(node)) + 1);
        
        std::iota(node_x.begin(), node_x.end(), x_start);
        std::iota(node_y.begin(), node_y.end(), y_start);
        
        VTR_ASSERT(size_t(std::max(node_xlow(node), node_xhigh(node))) == node_x.back());
        VTR_ASSERT(size_t(std::max(node_ylow(node), node_yhigh(node))) == node_y.back());

        size_t itype = node_type(node);

        for (const size_t& x : node_x) {
            for (const size_t& y : node_y) {
                size_t ptc = node_ptc_num(node);
                /* Routing channel nodes may have different ptc num 
                 * Find the track ids using the x/y offset  
                 */
                if (CHANX == node_type(node)) {
                    ptc = node_track_ids(node)[x - node_xlow(node)];
                } else if (CHANY == node_type(node)) {
                    ptc = node_track_ids(node)[y - node_ylow(node)];
                }

                if (ptc >= node_lookup_[x][y][itype].size()) {
                    node_lookup_[x][y][itype].resize(ptc + 1);
                }

                size_t iside = -1;
                if (node_type(node) == OPIN || node_type(node) == IPIN) {
                    iside = node_side(node);
                } else {
                    iside = NUM_SIDES;
                }

                if (iside >= node_lookup_[x][y][itype][ptc].size()) {
                    node_lookup_[x][y][itype][ptc].resize(iside + 1);
                }

                //Save node in lookup
                node_lookup_[x][y][itype][ptc][iside] = node;
            }
       }
    }
}

void RRGraph::invalidate_fast_node_lookup() const {
    node_lookup_.clear();
}

bool RRGraph::valid_fast_node_lookup() const {
    return !node_lookup_.empty();
}

void RRGraph::initialize_fast_node_lookup() const {
    if (!valid_fast_node_lookup()) {
        build_fast_node_lookup();
    }
}

bool RRGraph::valid_node_id(const RRNodeId& node) const {
    return (size_t(node) < num_nodes_)
           && (!invalid_node_ids_.count(node));
}

bool RRGraph::valid_edge_id(const RREdgeId& edge) const {
    return (size_t(edge) < num_edges_)
           && (!invalid_edge_ids_.count(edge));
}

/* check if a given switch id is valid or not */
bool RRGraph::valid_switch_id(const RRSwitchId& switch_id) const {
    /* TODO: should we check the index of switch[id] matches ? */
    return size_t(switch_id) < switches_.size();
}

/* check if a given segment id is valid or not */
bool RRGraph::valid_segment_id(const RRSegmentId& segment_id) const {
    /* TODO: should we check the index of segment[id] matches ? */
    return size_t(segment_id) < segments_.size();
}

/**
 * Internal checking codes to ensure data consistency
 * If you add any internal data to RRGraph and update create_node/edge etc. 
 * you need to update the validate_sizes() here to make sure these
 * internal vectors are aligned to the id vectors
 */
bool RRGraph::validate_sizes() const {
    return validate_node_sizes()
           && validate_edge_sizes()
           && validate_switch_sizes()
           && validate_segment_sizes();
}

bool RRGraph::validate_node_sizes() const {
    return node_types_.size() == num_nodes_
           && node_bounding_boxes_.size() == num_nodes_
           && node_capacities_.size() == num_nodes_
           && node_ptc_nums_.size() == num_nodes_
           && node_cost_indices_.size() == num_nodes_
           && node_directions_.size() == num_nodes_
           && node_sides_.size() == num_nodes_
           && node_Rs_.size() == num_nodes_
           && node_Cs_.size() == num_nodes_
           && node_segments_.size() == num_nodes_
           && node_num_non_configurable_in_edges_.size() == num_nodes_
           && node_num_non_configurable_out_edges_.size() == num_nodes_
           && node_edges_.size() == num_nodes_;
}

bool RRGraph::validate_edge_sizes() const {
    return edge_src_nodes_.size() == num_edges_
           && edge_sink_nodes_.size() == num_edges_
           && edge_switches_.size() == num_edges_;
}

bool RRGraph::validate_switch_sizes() const {
    return switches_.size() == switch_ids_.size();
}

bool RRGraph::validate_segment_sizes() const {
    return segments_.size() == segment_ids_.size();
}

void RRGraph::compress() {
    vtr::vector<RRNodeId, RRNodeId> node_id_map(num_nodes_);
    vtr::vector<RREdgeId, RREdgeId> edge_id_map(num_edges_);

    build_id_maps(node_id_map, edge_id_map);

    clean_nodes(node_id_map);
    clean_edges(edge_id_map);

    rebuild_node_refs(edge_id_map);

    invalidate_fast_node_lookup();

    /* Clear invalid node list */
    invalid_node_ids_.clear();

    /* Clear invalid edge list */
    invalid_edge_ids_.clear();

    clear_dirty();
}

void RRGraph::build_id_maps(vtr::vector<RRNodeId, RRNodeId>& node_id_map,
                            vtr::vector<RREdgeId, RREdgeId>& edge_id_map) {
    /* Build node ids including invalid ids and compress */
    vtr::vector<RRNodeId, RRNodeId> node_ids;
    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Give and invalid id */
            node_ids.push_back(RRNodeId::INVALID());
            continue;
        }
        /* Reach here, this is a valid id, push to the edge list */
        node_ids.push_back(RRNodeId(id));
    }
    node_id_map = compress_ids(node_ids);

    /* Build edge ids including invalid ids and compress */
    vtr::vector<RREdgeId, RREdgeId> edge_ids;
    for (size_t id = 0; id < num_edges_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_edge_id(RREdgeId(id))) {
            /* Give and invalid id */
            edge_ids.push_back(RREdgeId::INVALID());
            continue;
        }
        /* Reach here, this is a valid id, push to the edge list */
        edge_ids.push_back(RREdgeId(id));
    }
    edge_id_map = compress_ids(edge_ids);
}

void RRGraph::clean_nodes(const vtr::vector<RRNodeId, RRNodeId>& node_id_map) {
    num_nodes_ = node_id_map.size();

    node_types_ = clean_and_reorder_values(node_types_, node_id_map);

    node_bounding_boxes_ = clean_and_reorder_values(node_bounding_boxes_, node_id_map);

    node_capacities_ = clean_and_reorder_values(node_capacities_, node_id_map);
    node_ptc_nums_ = clean_and_reorder_values(node_ptc_nums_, node_id_map);
    node_cost_indices_ = clean_and_reorder_values(node_cost_indices_, node_id_map);
    node_directions_ = clean_and_reorder_values(node_directions_, node_id_map);
    node_sides_ = clean_and_reorder_values(node_sides_, node_id_map);
    node_Rs_ = clean_and_reorder_values(node_Rs_, node_id_map);
    node_Cs_ = clean_and_reorder_values(node_Cs_, node_id_map);

    node_segments_ = clean_and_reorder_values(node_segments_, node_id_map);
    node_num_non_configurable_in_edges_ = clean_and_reorder_values(node_num_non_configurable_in_edges_, node_id_map);
    node_num_non_configurable_out_edges_ = clean_and_reorder_values(node_num_non_configurable_out_edges_, node_id_map);
    node_num_in_edges_ = clean_and_reorder_values(node_num_in_edges_, node_id_map);
    node_num_out_edges_ = clean_and_reorder_values(node_num_out_edges_, node_id_map);
    //node_edges_ = clean_and_reorder_values(node_edges_, node_id_map);

    VTR_ASSERT(validate_node_sizes());
}

void RRGraph::clean_edges(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map) {
    num_edges_ = edge_id_map.size();

    edge_src_nodes_ = clean_and_reorder_values(edge_src_nodes_, edge_id_map);
    edge_sink_nodes_ = clean_and_reorder_values(edge_sink_nodes_, edge_id_map);
    edge_switches_ = clean_and_reorder_values(edge_switches_, edge_id_map);

    VTR_ASSERT(validate_edge_sizes());
}

void RRGraph::rebuild_node_refs(const vtr::vector<RREdgeId, RREdgeId>& edge_id_map) {
    for (size_t id = 0; id < num_nodes_; ++id) {
        /* Try to find if this is an invalid id or not */
        if (!valid_node_id(RRNodeId(id))) {
            /* Skip this id */
            continue;
        }
        RRNodeId node = RRNodeId(id);

        auto begin = node_edges_[node].get();
        auto end = begin + node_num_in_edges_[node] + node_num_out_edges_[node];
        update_valid_refs(begin, end, edge_id_map);

        VTR_ASSERT_MSG(all_valid(begin, end), "All Ids should be valid");
    }
}

/* Empty all the vectors related to nodes */
void RRGraph::clear_nodes() {
    num_nodes_ = 0;
    node_types_.clear();
    node_bounding_boxes_.clear();

    node_capacities_.clear();
    node_ptc_nums_.clear();
    node_cost_indices_.clear();
    node_directions_.clear();
    node_sides_.clear();
    node_Rs_.clear();
    node_Cs_.clear();
    node_rc_data_indices_.clear();
    node_segments_.clear();

    node_num_in_edges_.clear();
    node_num_out_edges_.clear();
    node_num_non_configurable_in_edges_.clear();
    node_num_non_configurable_out_edges_.clear();

    node_edges_.clear();

    /* clean node_look_up */
    node_lookup_.clear();
}

/* Empty all the vectors related to edges */
void RRGraph::clear_edges() {
    num_edges_ = 0;
    edge_src_nodes_.clear();
    edge_sink_nodes_.clear();
    edge_switches_.clear();
}

/* Empty all the vectors related to switches */
void RRGraph::clear_switches() {
    switch_ids_.clear();
    switches_.clear();
}

/* Empty all the vectors related to segments */
void RRGraph::clear_segments() {
    segment_ids_.clear();
    segments_.clear();
}

/* Clean the rr_graph */
void RRGraph::clear() {
    clear_nodes();
    clear_edges();
    clear_switches();
    clear_segments();

    invalidate_fast_node_lookup();

    /* Clear invalid node list */
    invalid_node_ids_.clear();

    /* Clear invalid edge list */
    invalid_edge_ids_.clear();

    clear_dirty();
}
