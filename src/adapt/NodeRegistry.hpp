// Copyright 2014 Sandia Corporation. Under the terms of
// Contract DE-AC04-94AL85000 with Sandia Corporation, the
// U.S. Government retains certain rights in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

/**
 * NodeRegistry: class that defines a map of key->value where key is a SubDimCell
 *   (e.g. an edge or face bewtween two elements), and value contains various data
 *   about the SubDimCell, such as which element owns the SubDimCell, an array of
 *   new nodes needed on it for refinement, etc.
 */

#ifndef adapt_NodeRegistry_hpp
#define adapt_NodeRegistry_hpp

#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <limits>
#include <cmath>
#include <utility>
#include <math.h>
#include <map>
#include <set>
#include <vector>

#include <Shards_BasicTopologies.hpp>
#include <Shards_CellTopologyData.h>

#include <stk_mesh/base/EntityKey.hpp>

#include <percept/stk_mesh.hpp>

#include <stk_util/environment/CPUTime.hpp>
#include <stk_util/parallel/ParallelComm.hpp>

#include <percept/NoMallocArray.hpp>
#include <percept/PerceptMesh.hpp>
#include <percept/Util.hpp>

#include <percept/PerceptBoostArray.hpp>

#include <boost/tuple/tuple_io.hpp>
#include <boost/tuple/tuple_comparison.hpp>

#define DEBUG_PRINT_11 0
#define NR_PRINT(a) do { if (DEBUG_PRINT_11) std::cout << #a << " = " << a ; } while(0)
#define NR_PRINT_OUT(a,out) do { if (DEBUG_PRINT_11) out << #a << " = " << a << std::endl; } while(0)

/// define only one of these to be 1
/// current best setting is NODE_REGISTRY_MAP_TYPE_BOOST = 1
#define NODE_REGISTRY_MAP_TYPE_BOOST 1

#define STK_ADAPT_NODEREGISTRY_USE_ENTITY_REPO 0
#define STK_ADAPT_NODEREGISTRY_DO_REHASH 1

#if NODE_REGISTRY_MAP_TYPE_BOOST
#include <boost/unordered_map.hpp>
#endif

#define DEBUG_NR_UNREF 0
#define DEBUG_NR_DEEP 0

#include <adapt/SubDimCell.hpp>
#include <adapt/SDCEntityType.hpp>
#include <adapt/NodeIdsOnSubDimEntityType.hpp>

  namespace percept {

    using std::vector;
    using std::map;
    using std::set;

    // pair of rank and number of entities of that rank needed on a SubDimCell
    //typedef std::pair<stk::mesh::EntityRank, unsigned> NeededEntityType;
    struct NeededEntityType
    {
      stk::mesh::EntityRank first;
      unsigned second;
      std::vector<int> third;
      NeededEntityType(stk::mesh::EntityRank f = stk::topology::NODE_RANK, unsigned s = 0) : first(f), second(s), third(0) {}
    };

    inline std::ostream &operator<<(std::ostream& out, const boost::array<stk::mesh::EntityId, 1>& arr)
    {
      out << arr[0];
      return out;
    }

    typedef boost::array<double, 2> Double2;
    // tuple storage: SDC_DATA_GLOBAL_NODE_IDS, SDC_DATA_OWNING_ELEMENT_KEY,  SDC_DATA_OWNING_SUBDIM_RANK, SDC_DATA_OWNING_SUBDIM_ORDINAL, SDC_DATA_SPACING
    typedef boost::tuple<NodeIdsOnSubDimEntityType, stk::mesh::EntityKey, unsigned char, unsigned char, Double2> SubDimCellData;

    //typedef SubDimCell<SDCEntityType> SubDimCell_SDCEntityType;
    typedef MySubDimCell<SDCEntityType, 4, CompareSDCEntityType> SubDimCell_SDCEntityType;

    inline std::ostream& operator<<(std::ostream& out,  SubDimCellData& val)
    {
      std::ostringstream ostr;
      ostr << "SDC:: node ids= [";
      for (unsigned ii=0; ii < val.get<SDC_DATA_GLOBAL_NODE_IDS>().size(); ++ii)
        {
          ostr << " " << val.get<SDC_DATA_GLOBAL_NODE_IDS>().m_entity_id_vector[ii];
        }
      ostr << "] owning element rank= " << val.get<SDC_DATA_OWNING_ELEMENT_KEY>().rank()
          << " id= " << val.get<SDC_DATA_OWNING_ELEMENT_KEY>().id()
          << " subDim-ord= " << (int)val.get<SDC_DATA_OWNING_SUBDIM_ORDINAL>()
          << " subDim-rank= " << (int)val.get<SDC_DATA_OWNING_SUBDIM_RANK>()
          << " spacing info= " << val.get<SDC_DATA_SPACING>()[0] << " " << val.get<SDC_DATA_SPACING>()[1];
      out << ostr.str() << std::endl;
      return out;
    }

    typedef stk::mesh::Entity EntityPtr;

    /// map of the node ids on a sub-dim entity to the data on the sub-dim entity

#if NODE_REGISTRY_MAP_TYPE_BOOST
#  ifdef STK_HAVE_TBB

    typedef tbb::scalable_allocator<std::pair<SubDimCell_SDCEntityType const, SubDimCellData> > RegistryAllocator;
    typedef boost::unordered_map<SubDimCell_SDCEntityType, SubDimCellData, my_fast_hash<SDCEntityType, 4>, my_fast_equal_to<SDCEntityType, 4>, RegistryAllocator > SubDimCellToDataMap;

#  else

    typedef boost::unordered_map<SubDimCell_SDCEntityType, SubDimCellData, my_fast_hash<SDCEntityType, 4>, my_fast_equal_to<SDCEntityType, 4> > SubDimCellToDataMap;
    typedef boost::unordered_map<stk::mesh::EntityId, EntityPtr > EntityRepo;

#  endif
#endif

    // Rank of sub-dim cells needing new nodes, which sub-dim entity, one non-owning element identifier, nodeId_elementOwnderId.first
    // FIXME - consider using bitfields to pack first two entries into a short - does this save anything on buffer size?
    enum CommDataTypeEnum {
      CDT_NEEDED_ENTITY_RANK,
      CDT_SUB_DIM_ENTITY_ORDINAL,
      CDT_NON_OWNING_ELEMENT_KEY
    };
    enum
      {
        NEW_NODE_IDS
      };
    // entity rank, ordinal of sub-dim entity, non-owning element key
    typedef boost::tuple<stk::mesh::EntityRank, unsigned, stk::mesh::EntityKey> CommDataType;

    enum NodeRegistryState {
      NRS_NONE,
      NRS_START_REGISTER_NODE,
      NRS_END_REGISTER_NODE,
      NRS_START_CHECK_FOR_REMOTE,
      NRS_END_CHECK_FOR_REMOTE,
      NRS_START_PARALLEL_OPS,
      NRS_START_GET_FROM_REMOTE,
      NRS_END_GET_FROM_REMOTE,
      NRS_END_PARALLEL_OPS
    };


    //========================================================================================================================
    //========================================================================================================================
    //========================================================================================================================
    class NodeRegistry
    {
    public:

      static const unsigned NR_MARK_NONE = 1u;
      static const unsigned NR_MARK = 2u;

      // FIXME use unordered_set
      typedef percept::SetOfEntities SetOfEntities;

      //========================================================================================================================
      // high-level interface

      NodeRegistry(percept::PerceptMesh& eMesh, 
                   bool useCustomGhosting = true) : m_eMesh(eMesh),
                                                    m_comm_all( new stk::CommAll(eMesh.parallel()) ),
                                                    m_ghosting(0),
                                                    m_pseudo_entities(*eMesh.get_bulk_data()),
                                                    m_useCustomGhosting(useCustomGhosting),
                                                    m_useAddNodeSharing(false),
                                                    m_checkForGhostedNodes(false),
                                                    m_gee_cnt(0), m_gen_cnt(0),
                                                    m_entity_repo(percept::EntityRankEnd),
                                                    m_debug(false),
                                                    m_state(NRS_NONE)
      {
        m_useCustomGhosting = true;  // toggle this to turn on add_node_sharing test
        m_useAddNodeSharing = !m_useCustomGhosting;
      }

      ~NodeRegistry() {
        if (m_comm_all)
          delete m_comm_all;
      }

      void init_comm_all();
      void init_entity_repo();
      void clear_dangling_nodes(SetOfEntities* nodes_to_be_deleted);
      void clear_elements_to_be_deleted(SetOfEntities* elements_to_be_deleted);
      void initialize();

      // avoid adding subDimEntity if it has a ghosted node
      void setCheckForGhostedNodes(bool val) { m_checkForGhostedNodes = val; }
      bool getCheckForGhostedNodes() { return m_checkForGhostedNodes; }

#define CHECK_DB 0
      void beginRegistration(int ireg=0, int nreg=1);
      void endRegistration(int ireg=0, int nreg=1);

      void beginLocalMeshMods(int ireg=0, int nreg=1);
      void endLocalMeshMods(int ireg=0, int nreg=1);

      void beginCheckForRemote(int ireg=0, int nreg=1);
      void endCheckForRemote(int ireg=0, int nreg=1);

      void beginGetFromRemote(int ireg=0, int nreg=1);
      void endGetFromRemote(int ireg=0, int nreg=1);


      /// Register the need for a new node on the sub-dimensional entity @param subDimEntity on element @param element.
      /// If the element is a ghost element, the entity is still registered: the locality/ownership of the new entity
      /// can be determined by the locality of the element (ghost or not).
      bool registerNeedNewNode(const stk::mesh::Entity element, NeededEntityType& needed_entity_rank, unsigned iSubDimOrd, bool needNodes,const CellTopologyData * const bucket_topo_data);

      /// check the newly registered node from the registry, which does one of three things, depending on what mode we are in:
      ///   1. counts buffer in prep for sending (just does a pack)
      ///   2. packs the buffer (after buffers are alloc'd)
      ///   3. returns the new node after all communications are done
      bool checkForRemote(const stk::mesh::Entity element, NeededEntityType& needed_entity_rank, unsigned iSubDimOrd, bool needNodes_notUsed,const CellTopologyData * const bucket_topo_data);
      bool getFromRemote(const stk::mesh::Entity element, NeededEntityType& needed_entity_rank, unsigned iSubDimOrd, bool needNodes_notUsed,const CellTopologyData * const bucket_topo_data);


      /// makes coordinates of this new node be the centroid of its sub entity
      void prolongateCoords(const stk::mesh::Entity element,  stk::mesh::EntityRank needed_entity_rank, unsigned iSubDimOrd);
      void prolongateCoordsAllSubDims(stk::mesh::Entity element);
      void prolongateField(const stk::mesh::Entity element,  stk::mesh::EntityRank needed_entity_rank, unsigned iSubDimOrd, stk::mesh::FieldBase *field);

      /// do interpolation for all fields, for the given nodes
      void prolongateFieldNodeVector(std::vector<stk::mesh::Entity> & nodes);

      double spacing_edge(std::vector<stk::mesh::Entity>& nodes,
                          unsigned iv0, unsigned iv1, unsigned nsz, unsigned nsp,  double lspc[8][3], double den_xyz[3], double *coord[8]);

      void normalize_spacing(stk::mesh::Entity element, std::vector<stk::mesh::Entity> &nodes,
                             unsigned nsz, unsigned nsp, double spc[8][3], double den_xyz[3], double *coord[8]);

      void prolongate(stk::mesh::FieldBase *field, unsigned *subDimSize_in=0, bool useFindValidCentroid = true);

      /// do interpolation for all fields
      void prolongateFields(const stk::mesh::Entity element,  stk::mesh::EntityRank needed_entity_rank, unsigned iSubDimOrd);

      /// do interpolation for all fields
      void prolongateFields();

      /// check for adding new nodes to existing parts based on sub-entity part ownership
      void addToExistingParts(const stk::mesh::Entity element,  stk::mesh::EntityRank needed_entity_rank, unsigned iSubDimOrd);

      void add_rbars(std::vector<std::vector<std::string> >& rbar_types );

      /// Check for adding new nodes to existing parts based on sub-entity part ownership.
      /// This version does it in bulk and thus avoids repeats on shared sub-dim entities.
      void addToExistingPartsNew();

      inline SubDimCellData& getNewNodeAndOwningElement(SubDimCell_SDCEntityType& subDimEntity)
      {
        return m_cell_2_data_map[subDimEntity];
      }

      inline SubDimCellData * getFromMapPtr(const SubDimCell_SDCEntityType& subDimEntity) const
      {
        const SubDimCellToDataMap::const_iterator i = m_cell_2_data_map.find( subDimEntity );
        return i != m_cell_2_data_map.end() ? (const_cast<SubDimCellData *>(&(i->second))) : 0 ;
      }

      inline SubDimCellData& getFromMap(const SubDimCell_SDCEntityType& subDimEntity) const
      {
        const SubDimCellToDataMap::const_iterator i = m_cell_2_data_map.find( subDimEntity );
        return * (const_cast<SubDimCellData *>(&(i->second)));
      }

      // allow client code to insert into map (e.g. for nodes created on-the-fly - see quad transition code)
      void forceInMap(std::vector<stk::mesh::Entity>& vecNodes, unsigned mark, stk::mesh::Entity element, stk::mesh::EntityRank needed_rank, unsigned iSubDimOrd );

      inline void putInMap(SubDimCell_SDCEntityType& subDimEntity, SubDimCellData& data)
      {

        bool debug = false;
        // if (data.get<SDC_DATA_OWNING_ELEMENT_KEY>().id() == 7577 ||data.get<SDC_DATA_OWNING_ELEMENT_KEY>().id() == 2214)
        //   {
        //     debug = false;
        //   }

        if (debug)
          {
            //const SubDimCell_SDCEntityType& subDimEntity = to_delete[itd]->first;
            SubDimCellData& nodeId_elementOwnderId = data;

            NodeIdsOnSubDimEntityType& nodeIds_onSE = nodeId_elementOwnderId.get<SDC_DATA_GLOBAL_NODE_IDS>();
            stk::mesh::EntityId owning_elementId = nodeId_elementOwnderId.get<SDC_DATA_OWNING_ELEMENT_KEY>().id();

            if (1)
              std::cout << "put in map: nodeIds_onSE.size= " << (nodeIds_onSE.size())
                        // <<  m_eMesh.identifier(nodeIds_onSE[0])
                        // << " id= " << nodeIds_onSE.m_entity_id_vector[0]
                        << " owning_elementId= " << owning_elementId
                        << " subDimEntity.size= " << subDimEntity.size()
                        << "\n" << m_eMesh.demangled_stacktrace(20)
                        << std::endl;
          }
        m_cell_2_data_map[subDimEntity] = data;
      }

      /// @param needNodes should be true in general; it's used by registerNeedNewNode to generate actual data or not on the subDimEntity
      ///   For local refinement, subDimEntity's needs are not always known uniquely by the pair {elementId, iSubDimOrd}; for example, in
      ///   an element-based marking scheme, the shared face between two elements may be viewed differently.  So, we need the ability to
      ///   override the default behavior of always creating new nodes on the subDimEntity, but still allow the entity to be created in
      ///   the NodeRegistry databse.

      typedef bool (NodeRegistry::*ElementFunctionPrototype)( const stk::mesh::Entity element, NeededEntityType& needed_entity_rank, unsigned iSubDimOrd, bool needNodes,
                                                              const CellTopologyData * const bucket_topo_data
                                                              );

      /// this is a helper method that loops over all sub-dimensional entities whose rank matches on of those in @param needed_entity_ranks
      ///    and registers that sub-dimensional entity as needing a new node.
      /// @param isGhost should be true if this element is a ghost, in which case this will call the appropriate method to set up for
      //     communications

      void doForAllSubEntities(ElementFunctionPrototype function, const stk::mesh::Entity element, vector<NeededEntityType>& needed_entity_ranks, const CellTopologyData * const bucket_topo_data);

      /// fill
      ///    @param subDimEntity with the stk::mesh::EntityId's of
      ///    the ordinal @param iSubDimOrd sub-dimensional entity of
      ///    @param element of rank
      ///    @param needed_entity_rank
      ///
      bool getSubDimEntity(SubDimCell_SDCEntityType& subDimEntity, const stk::mesh::Entity element, stk::mesh::EntityRank needed_entity_rank, unsigned iSubDimOrd,
                           const CellTopologyData * const bucket_topo_data_element_0 =0
                           );

      //========================================================================================================================
      // low-level interface

      unsigned total_size();
      unsigned local_size();

      /// Replace element ownership
      /// When remeshing during unrefinement, replace ownership of sub-dim entities by non-deleted elements
      bool replaceElementOwnership(const stk::mesh::Entity element, NeededEntityType& needed_entity_rank, unsigned iSubDimOrd, bool needNodes,
                                   const CellTopologyData * const bucket_topo_data
                                   );

      bool initializeEmpty(const stk::mesh::Entity element, NeededEntityType& needed_entity_rank, unsigned iSubDimOrd, bool needNodes_notUsed,
                           const CellTopologyData * const bucket_topo_data);

      void setAllReceivedNodeData();

      /// when a sub-dim entity is visited during node registration but is flagged as not being marked, and thus not requiring
      ///   any new nodes, we flag it with NR_MARK_NONE, then remove it here
      void removeUnmarkedSubDimEntities();

      // for communicating marks to other procs
      void communicate_marks();
      void verify_parallel_consistency(bool requireMarks, bool requireNodeIds, bool requireNodes, bool requireSubDimExists);

    private:
      void communicate_marks_pack(stk::CommAll& commAll);
      void communicate_marks_pack_stage_2(stk::CommAll& commAll);
      void communicate_marks_unpack(stk::CommAll& commAll, bool recv_from_non_owner);

      void verify_parallel_consistency_pack(stk::CommAll& commAll);
      void verify_parallel_consistency_unpack(stk::CommAll& commAll, bool requireMarks, bool requireNodeIds, bool requireNodes, bool requireSubDimExists);

      void do_add_node_sharing_comm();

    public:

      bool is_empty( const stk::mesh::Entity element, stk::mesh::EntityRank needed_entity_rank, unsigned iSubDimOrd);
      void query(std::ostream& out, stk::mesh::EntityId elementId, int rank, unsigned iSubDimOrd, std::string msg="", stk::mesh::EntityId *edge=0, SubDimCell_SDCEntityType* subDimEntityIn=0);
      void query(stk::mesh::EntityId elementId, int rank, unsigned iSubDimOrd, std::string msg="", stk::mesh::EntityId *edge=0, SubDimCell_SDCEntityType* subDimEntityIn=0)
      {
        query(std::cout, elementId, rank, iSubDimOrd, msg, edge, subDimEntityIn);
      }
      inline stk::mesh::Entity get_entity_using_find(stk::mesh::EntityRank& rank, const stk::mesh::EntityId& id) const
      {
        throw std::runtime_error("not implemented");
        const EntityRepo::const_iterator i = m_entity_repo[rank].find( id );
        return i != m_entity_repo[rank].end() ? i->second : stk::mesh::Entity() ;
      }

      static std::string print_string(PerceptMesh& eMesh, const SubDimCell_SDCEntityType& subDimEntity)
      {
        std::ostringstream ost;
        ost << "SubDimCell:";
        for (unsigned ii=0; ii < subDimEntity.size(); ++ii)
          {
            ost << " " << eMesh.identifier(subDimEntity[ii]);
          }
        return ost.str();
      }
      static void print(PerceptMesh& eMesh, const SubDimCell_SDCEntityType& subDimEntity)
      {
        std::cout << print_string(eMesh, subDimEntity);
      }

      inline stk::mesh::Entity get_entity(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return bulk.get_entity(rank, id);
      }

      inline stk::mesh::Entity get_entity_I(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return bulk.get_entity(rank, id);
      }

      inline stk::mesh::Entity get_entity_Ia(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return bulk.get_entity(rank, id);
      }

      inline stk::mesh::Entity get_entity_Ib(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return bulk.get_entity(rank, id);
      }

      inline stk::mesh::Entity get_entity_element(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return get_entity(bulk, rank, id);
      }

      inline stk::mesh::Entity get_entity_node_I(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return get_entity_I(bulk, rank, id);
      }

      inline stk::mesh::Entity get_entity_node_Ia(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, NodeIdsOnSubDimEntityType& nodeIds_onSE, unsigned index)
      {
        stk::mesh::Entity entity = nodeIds_onSE[index];
        return entity;
      }

      inline stk::mesh::Entity get_entity_node_Ib(stk::mesh::BulkData& bulk, stk::mesh::EntityRank rank, stk::mesh::EntityId id)
      {
        return get_entity_Ib(bulk, rank, id);
      }

      NodeIdsOnSubDimEntityType* getNewNodesOnSubDimEntity(const stk::mesh::Entity element,  stk::mesh::EntityRank& needed_entity_rank, unsigned iSubDimOrd);

      void checkDB(std::string msg="");

      /// allocate the send/recv buffers for all-to-all communication
      bool allocateBuffers();
      void communicate();
      void unpack();

      /// after registering all needed nodes, this method is used to request new nodes on this processor
      void createNewNodesInParallel();

      /// unpacks the incoming information in @param buffer_entry and adds that information to my local node registry
      /// (i.e. the map of sub-dimensional entity to global node id is updated)
      void createNodeAndConnect(CommDataType& buffer_entry, NodeIdsOnSubDimEntityType& nodeIds_onSE, unsigned from_proc, vector<stk::mesh::EntityProc>& nodes_to_ghost);

    public:
      inline SubDimCellToDataMap& getMap() { return  m_cell_2_data_map; }
      inline PerceptMesh& getMesh() { return m_eMesh; }
      inline bool getUseCustomGhosting() { return m_useCustomGhosting; }

      // remove any sub-dim entities from the map that have a node in deleted_nodes
      void cleanDeletedNodes(SetOfEntities& deleted_nodes,
                             SetOfEntities& kept_nodes_orig_minus_kept_nodes,
                             SubDimCellToDataMap& to_save,
                             bool debug=false);

      // further cleanup of the NodeRegistry db - some elements get deleted on some procs but the ghost elements
      //   are still in the db - the easiest way to detect this is as used here: during modification_begin(),
      //   cleanup of cached transactions are performed, then we find these by seeing if our element id's are
      //   no longer in the stk_mesh db.
      void clear_element_owner_data_phase_2(bool resize_nodeId_data=true, bool mod_begin_end=true, SetOfEntities * elemsToBeDeleted=0);

      // remove/zero any data that points to a deleted element
      // called by Refiner with "children_to_be_removed_with_ghosts"
      void clear_element_owner_data( SetOfEntities& elems_to_be_deleted, bool resize_nodeId_data=true);

      void dumpDB(std::string msg="");

      // estimate of memory used by this object
      unsigned get_memory_usage();

    private:
      percept::PerceptMesh& m_eMesh;
      stk::CommAll * m_comm_all;
      SubDimCellToDataMap m_cell_2_data_map;

      vector<stk::mesh::EntityProc> m_nodes_to_ghost;
      stk::mesh::Ghosting *m_ghosting;
      SetOfEntities m_pseudo_entities;
      bool m_useCustomGhosting;
      bool m_useAddNodeSharing;
      bool m_checkForGhostedNodes;

    public:
      int m_gee_cnt;
      int m_gen_cnt;
      //const CellTopologyData * const m_cell_topo_data;
      std::vector<EntityRepo> m_entity_repo;

      bool m_debug;

      NodeRegistryState m_state;
    };
  } // namespace percept

#endif