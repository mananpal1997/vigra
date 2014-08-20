/************************************************************************/
/*                                                                      */
/*     Copyright 2014 by Thorsten Beier and Ullrich Koethe              */
/*                                                                      */
/*    This file is part of the VIGRA computer vision library.           */
/*    The VIGRA Website is                                              */
/*        http://hci.iwr.uni-heidelberg.de/vigra/                       */
/*    Please direct questions, bug reports, and contributions to        */
/*        ullrich.koethe@iwr.uni-heidelberg.de    or                    */
/*        vigra@informatik.uni-hamburg.de                               */
/*                                                                      */
/*    Permission is hereby granted, free of charge, to any person       */
/*    obtaining a copy of this software and associated documentation    */
/*    files (the "Software"), to deal in the Software without           */
/*    restriction, including without limitation the rights to use,      */
/*    copy, modify, merge, publish, distribute, sublicense, and/or      */
/*    sell copies of the Software, and to permit persons to whom the    */
/*    Software is furnished to do so, subject to the following          */
/*    conditions:                                                       */
/*                                                                      */
/*    The above copyright notice and this permission notice shall be    */
/*    included in all copies or substantial portions of the             */
/*    Software.                                                         */
/*                                                                      */
/*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND    */
/*    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES   */
/*    OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND          */
/*    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT       */
/*    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,      */
/*    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR     */
/*    OTHER DEALINGS IN THE SOFTWARE.                                   */
/*                                                                      */
/************************************************************************/

/**
 * This header provides definitions of graph-related algorithms
 */

#ifndef VIGRA_GRAPH_ALGORITHMS_HXX
#define VIGRA_GRAPH_ALGORITHMS_HXX

/*std*/
#include <algorithm>
#include <vector>
#include <functional>
#include <set>
#include <iomanip>

/*vigra*/
#include "graphs.hxx"
#include "graph_generalization.hxx"
#include "multi_gridgraph.hxx"
#include "priority_queue.hxx"
#include "union_find.hxx"
#include "adjacency_list_graph.hxx"
#include "graph_maps.hxx"
#include "timing.hxx"
//#include "openmp_helper.hxx"



namespace vigra{




    namespace detail_graph_algorithms{
        template <class GRAPH_MAP,class COMPERATOR>
        struct GraphItemCompare
        {
            
            GraphItemCompare(const GRAPH_MAP & map,const COMPERATOR & comperator)
            : map_(map),
              comperator_(comperator){

            }

            template<class KEY>
            bool operator()(const KEY & a, const KEY & b) const{
                return comperator_(map_[a],map_[b]);
            }

            const GRAPH_MAP & map_;
            const COMPERATOR & comperator_;
        };
    }

    /// \brief get a vector of Edge descriptors
    ///
    /// Sort the Edge descriptors given weights 
    /// and a comperator
    template<class GRAPH,class WEIGHTS,class COMPERATOR>
    void edgeSort(
        const GRAPH   & g,
        const WEIGHTS & weights,
        const COMPERATOR  & comperator,
        std::vector<typename GRAPH::Edge> & sortedEdges
    ){
        sortedEdges.resize(g.edgeNum());
        size_t c=0;
        for(typename GRAPH::EdgeIt e(g);e!=lemon::INVALID;++e){
            sortedEdges[c]=*e;
            ++c;
        }
        detail_graph_algorithms::GraphItemCompare<WEIGHTS,COMPERATOR> edgeComperator(weights,comperator);
        std::sort(sortedEdges.begin(),sortedEdges.end(),edgeComperator);
    }


    /// \brief copy a lemon node map
    template<class G,class A,class B>
    void copyNodeMap(const G & g,const A & a ,B & b){
        typename  G::NodeIt iter(g);
        while(iter!=lemon::INVALID){
            b[*iter]=a[*iter];
            ++iter;
        }

    }
    /// \brief copy a lemon edge map
    template<class G,class A,class B>
    void copyEdgeMap(const G & g,const A & a ,B & b){
        typename  G::EdgeIt iter(g);
        while(iter!=lemon::INVALID){
            b[*iter]=a[*iter];
            ++iter;
        }
    }
    /// \brief fill a lemon node map 
    template<class G,class A,class T>
    void fillNodeMap(const G & g, A & a, const T & value){
        typename  G::NodeIt iter(g);
        while(iter!=lemon::INVALID){
            a[*iter]=value;
            ++iter;
        }
    }
    /// \brief fill a lemon edge map
    template<class G,class A,class T>
    void fillEdgeMap(const G & g,A & a ,const T & value){
        typename  G::EdgeIt iter(g);
        while(iter!=lemon::INVALID){
            a[*iter]=value;
            ++iter;
        }
    }

    /// \brief make a region adjacency graph from a graph and labels w.r.t. that graph
    ///
    /// \param graphIn  : input graph
    /// \param labels   : labels w.r.t. graphIn
    /// \param[out] rag  : region adjacency graph 
    /// \param[out] affiliatedEdges : a vector of edges of graphIn for each edge in rag
    /// \param      ingoreLabel : optional label to ignore (default: -1 means no label will be ignored)
    ///
    template<
        class GRAPH_IN,
        class GRAPH_IN_NODE_LABEL_MAP
    >
    void makeRegionAdjacencyGraph(
        GRAPH_IN                   graphIn,
        GRAPH_IN_NODE_LABEL_MAP    labels,
        AdjacencyListGraph & rag,
        typename AdjacencyListGraph:: template EdgeMap< std::vector<typename GRAPH_IN::Edge> > & affiliatedEdges,
        const Int64   ignoreLabel=-1
    ){
        rag=AdjacencyListGraph();
        typedef typename GraphMapTypeTraits<GRAPH_IN_NODE_LABEL_MAP>::Value LabelType;
        typedef GRAPH_IN GraphIn;
        typedef AdjacencyListGraph GraphOut;

        typedef typename GraphIn::Edge   EdgeGraphIn;
        typedef typename GraphIn::NodeIt NodeItGraphIn;
        typedef typename GraphIn::EdgeIt EdgeItGraphIn;
        typedef typename GraphOut::Edge   EdgeGraphOut; 


        USETICTOC;


        std::cout<<"add nodes\n";
        TIC;
        for(NodeItGraphIn iter(graphIn);iter!=lemon::INVALID;++iter){
            const LabelType l=labels[*iter];
            if(ignoreLabel==-1 || static_cast<Int64>(l)!=ignoreLabel)
                rag.addNode(l);
        }
        TOC;

        std::cout<<"add edges\n";
        TIC;       
        for(EdgeItGraphIn e(graphIn);e!=lemon::INVALID;++e){
            const EdgeGraphIn edge(*e);
            const LabelType lu = labels[graphIn.u(edge)];
            const LabelType lv = labels[graphIn.v(edge)];
            if(  lu!=lv && ( ignoreLabel==-1 || (static_cast<Int64>(lu)!=ignoreLabel  && static_cast<Int64>(lv)!=ignoreLabel) )  ){
                // if there is an edge between lu and lv no new edge will be added
                rag.addEdge( rag.nodeFromId(lu),rag.nodeFromId(lv));
            }
        }
        TOC;


        std::cout<<"setup hyper edges\n";
        TIC
        //SET UP HYPEREDGES

        affiliatedEdges.assign(rag);
        for(EdgeItGraphIn e(graphIn);e!=lemon::INVALID;++e){
            const EdgeGraphIn edge(*e);
            const LabelType lu = labels[graphIn.u(edge)];
            const LabelType lv = labels[graphIn.v(edge)];
            //std::cout<<"edge between ?? "<<lu<<" "<<lv<<"\n";
            if(  lu!=lv && ( ignoreLabel==-1 || (static_cast<Int64>(lu)!=ignoreLabel  && static_cast<Int64>(lv)!=ignoreLabel) )  ){
                //std::cout<<"find edge between "<<lu<<" "<<lv<<"\n";
                EdgeGraphOut ragEdge= rag.findEdge(rag.nodeFromId(lu),rag.nodeFromId(lv));
                //std::cout<<"invalid?"<<bool(ragEdge==lemon::INVALID)<<" id "<<rag.id(ragEdge)<<"\n";
                affiliatedEdges[ragEdge].push_back(edge);
                //std::cout<<"write done\n";
            }
        }
        TOC;
    }





    template<unsigned int DIM, class DTAG, class AFF_EDGES>
    size_t affiliatedEdgesSerializationSize(
        const GridGraph<DIM,DTAG> & gridGraph,
        const AdjacencyListGraph & rag,
        const AFF_EDGES & affEdges
    ){
        size_t size = 0;

        typedef typename  AdjacencyListGraph::EdgeIt EdgeIt;
        typedef typename  AdjacencyListGraph::Edge Edge;

        for(EdgeIt iter(rag); iter!=lemon::INVALID; ++iter){
            const Edge e(*iter);
            size+=1;
            size+=affEdges[e].size()*(DIM+1);
        }
        return size;
    }

    template<class OUT_ITER, unsigned int DIM, class DTAG, class AFF_EDGES>
    void serializeAffiliatedEdges(
        const GridGraph<DIM,DTAG> & gridGraph,
        const AdjacencyListGraph & rag,
        const AFF_EDGES & affEdges,
        OUT_ITER outIter
    ){
      
        typedef typename  AdjacencyListGraph::EdgeIt EdgeIt;
        typedef typename  AdjacencyListGraph::Edge Edge;
        typedef typename  GridGraph<DIM,DTAG>::Edge GEdge;

        for(EdgeIt iter(rag); iter!=lemon::INVALID; ++iter){

            const Edge edge = *iter;
            const size_t numAffEdge = affEdges[edge].size();
            *outIter = numAffEdge; ++outIter;

            for(size_t i=0; i<numAffEdge; ++i){
                const GEdge gEdge = affEdges[edge][i];
                for(size_t d=0; d<DIM+1; ++d){
                    *outIter = gEdge[d]; ++outIter;
                }
            }
        }
    }

    template<class IN_ITER, unsigned int DIM, class DTAG, class AFF_EDGES>
    void deserializeAffiliatedEdges(
        const GridGraph<DIM,DTAG> & gridGraph,
        const AdjacencyListGraph & rag,
        AFF_EDGES & affEdges,
        IN_ITER begin,
        IN_ITER end
    ){
      
        typedef typename  AdjacencyListGraph::EdgeIt EdgeIt;
        typedef typename  AdjacencyListGraph::Edge Edge;
        typedef typename  GridGraph<DIM,DTAG>::Edge GEdge;

        affEdges.assign(rag);

        for(EdgeIt iter(rag); iter!=lemon::INVALID; ++iter){

            const Edge edge = *iter;
            const size_t numAffEdge = *begin; ++begin;

            for(size_t i=0; i<numAffEdge; ++i){
                GEdge gEdge;
                for(size_t d=0; d<DIM+1; ++d){
                    gEdge[d]=*begin; ++begin;
                }
                affEdges[edge].push_back(gEdge);
            }
        }
    }




    /// \brief shortest path computer
    template<class GRAPH,class WEIGHT_TYPE>
    class ShortestPathDijkstra{
    public:
        typedef GRAPH Graph;

        typedef typename Graph::Node Node;
        typedef typename Graph::NodeIt NodeIt;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::OutArcIt OutArcIt;

        typedef WEIGHT_TYPE WeightType;
        typedef ChangeablePriorityQueue<WeightType>           PqType;
        typedef typename Graph:: template NodeMap<Node>       PredecessorsMap;
        typedef typename Graph:: template NodeMap<WeightType> DistanceMap;
            
        /// \ brief constructor from graph
        ShortestPathDijkstra(const Graph & g)
        :   graph_(g),
            pq_(g.maxNodeId()+1),
            predMap_(g),
            distMap_(g)
        {
        }

        /// \brief run shortest path given edge weights
        ///
        /// \param weights : edge weights encoding the distance between adjacent nodes (must be non-negative) 
        /// \param source  : source node where shortest path should start
        /// \param target  : target node where shortest path should stop. If target is not given,
        ///     the shortest path from source to all reachable nodes is computed
        template<class WEIGHTS>
        void run(const WEIGHTS & weights,const Node & source,const Node & target = lemon::INVALID){
            source_=source;
            target_=target;
            this->initializeMaps();
            while(!pq_.empty() ){ //&& !finished){
                const Node topNode(graph_.nodeFromId(pq_.top()));
                if(topNode==target_){
                    break;
                }
                if(predMap_[topNode]!=lemon::INVALID || topNode==source_ ){
                    pq_.pop();
                    // loop over all neigbours
                    for(OutArcIt outArcIt(graph_,topNode);outArcIt!=lemon::INVALID;++outArcIt){
                        const Node otherNode = graph_.target(*outArcIt);
                        const size_t otherNodeId = graph_.id(otherNode);
    
                        if(pq_.contains(otherNodeId)){
                            const Edge edge(*outArcIt);
                            const WeightType currentDist     = distMap_[otherNode];
                            const WeightType alternativeDist = distMap_[topNode]+weights[edge];
                            if(alternativeDist<currentDist){
                                pq_.push(otherNodeId,alternativeDist);
                                distMap_[otherNode]=alternativeDist;
                                predMap_[otherNode]=topNode;
                            }
                        }

                    }
                }
            }
        }


        /// \brief get the graph
        const Graph & graph()const{
            return graph_;
        }
        /// \brief get the source node
        const Node & source()const{
            return source_;
        }
        /// \brief get the target node
        const Node & target()const{
            return target_;
        }

        /// \brief check if explicit target is given
        bool hasTarget()const{
            return target_!=lemon::INVALID;
        }

        /// \brief get the predecessors node map (after a call of run)
        const PredecessorsMap & predecessors()const{
            return predMap_;
        }
        /// \brief get the distances node map (after a call of run)
        const DistanceMap & distances()const{
            return distMap_;
        }
        /// \brief get the distance to a rarget node (after a call of run)
        WeightType distance(const Node & target)const{
            return distMap_[target];
        }


    private:


        void initializeMaps(){
            for(NodeIt n(graph_);n!=lemon::INVALID;++n){
                const Node node(*n);
                pq_.push(graph_.id(node),std::numeric_limits<WeightType>::infinity() );
                distMap_[node]=std::numeric_limits<WeightType>::infinity();
                predMap_[node]=lemon::INVALID;
            }
            distMap_[source_]=static_cast<WeightType>(0.0);
            pq_.push(graph_.id(source_),0.0);
        }

        const Graph  & graph_;
        PqType  pq_;
        PredecessorsMap predMap_;
        DistanceMap     distMap_;

        Node source_;
        Node target_;
    };

    /// \brief get the length in node units of a path
    template<class NODE,class PREDECESSORS>
    size_t pathLength(
        const NODE source,
        const NODE target,
        const PREDECESSORS & predecessors
    ){
        if(predecessors[target]==lemon::INVALID)
            return 0;
        else{
            NODE currentNode = target;
            size_t length=1;
            while(currentNode!=source){
                currentNode=predecessors[currentNode];
                length+=1;
            }
            return length;
        }
    }

    /// \brief Astar Shortest path search
    template<class GRAPH,class WEIGHTS,class PREDECESSORS,class DISTANCE,class HEURSTIC>
    void shortestPathAStar(
        const GRAPH         &           graph,
        const typename GRAPH::Node &    source,
        const typename GRAPH::Node &    target,
        const WEIGHTS       &           weights,
        PREDECESSORS        &           predecessors,
        DISTANCE            &           distance,
        const HEURSTIC      &           heuristic
    ){

        typedef GRAPH                       Graph;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::Node Node;
        typedef typename Graph::NodeIt NodeIt;
        typedef typename Graph::OutArcIt OutArcIt;
        typedef typename DISTANCE::value_type    DistanceType;

        typename  GRAPH:: template NodeMap<bool> closedSet(graph);
        vigra::ChangeablePriorityQueue<typename WEIGHTS::value_type> estimatedDistanceOpenSet(graph.maxNodeId()+1);
        // initialize
        for(NodeIt n(graph);n!=lemon::INVALID;++n){
            const Node node(*n);
            closedSet[node]=false;
            distance[node]=std::numeric_limits<DistanceType>::infinity();
            predecessors[node]=lemon::INVALID;
        }
        // distance and estimated distance for start node
        distance[source]=static_cast<DistanceType>(0.0);
        estimatedDistanceOpenSet.push(graph.id(source),heuristic(source,target));

        // while any nodes left in openSet
        while(!estimatedDistanceOpenSet.empty()){

            // get the node with the lpwest estimated distance in the open set
            const Node current = graph.nodeFromId(estimatedDistanceOpenSet.top());

            // reached target?
            if(current==target)
                break;

            // remove current from openSet
            // add current to closedSet
            estimatedDistanceOpenSet.pop();
            closedSet[current]=true;

            // iterate over neigbours of current
            for(OutArcIt outArcIt(graph,current);outArcIt!=lemon::INVALID;++outArcIt){

                // get neigbour node and id
                const Node neighbour = graph.target(*outArcIt);
                const size_t neighbourId = graph.id(neighbour);

                // if neighbour is not yet in closedSet
                if(!closedSet[neighbour]){

                    // get edge between current and neigbour
                    const Edge edge(*outArcIt);

                    // get tentative score
                    const DistanceType tenativeScore = distance[current] + weights[edge];

                    // neighbour NOT in openSet OR tentative score better than the current distance
                    if(!estimatedDistanceOpenSet.contains(neighbourId) || tenativeScore < distance[neighbour] ){
                        // set predecessors and distance
                        predecessors[neighbour]=current;
                        distance[neighbour]=tenativeScore;

                        // update the estimated cost from neighbour to target
                        // ( and neigbour will be (re)-added to openSet)
                        estimatedDistanceOpenSet.push(neighbourId,distance[neighbour]+heuristic(neighbour,target));
                    }
                }
            }
        }
    }
    
    template<
    class GRAPH, 
    class EDGE_WEIGHTS, 
    class NODE_WEIGHTS,
    class SEED_NODE_MAP,
    class WEIGHT_TYPE
    >
    void shortestPathSegmentation(
        const GRAPH & graph,
        const EDGE_WEIGHTS & edgeWeights,
        const NODE_WEIGHTS & nodeWeights,
        SEED_NODE_MAP & seeds
    ){

        typedef GRAPH Graph;

        typedef typename Graph::Node Node;
        typedef typename Graph::NodeIt NodeIt;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::OutArcIt OutArcIt;

        typedef WEIGHT_TYPE WeightType;
        typedef ChangeablePriorityQueue<WeightType>           PqType;
        typedef typename Graph:: template NodeMap<Node>       PredecessorsMap;
        typedef typename Graph:: template NodeMap<WeightType> DistanceMap;



        // allocate maps
        DistanceMap distMap(graph);
        PredecessorsMap predMap(graph);
        PqType pq(graph.maxNodeId()+1);


        for(NodeIt n(graph);n!=lemon::INVALID;++n){
            const Node node(*n);
            // not a seed
            if(seeds[node]==0){
                pq.push(graph.id(node),std::numeric_limits<WeightType>::infinity() );
                distMap[node]=std::numeric_limits<WeightType>::infinity();
                predMap[node]=lemon::INVALID;
            }
            // a seed
            else{

                // seeds are not added to pq
                // but direct neighbors of seed which are not seeds
                // will be added to the queue with their distance
                for(OutArcIt oa(graph,node); oa!=lemon::INVALID; ++oa){
                    Edge e(*oa);
                    const Node nNode=graph.target(*oa);

                    // check that other node is NOT a seed
                    if(seeds[nNode]==0){
                        
                        // set starting distance
                        const WeightType startDist = edgeWeights[e]+nodeWeights[nNode];
                        pq.push(graph.id(node),startDist );
                        distMap[nNode]=startDist;

                        // make seed node the predecessors
                        // of non seed direct neighbor
                        predMap[nNode]=node;
                    }
                }
            }
        }


        while(!pq.empty() ){ //&& !finished){
            const Node topNode(graph.nodeFromId(pq.top()));
            pq.pop();
            // loop over all neigbours
            for(OutArcIt outArcIt(graph,topNode);outArcIt!=lemon::INVALID;++outArcIt){
                const Node otherNode = graph.target(*outArcIt);
                const size_t otherNodeId = graph.id(otherNode);

                if(pq.contains(otherNodeId)){
                    const Edge edge(*outArcIt);
                    const WeightType currentDist     = distMap[otherNode];
                    const WeightType alternativeDist = distMap[topNode]+edgeWeights[edge]+nodeWeights[otherNode];
                    if(alternativeDist<currentDist){
                        pq.push(otherNodeId,alternativeDist);
                        distMap[otherNode]=alternativeDist;
                        predMap[otherNode]=topNode;
                    }
                }

            }
        }

        // do the labeling
        for(NodeIt n(graph);n!=lemon::INVALID;++n){
            Node node(*n);
            if(seeds[node]==0){
                int label = 0 ;
                Node pred=predMap[node];
                while(seeds[pred]==0){
                    pred=predMap[pred];
                }
                seeds[node]=seeds[pred];
            }
        }
    }



    namespace detail_watersheds_segmentation{

    struct RawPriorityFunctor{
        template<class L, class T>
        T operator()(const L label,const T  priority)const{
            return priority;
        }
    };

    template<class PRIORITY_TYPE,class LABEL_TYPE>
    struct CarvingFunctor{
        CarvingFunctor(const LABEL_TYPE backgroundLabel,const PRIORITY_TYPE & factor)
        :   backgroundLabel_(backgroundLabel),
            factor_(factor){
        }
        PRIORITY_TYPE operator()(const LABEL_TYPE label,const PRIORITY_TYPE  priority)const{
            return (label==backgroundLabel_ ? priority*factor_ : priority);
        }
        LABEL_TYPE     backgroundLabel_;
        PRIORITY_TYPE  factor_;
    };


    template<class GRAPH,class EDGE_WEIGHTS,class SEEDS,class PRIORITY_MANIP_FUNCTOR,class LABELS>
    void edgeWeightedWatershedsSegmentationImpl(
        const GRAPH & g,
        const EDGE_WEIGHTS      & edgeWeights,
        const SEEDS             & seeds,
        PRIORITY_MANIP_FUNCTOR  & priorManipFunctor,
        LABELS                  & labels
    ){  
        typedef GRAPH Graph;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::Node Node;
        typedef typename Graph::NodeIt NodeIt;
        typedef typename Graph::OutArcIt OutArcIt;

        typedef typename EDGE_WEIGHTS::Value WeightType;
        typedef typename LABELS::Value  LabelType;
        typedef typename Graph:: template NodeMap<bool>    NodeBoolMap;
        typedef PriorityQueue<Node,WeightType,true> PQ;

        PQ pq;
        NodeBoolMap inPQ(g);
        copyNodeMap(g,seeds,labels);
        fillNodeMap(g,inPQ,false);

        bool anySeed=false;
        for(NodeIt n(g);n!=lemon::INVALID;++n){
            const Node node(*n);
            if(labels[node]!=static_cast<LabelType>(0)){
                anySeed=true;
                for(OutArcIt a(g,node);a!=lemon::INVALID;++a){
                    const Edge edge(*a);
                    const Node neigbour=g.target(*a);
                    //std::cout<<"n- node "<<g.id(neigbour)<<"\n";
                    if(labels[neigbour]==static_cast<LabelType>(0) && !inPQ[neigbour]){
                        const WeightType priority = priorManipFunctor(labels[node],edgeWeights[edge]);
                        pq.push(neigbour,priority);
                        inPQ[neigbour]=true;
                    }
                }
            }
        }


        if(anySeed){

            while(!pq.empty()){
                const Node node       = pq.top();
                const LabelType label = labels[node]; 
                //std::cout<<"node "<<g.id(node)<<" with label "<<label<<"\n";
                if(label!=0){
                    throw std::runtime_error("seems like there are no seeds at all");
                }

                pq.pop();
                bool moreThanOneLabel = false;
                LabelType labelFound  = 0 ;
                for(OutArcIt a(g,node);a!=lemon::INVALID;++a){
                    const Edge edge(*a);
                    const Node neigbour=g.target(*a);
                    if(labels[neigbour]!=static_cast<LabelType>(0)){
                        if(labelFound==0){
                            labelFound=labels[neigbour];
                        }
                        else{
                            moreThanOneLabel=true;
                            break;
                        }
                    }
                }
                if(labelFound!=0 && !moreThanOneLabel ){
                    labels[node]=labelFound;
                    for(OutArcIt a(g,node);a!=lemon::INVALID;++a){
                        const Edge edge(*a);
                        const Node neigbour=g.target(*a);
                        if(labels[neigbour]==static_cast<LabelType>(0)){
                            if(!inPQ[neigbour]){
                                const WeightType priority = priorManipFunctor(labelFound,edgeWeights[edge]);
                                pq.push(neigbour,edgeWeights[edge]);
                                inPQ[neigbour]=true;
                            }
                        }
                    }
                }
            }


            for(NodeIt n(g);n!=lemon::INVALID;++n){
                const Node node(*n);
                if(labels[node]==static_cast<LabelType>(0)){

                    WeightType minWeight       = std::numeric_limits<WeightType>::infinity();
                    LabelType  minWeightLabel  = static_cast<LabelType>(0);
                    for(OutArcIt a(g,node);a!=lemon::INVALID;++a){
                        const Edge edge(*a);
                        const Node neigbour=g.target(*a);
                        const WeightType priority = priorManipFunctor(labels[neigbour],edgeWeights[edge]);
                        if(labels[neigbour]!=0 && priority<minWeight){
                            minWeight=priority;
                            minWeightLabel=labels[neigbour];
                        }
                    }
                    labels[node]=minWeightLabel;
                }
            }
        }
    }

    } // end namespace detail_watersheds_segmentation


    /// \brief edge weighted watersheds Segmentataion
    /// 
    /// \param graph: input graph
    /// \param edgeWeights : edge weights / edge indicator
    /// \param seeds : seed must be non empty!
    /// \param[labels] labels : resulting  nodeLabeling (not necessarily dense)
    template<class GRAPH,class EDGE_WEIGHTS,class SEEDS,class LABELS>
    void edgeWeightedWatershedsSegmentation(
        const GRAPH & g,
        const EDGE_WEIGHTS & edgeWeights,
        const SEEDS        & seeds,
        LABELS             & labels
    ){  
        detail_watersheds_segmentation::RawPriorityFunctor f;
        detail_watersheds_segmentation::edgeWeightedWatershedsSegmentationImpl(g,edgeWeights,seeds,f,labels);
    }   
    
    /// \brief edge weighted watersheds Segmentataion
    /// 
    /// \param graph: input graph
    /// \param edgeWeights : edge weights / edge indicator
    /// \param seeds : seed must be non empty!
    /// \param backgroundLabel : which label is background
    /// \param backgroundBias  : bias for background
    /// \param[labels] labels : resulting  nodeLabeling (not necessarily dense)
    template<class GRAPH,class EDGE_WEIGHTS,class SEEDS,class LABELS>
    void carvingSegmentation(
        const GRAPH                         & g,
        const EDGE_WEIGHTS                  & edgeWeights,
        const SEEDS                         & seeds,
        const typename LABELS::Value        backgroundLabel,
        const typename EDGE_WEIGHTS::Value  backgroundBias,
        LABELS                      & labels
    ){
        typedef typename EDGE_WEIGHTS::Value WeightType;
        typedef typename LABELS::Value       LabelType;
        detail_watersheds_segmentation::CarvingFunctor<WeightType,LabelType> f(backgroundLabel,backgroundBias);
        detail_watersheds_segmentation::edgeWeightedWatershedsSegmentationImpl(g,edgeWeights,seeds,f,labels);
    }


    /// \brief edge weighted watersheds Segmentataion
    /// 
    /// \param graph: input graph
    /// \param edgeWeights : edge weights / edge indicator
    /// \param nodeSizes : size of each node
    /// \param k : free parameter of felzenszwalb algorithm
    /// \param backgroundBias  : bias for background
    /// \param[labels] nodeLabeling :  nodeLabeling (not necessarily dense)
    /// \param nodeNumStopCond      : optional stopping condition
    template< class GRAPH , class EDGE_WEIGHTS, class NODE_SIZE,class NODE_LABEL_MAP>
    void felzenszwalbSegmentation(
        const GRAPH &         graph,
        const EDGE_WEIGHTS &  edgeWeights,
        const NODE_SIZE    &  nodeSizes,
        float           k,
        NODE_LABEL_MAP     &  nodeLabeling,
        const int             nodeNumStopCond = -1
    ){
        typedef GRAPH Graph;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::Node Node;

        typedef typename EDGE_WEIGHTS::Value WeightType;
        typedef typename EDGE_WEIGHTS::Value NodeSizeType;
        typedef typename Graph:: template NodeMap<WeightType>   NodeIntDiffMap;
        typedef typename Graph:: template NodeMap<NodeSizeType> NodeSizeAccMap;

        // initalize node size map  and internal diff map
        NodeIntDiffMap internalDiff(graph);
        NodeSizeAccMap nodeSizeAcc(graph);  
        copyNodeMap(graph,nodeSizes,nodeSizeAcc);
        fillNodeMap(graph,internalDiff,static_cast<WeightType>(0.0));



        // initlaize internal node diff map

        // sort the edges by their weights
        std::vector<Edge> sortedEdges;
        std::less<WeightType> comperator;
        edgeSort(graph,edgeWeights,comperator,sortedEdges);

        // make the ufd
        UnionFindArray<UInt64> ufdArray(graph.maxNodeId()+1);


        size_t nodeNum = graph.nodeNum();   


        while(true){
            // iterate over edges is the sorted order
            for(size_t i=0;i<sortedEdges.size();++i){
                const Edge e  = sortedEdges[i];
                const size_t rui = ufdArray.findIndex(graph.id(graph.u(e)));
                const size_t rvi = ufdArray.findIndex(graph.id(graph.v(e)));
                const Node   ru  = graph.nodeFromId(rui);
                const Node   rv  = graph.nodeFromId(rvi);
                if(rui!=rvi){

                    //check if to merge or not ?
                    const WeightType   w         = edgeWeights[e];
                    const NodeSizeType sizeRu    = nodeSizeAcc[ru];
                    const NodeSizeType sizeRv    = nodeSizeAcc[rv];
                    const WeightType tauRu       = static_cast<WeightType>(k)/static_cast<WeightType>(sizeRu);
                    const WeightType tauRv       = static_cast<WeightType>(k)/static_cast<WeightType>(sizeRv);
                    const WeightType minIntDiff  = std::min(internalDiff[ru]+tauRu,internalDiff[rv]+tauRv);
                    if(w<=minIntDiff){
                        // do merge
                        ufdArray.makeUnion(rui,rvi);
                        --nodeNum;
                        // update size and internal difference
                        const size_t newRepId = ufdArray.findIndex(rui);
                        const Node newRepNode = graph.nodeFromId(newRepId);
                        internalDiff[newRepNode]=w;
                        nodeSizeAcc[newRepNode] = sizeRu+sizeRv;
                    }
                }
                if(nodeNum==nodeNumStopCond){
                    break;
                }
            }
            if(nodeNumStopCond==-1){
                break;
            }
            else{
                if(nodeNum>nodeNumStopCond){
                    k *= 1.2f;
                }
                else{
                    break;
                }
            }
        }
        ufdArray.makeContiguous();
        for(typename  GRAPH::NodeIt n(graph);n!=lemon::INVALID;++n){
            const Node node(*n);
            nodeLabeling[node]=ufdArray.findLabel(graph.id(node));
        }
    } 




    namespace detail_graph_smoothing{

    template<
        class GRAPH, 
        class NODE_FEATURES_IN,
        class EDGE_WEIGHTS,
        class WEIGHTS_TO_SMOOTH_FACTOR,
        class NODE_FEATURES_OUT
    >
    void graphSmoothingImpl(
        const GRAPH & g,
        const NODE_FEATURES_IN   & nodeFeaturesIn,
        const EDGE_WEIGHTS       & edgeWeights,
        WEIGHTS_TO_SMOOTH_FACTOR & weightsToSmoothFactor,
        NODE_FEATURES_OUT        & nodeFeaturesOut

    ){
        typedef GRAPH Graph;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::Node Node;
        typedef typename Graph::NodeIt NodeIt;
        typedef typename Graph::OutArcIt OutArcIt;

        typedef typename NODE_FEATURES_IN::Value          NodeFeatureInValue;
        typedef typename NODE_FEATURES_OUT::Reference     NodeFeatureOutRef;
        typedef typename EDGE_WEIGHTS::ConstReference SmoothFactorType;


        //fillNodeMap(g, nodeFeaturesOut, typename NODE_FEATURES_OUT::value_type(0.0));

        for(NodeIt n(g);n!=lemon::INVALID;++n){

            const Node node(*n);

            NodeFeatureInValue    featIn  = nodeFeaturesIn[node];
            NodeFeatureOutRef     featOut = nodeFeaturesOut[node];

            featOut=0;
            float weightSum = 0.0;
            size_t degree    = 0;
            for(OutArcIt a(g,node);a!=lemon::INVALID;++a){
                const Edge edge(*a);
                const Node neigbour(g.target(*a));
                SmoothFactorType smoothFactor= weightsToSmoothFactor(edgeWeights[edge]);

                NodeFeatureInValue neighbourFeat = nodeFeaturesIn[neigbour];
                neighbourFeat*=smoothFactor;
                if(degree==0)
                    featOut = neighbourFeat;
                else
                    featOut += neighbourFeat;
                weightSum+=smoothFactor;
                ++degree;
            }
            // fixme..set me to right type 
            featIn*=static_cast<float>(degree);
            weightSum+=static_cast<float>(degree);
            featOut+=featIn;
            featOut/=weightSum;
        }
    }

    template<class T>
    struct ExpSmoothFactor{
        ExpSmoothFactor(const T lambda,const T edgeThreshold,const T scale)
        :   lambda_(lambda),
            edgeThreshold_(edgeThreshold),
            scale_(scale){
        }
        T operator()(const T weight){
            return weight> edgeThreshold_ ? 0 :  std::exp(-1.0*lambda_*weight)*scale_;
        }
        T lambda_;
        T edgeThreshold_;
        T scale_;
    };



    }


    /// \smooth node features of a graph
    ///
    /// \param g               : input graph
    /// \param nodeFeaturesIn  : input node features which should be smoothed       
    /// \param edgeIndicator   : edge indicator to indicate over which edges one should smooth        
    /// \param lambda          : scale edge indicator by lambda bevore taking negative exponent
    /// \param edgeThreshold   : edge threshold
    /// \param scale           : how much smoothing should be applied
    /// \param[out] nodeFeaturesOut : smoothed node features
    template<class GRAPH, class NODE_FEATURES_IN,class EDGE_INDICATOR,class NODE_FEATURES_OUT>
    void graphSmoothing(
        const GRAPH & g,
        const NODE_FEATURES_IN  & nodeFeaturesIn,
        const EDGE_INDICATOR    & edgeIndicator,
        const float lambda,
        const float edgeThreshold,
        const float scale,
        NODE_FEATURES_OUT       & nodeFeaturesOut
    ){
        detail_graph_smoothing::ExpSmoothFactor<float> functor(lambda,edgeThreshold,scale);
        detail_graph_smoothing::graphSmoothingImpl(g,nodeFeaturesIn,edgeIndicator,functor,nodeFeaturesOut);
    }

    /// \smooth node features of a graph
    ///
    /// \param g               : input graph
    /// \param nodeFeaturesIn  : input node features which should be smoothed       
    /// \param edgeIndicator   : edge indicator to indicate over which edges one should smooth        
    /// \param lambda          : scale edge indicator by lambda bevore taking negative exponent
    /// \param edgeThreshold   : edge threshold
    /// \param scale           : how much smoothing should be applied
    /// \param iteration       : how often should this algorithm be called recursively
    /// \param[out] nodeFeaturesOut : preallocated(!) buffer to store node features temp.
    /// \param[out] nodeFeaturesOut : smoothed node features
    template<class GRAPH, class NODE_FEATURES_IN,class EDGE_INDICATOR,class NODE_FEATURES_OUT>
    void recursiveGraphSmoothing(
        const GRAPH & g,
        const NODE_FEATURES_IN   & nodeFeaturesIn,
        const EDGE_INDICATOR     & edgeIndicator,
        const float lambda,
        const float edgeThreshold,
        const float scale,
        size_t                    iterations,
        NODE_FEATURES_OUT       & nodeFeaturesBuffer,
        NODE_FEATURES_OUT       & nodeFeaturesOut
    ){

        iterations = std::max(size_t(1),iterations);
        // initial run
        graphSmoothing(g,nodeFeaturesIn,edgeIndicator,lambda,edgeThreshold,scale,nodeFeaturesOut);
        iterations -=1;

        bool outAsIn=true;
        for(size_t i=0;i<iterations;++i){
            if(outAsIn){
                graphSmoothing(g,nodeFeaturesOut,edgeIndicator,lambda,edgeThreshold,scale,nodeFeaturesBuffer);
                outAsIn=false;
            }
            else{
                graphSmoothing(g,nodeFeaturesBuffer,edgeIndicator,lambda,edgeThreshold,scale,nodeFeaturesOut);
                outAsIn=true;
            }
        }
        if(!outAsIn){
            copyNodeMap(g,nodeFeaturesBuffer,nodeFeaturesOut);
        }
    }


    template<class GRAPH, class NODE_MAP, class EDGE_MAP>
    void nodeGtToEdgeGt(
        const GRAPH & g,
        const NODE_MAP & nodeGt,
        const Int64 ignoreLabel,
        EDGE_MAP & edgeGt
    ){
        typedef typename  GRAPH::Node Node;
        typedef typename  GRAPH::EdgeIt EdgeIt;
        typedef typename  GRAPH::Edge Edge;

        for(EdgeIt edgeIt(g); edgeIt!=lemon::INVALID; ++edgeIt){
            const Edge edge(*edgeIt);
            const Node u = g.u(edge);
            const Node v = g.v(edge);

            const Int64 lU = static_cast<Int64>(nodeGt[u]);
            const Int64 lV = static_cast<Int64>(nodeGt[v]);

            if(ignoreLabel==-1 || (lU!=ignoreLabel && lV!=ignoreLabel)){
                edgeGt[edge] = lU == lV ? 0 : 1;
            }
            else{
                edgeGt[edge] = 2;
            }
        }
    }






    /// project ground truth from a base graph, to a region adjacency graph.
    ///     
    ///
    ///
    ///
    template<class RAG, class BASE_GRAPH, class BASE_GRAPH_RAG_LABELS,  
             class BASE_GRAPH_GT,  class RAG_GT, class RAG_GT_QT>
    void projectGroundTruth(
        const RAG                   & rag,
        const BASE_GRAPH            & baseGraph,
        const BASE_GRAPH_RAG_LABELS & baseGraphRagLabels,
        const BASE_GRAPH_GT         & baseGraphGt,
        RAG_GT                      & ragGt,
        RAG_GT_QT                   & ragGtQt
    ){
        typedef typename BASE_GRAPH::Node BaseGraphNode;
        typedef typename BASE_GRAPH::NodeIt BaseGraphNodeIt;
        typedef typename RAG::NodeIt RagNodeIt;
        typedef typename RAG::Node RagNode;
        typedef typename BASE_GRAPH_RAG_LABELS::Value BaseRagLabelType;
        typedef typename BASE_GRAPH_GT::Value GtLabelType;
        typedef typename RAG_GT::Value RagGtLabelType;

        // overlap map
        typedef std::map<GtLabelType,UInt32> MapType;
        typedef typename MapType::const_iterator MapIter;
        typedef typename  RAG:: template NodeMap<MapType> Overlap;
        Overlap overlap(rag);
        
        size_t i=0;
        std::cout<<"\n"; 
        for(BaseGraphNodeIt baseNodeIter(baseGraph); baseNodeIter!=lemon::INVALID; ++baseNodeIter , ++i ){

            if (i%2000 == 0){
                std::cout<<"\r"<<std::setw(10)<< float(i)/float(baseGraph.nodeNum())<<std::flush;
            }

            const BaseGraphNode baseNode = *baseNodeIter;

            // get gt label
            const GtLabelType gtLabel = baseGraphGt[baseNode];

            // get the label which generated rag 
            // (node mapping from  bg-node to rag-node-id)
            const BaseRagLabelType bgRagLabel = baseGraphRagLabels[baseNode];
            const RagNode  ragNode = rag.nodeFromId(bgRagLabel);

            // fill overlap 
            overlap[ragNode][gtLabel]+=1;
        }
        std::cout<<"\n"; 
        // select label with max overlap
        for(RagNodeIt ragNodeIter(rag); ragNodeIter!=lemon::INVALID; ++ragNodeIter ){
            const RagNode  ragNode = *ragNodeIter;
            const MapType olMap = overlap[ragNode];
            UInt32 olSize=0;
            RagGtLabelType bestLabel = 0;
            for(MapIter  olIter = olMap.begin(); olIter!=olMap.end(); ++olIter ){
                if(olIter->second > olSize){
                    olSize = olIter->second;
                    bestLabel = olIter->first;
                }
            }
            ragGt[ragNode]=bestLabel;
        }
    }

    /// \brief create edge weights from an interpolated image
    ///
    /// \param g : input graph
    /// \param interpolatedImage : interpolated image
    /// \param[out] edgeWeights : edge weights
    template<unsigned int N, class T, class EDGEMAP>
    void edgeWeightsFromInterpolatedImage(
            const GridGraph<N, undirected_tag> & g,
            const MultiArray<N, T>  & interpolatedImage,
            EDGEMAP & edgeWeights,
            bool euclidean = false
    ){
        for (int d=0; d<N; ++d)
        {
            vigra_precondition(interpolatedImage.shape(d) == 2*g.shape()[d]-1, "interpolated shape must be shape*2-1");
        }

        typedef GridGraph<N, undirected_tag> Graph;
        typedef typename Graph::Edge Edge;
        typedef typename Graph::EdgeIt EdgeIt;
        typedef typename MultiArray<N, T>::difference_type CoordType;

        for (EdgeIt iter(g); iter!=lemon::INVALID; ++iter)
        {
            const Edge edge(*iter);
            const CoordType uCoord(g.u(edge));
            const CoordType vCoord(g.v(edge));
            const CoordType tCoord = uCoord+vCoord;
            if (euclidean)
            {
                int diffCounter = 0;
                for (int i=0; i<N; ++i)
                {
                    if (uCoord[i] != vCoord[i]) {
                        diffCounter++;
                    }
                }
                edgeWeights[edge] = sqrt(diffCounter) * interpolatedImage[tCoord];
            }
            else
            {
                edgeWeights[edge] = interpolatedImage[tCoord];
            }
        }
    }

    /// \brief Find indices of points on the edges
    ///
    /// \param rag : Region adjacency graph of the labels array
    /// \param g : Graph of labels array
    /// \param affiliatedEdges : The affiliated edges of the region adjacency graph
    /// \param labelsArray : The label image
    /// \param node : The node (of the region adjacency graph), whose edges shall be found
    template<class RAGGRAPH, class GRAPH, class RAGEDGES, unsigned int N, class T>
    MultiArray<2, MultiArrayIndex> ragFindEdges(
            const RAGGRAPH & rag,
            const GRAPH & graph,
            const RAGEDGES & affiliatedEdges,
            MultiArrayView<N, T> labelsArray,
            const typename RAGGRAPH::Node & node
    ){
        typedef typename GRAPH::Node Node;
        typedef typename GRAPH::Edge Edge;
        typedef typename RAGGRAPH::OutArcIt RagOutArcIt;
        typedef typename RAGGRAPH::Edge RagEdge;
        typedef typename GraphDescriptorToMultiArrayIndex<GRAPH>::IntrinsicNodeMapShape NodeCoordinate;

        T nodeLabel = rag.id(node);

        // Find edges and write them into a set.
        std::set< NodeCoordinate > edgeCoordinates;
        for (RagOutArcIt iter(rag, node); iter != lemon::INVALID; ++iter)
        {
            const RagEdge ragEdge(*iter);
            const std::vector<Edge> & affEdges = affiliatedEdges[ragEdge];
            for (int i=0; i<affEdges.size(); ++i)
            {
                Node u = graph.u(affEdges[i]);
                Node v = graph.v(affEdges[i]);
                T uLabel = labelsArray[u];
                T vLabel = labelsArray[v];

                NodeCoordinate coords;
                if (uLabel == nodeLabel)
                {
                    coords = GraphDescriptorToMultiArrayIndex<GRAPH>::intrinsicNodeCoordinate(graph, u);
                }
                else if (vLabel == nodeLabel)
                {
                    coords = GraphDescriptorToMultiArrayIndex<GRAPH>::intrinsicNodeCoordinate(graph, v);
                }
                else
                {
                    vigra_precondition(false, "You should not come to this part of the code.");
                }
                edgeCoordinates.insert(coords);
            }
        }

        // Fill the return array.
        MultiArray<2, MultiArrayIndex> edgePoints(Shape2(edgeCoordinates.size(), N));
        edgePoints.init(0);
        int next = 0;
        typedef typename std::set< NodeCoordinate >::iterator setIter;
        for (setIter iter = edgeCoordinates.begin(); iter!=edgeCoordinates.end(); ++iter)
        {
            NodeCoordinate coords = *iter;
            for (int k=0; k<coords.size(); ++k)
            {
                edgePoints(next, k) = coords[k];
            }
            next++;
        }
        return edgePoints;
    }

} // namespace vigra

#endif // VIGRA_GRAPH_ALGORITHMS_HXX
