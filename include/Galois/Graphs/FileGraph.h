/** Basic serialized graphs -*- C++ -*-
 * @file
 * @section License
 *
 * Galois, a framework to exploit amorphous data-parallelism in irregular
 * programs.
 *
 * Copyright (C) 2011, The University of Texas at Austin. All rights reserved.
 * UNIVERSITY EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES CONCERNING THIS
 * SOFTWARE AND DOCUMENTATION, INCLUDING ANY WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR ANY PARTICULAR PURPOSE, NON-INFRINGEMENT AND WARRANTIES OF
 * PERFORMANCE, AND ANY WARRANTY THAT MIGHT OTHERWISE ARISE FROM COURSE OF
 * DEALING OR USAGE OF TRADE.  NO WARRANTY IS EITHER EXPRESS OR IMPLIED WITH
 * RESPECT TO THE USE OF THE SOFTWARE OR DOCUMENTATION. Under no circumstances
 * shall University be liable for incidental, special, indirect, direct or
 * consequential damages or loss of profits, interruption of business, or
 * related expenses which may arise from use of Software or Documentation,
 * including but not limited to those resulting from defects in Software and/or
 * Documentation, or loss or inaccuracy of data of any kind.
 *
 * @section Description
 *
 * There are two main classes, ::FileGraph and ::LC_FileGraph. The former
 * represents the pure structure of a graph (i.e., whether an edge exists between
 * two nodes) and cannot be modified. The latter allows values to be stored on
 * nodes and edges, but the structure of the graph cannot be modified.
 *
 * An example of use:
 * 
 * \code
 * typedef Galois::Graph::LC_FileGraph<int,int> Graph;
 * 
 * // Create graph
 * Graph g;
 * g.structureFromFile(inputfile);
 *
 * // Traverse graph
 * for (Graph::active_iterator i = g.active_begin(), iend = g.active_end();
 *      i != iend;
 *      ++i) {
 *   Graph::GraphNode src = *i;
 *   for (Graph::neighbor_iterator j = g.neighbor_begin(src),
 *                                 jend = g.neighbor_end(src);
 *        j != jend;
 *        ++j) {
 *     Graph::GraphNode dst = *j;
 *     int edgeData = g.getEdgeData(src, dst);
 *     int nodeData = g.getData(dst);
 *   }
 * }
 * \endcode
 *
 * @author Andrew Lenharth <andrewl@lenharth.org>
 */
#ifndef GALOIS_GRAPHS_FILEGRAPH_H
#define GALOIS_GRAPHS_FILEGRAPH_H

#include "Galois/ConflictFlags.h"
#include "Galois/Runtime/Context.h"
#include <boost/iterator/counting_iterator.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/detail/endian.hpp>

using namespace GaloisRuntime;

namespace Galois {
namespace Graph {

//! Graph serialized to a file
class FileGraph {
public:
  typedef uint32_t GraphNode;

protected:
  void* masterMapping;
  size_t masterLength;
  uint64_t sizeEdgeTy;
  int masterFD;

  uint64_t* outIdx;
  uint32_t* outs;

  char* edgeData;

  uint64_t numEdges;
  uint64_t numNodes;

  uint64_t getEdgeIdx(GraphNode src, GraphNode dst) {
    for (uint32_t* ii = raw_neighbor_begin(src),
	   *ee = raw_neighbor_end(src); ii != ee; ++ii)
      if (convert32(*ii) == dst)
	return std::distance(outs, ii);
    return ~(uint64_t)0;
  }

  static uint32_t swap32(int32_t x) {
#ifdef __GNUC__
    return __builtin_bswap32(x);
#else
    return (x >> 24) | 
           ((x << 8) & 0x00FF0000) |
           ((x >> 8) & 0x0000FF00) |
           (x << 24);
#endif
  }

  static uint64_t swap64(int64_t x) {
#ifdef __GNUC__
    return __builtin_bswap64(x);
#else
    return (x>>56) | 
        ((x<<40) & 0x00FF000000000000) |
        ((x<<24) & 0x0000FF0000000000) |
        ((x<<8)  & 0x000000FF00000000) |
        ((x>>8)  & 0x00000000FF000000) |
        ((x>>24) & 0x0000000000FF0000) |
        ((x>>40) & 0x000000000000FF00) |
        (x<<56);
#endif
  }

  static uint32_t convert32(int32_t x) {
#ifdef BOOST_BIG_ENDIAN
    return swap32(x);
#else
    return x;
#endif
  }

  static uint64_t convert64(int64_t x) {
#ifdef BOOST_BIG_ENDIAN
    return swap64(x);
#else
    return x;
#endif
  }

  uint32_t* raw_neighbor_begin(GraphNode N, MethodFlag mflag = ALL) const {
    return (N == 0) ? &outs[0] : &outs[convert64(outIdx[N-1])];
  }

  uint32_t* raw_neighbor_end(GraphNode N, MethodFlag mflag = ALL) const {
    return &outs[convert64(outIdx[N])];
  }

  struct Convert : public std::unary_function<uint32_t, uint32_t> {
    uint32_t operator()(uint32_t x) const {
      return convert32(x);
    }
  };

public:
  // Node Handling

  //! Check if a node is in the graph (already added)
  bool containsNode(const GraphNode n) const {
    return n < numNodes;
  }

  // Edge Handling
  template<typename EdgeTy>
  EdgeTy& getEdgeData(GraphNode src, GraphNode dst, MethodFlag mflag = ALL) {
    assert(sizeEdgeTy == sizeof(EdgeTy));
    return ((EdgeTy*)edgeData)[getEdgeIdx(src, dst)];
  }

  // General Things
#ifdef BOOST_LITTLE_ENDIAN
  typedef uint32_t* neighbor_iterator;
#else
  typedef boost::transform_iterator<Convert, uint32_t*> neighbor_iterator;
#endif

  neighbor_iterator neighbor_begin(GraphNode N, MethodFlag mflag = ALL) const {
#ifdef BOOST_LITTLE_ENDIAN
      return raw_neighbor_begin(N, mflag);
#else
      return boost::make_transform_iterator(raw_neighbor_begin(N, mflag), Convert());
#endif
  }

  neighbor_iterator neighbor_end(GraphNode N, MethodFlag mflag = ALL) const {
#ifdef BOOST_LITTLE_ENDIAN
      return raw_neighbor_end(N, mflag);
#else
      return boost::make_transform_iterator(raw_neighbor_end(N, mflag), Convert());
#endif
  }

  bool has_neighbor(GraphNode N1, GraphNode N2, MethodFlag mflag = ALL) const {
    return std::find(neighbor_begin(N1), neighbor_end(N1), N2) != neighbor_end(N1);
  }

  typedef boost::counting_iterator<uint64_t> active_iterator;

  //! Iterate over nodes in graph (not thread safe)
  active_iterator active_begin() const {
    return active_iterator(0);
  }

  active_iterator active_end() const {
    return active_iterator(numNodes);
  }

  //! The number of nodes in the graph
  unsigned int size() const {
    return numNodes;
  }

  //! The number of edges in the graph
  unsigned int sizeEdges () const {
    return numEdges;
  }

  FileGraph();
  ~FileGraph();

  //! Read graph connectivity information from file
  bool structureFromFile(const char* filename);
};

//! Local computation graph (i.e., graph structure does not change)
template<typename NodeTy, typename EdgeTy>
class LC_FileGraph : public FileGraph {

  struct gNode : public GaloisRuntime::Lockable {
    NodeTy data;
    gNode() {}
  };

  //null if type is void
  cache_line_storage<gNode>* NodeData;

public:
  LC_FileGraph() :NodeData(0) {}
  ~LC_FileGraph() {
    if (NodeData)
      delete[] NodeData;
  }
  
  NodeTy& getData(GraphNode N, MethodFlag mflag = ALL) {
    GaloisRuntime::acquire(&NodeData[N].data, mflag);
    return NodeData[N].data.data;
  }

  EdgeTy& getEdgeData(GraphNode src, GraphNode dst, MethodFlag mflag = ALL) {
    return FileGraph::getEdgeData<EdgeTy>(src, dst, mflag);
  }

  //! Loads node data from file
  void nodeDataFromFile(const char* filename) {
    emptyNodeData();
    std::ifstream file(filename);
    for (uint64_t i = 0; i < numNodes; ++i)
      file >> NodeData[i];
  }
  
  //! Initializes node data for the graph to default values
  void emptyNodeData(NodeTy init = NodeTy()) {
    NodeData = new cache_line_storage<gNode>[numNodes];
    for (uint64_t i = 0; i < numNodes; ++i)
      NodeData[i].data.data = init;
  }
};

//! Local computation graph (i.e., graph structure does not change)
template<typename EdgeTy>
class LC_FileGraph<void, EdgeTy> : public FileGraph {

  struct gNode : public GaloisRuntime::Lockable {
    gNode() {}
  };

  //null if type is void
  cache_line_storage<gNode>* NodeData;

public:
  LC_FileGraph() :NodeData(0) {}
  ~LC_FileGraph() {
    if (NodeData)
      delete[] NodeData;
  }
  
  EdgeTy& getEdgeData(GraphNode src, GraphNode dst, MethodFlag mflag = ALL) {
    return FileGraph::getEdgeData<EdgeTy>(src, dst, mflag);
  }
};

template<typename NodeTy>
class LC_FileGraph<NodeTy, void>: public FileGraph { 

  struct gNode : public GaloisRuntime::Lockable {
    NodeTy data;
    gNode() {}
  };

  //null if type is void
  cache_line_storage<gNode>* NodeData;

public:
  LC_FileGraph() :NodeData(0) {}
  ~LC_FileGraph() {
    if (NodeData)
      delete[] NodeData;
  }
  
  NodeTy& getData(GraphNode N, MethodFlag mflag = ALL) {
    GaloisRuntime::acquire(&NodeData[N].data, mflag);
    return NodeData[N].data.data;
  }

  void nodeDataFromFile(const char* filename) {
    emptyNodeData();
    std::ifstream file(filename);
    for (uint64_t i = 0; i < numNodes; ++i)
      file >> NodeData[i];
  }
  
  void emptyNodeData(NodeTy init = NodeTy()) {
    NodeData = new cache_line_storage<gNode>[numNodes];
    for (uint64_t i = 0; i < numNodes; ++i)
      NodeData[i].data.data = init;
  }
};

template<>
class LC_FileGraph<void, void>: public FileGraph { 
};

}
}
#endif
