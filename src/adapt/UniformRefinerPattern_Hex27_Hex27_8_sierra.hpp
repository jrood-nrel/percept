// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef adapt_UniformRefinerPattern_Hex27_Hex27_8_sierra_hpp
#define adapt_UniformRefinerPattern_Hex27_Hex27_8_sierra_hpp


//#include "UniformRefinerPattern.hpp"
#include <adapt/sierra_element/RefinementTopology.hpp>
#include <adapt/sierra_element/StdMeshObjTopologies.hpp>

#define FACE_BREAKER_H27_H27 1
#if FACE_BREAKER_H27_H27
#include "UniformRefinerPattern_Quad9_Quad9_4_sierra.hpp"
#endif

  namespace percept {

    template <>
    class UniformRefinerPattern<shards::Hexahedron<27>, shards::Hexahedron<27>, 8, SierraPort > : public URP<shards::Hexahedron<27>, shards::Hexahedron<27>  >
    {

      //Teuchos::RCP< UniformRefinerPattern<shards::Line<2>, shards::Line<2>, 2, SierraPort >  > m_face_breaker;
      //UniformRefinerPatternBase * m_face_breaker;
#if FACE_BREAKER_H27_H27
      UniformRefinerPattern<shards::Quadrilateral<9>, shards::Quadrilateral<9>, 4, SierraPort > * m_face_breaker;
#endif

    public:

      UniformRefinerPattern(percept::PerceptMesh& eMesh, BlockNamesType block_names = BlockNamesType()) :  URP<shards::Hexahedron<27>, shards::Hexahedron<27>  >(eMesh)
      {
        m_primaryEntityRank = stk::topology::ELEMENT_RANK;

        setNeededParts(eMesh, block_names, true);
        Elem::StdMeshObjTopologies::bootstrap();

#if FACE_BREAKER_H27_H27

        m_face_breaker =  new UniformRefinerPattern<shards::Quadrilateral<9>, shards::Quadrilateral<9>, 4, SierraPort > (eMesh, block_names);

#endif

      }
      ~UniformRefinerPattern()
      {
        if (m_face_breaker) delete m_face_breaker;
      }

      void setSubPatterns( std::vector<UniformRefinerPatternBase *>& bp, percept::PerceptMesh& eMesh )
      {
        EXCEPTWATCH;
#if FACE_BREAKER_H27_H27
        bp.resize(2);
#else
        bp.resize(1);
#endif

        bp[0] = this;
#if FACE_BREAKER_H27_H27
        bp[1] = m_face_breaker;
#endif
      }

      virtual void doBreak() {}
      void fillNeededEntities(std::vector<NeededEntityType>& needed_entities)
      {
        needed_entities.resize(3);
        needed_entities[0] = NeededEntityType(m_eMesh.edge_rank(), 3u);
        needed_entities[1] = NeededEntityType(m_eMesh.face_rank(), 9u);
        needed_entities[2] = NeededEntityType(stk::topology::ELEMENT_RANK, 27u);
        //setToOne(needed_entities);
      }

      virtual unsigned getNumNewElemPerElem() { return 8; }

      //typedef boost::array<unsigned, ToTopology::node_count > refined_element_type;


      void
      createNewElements(percept::PerceptMesh& eMesh, NodeRegistry& nodeRegistry,
                        stk::mesh::Entity element,  NewSubEntityNodesType& new_sub_entity_nodes, vector<stk::mesh::Entity>::iterator& element_pool,
                        vector<stk::mesh::Entity>::iterator& ft_element_pool,
                        stk::mesh::FieldBase *proc_rank_field=0)
      {
        genericRefine_createNewElements(eMesh, nodeRegistry,
                                        element, new_sub_entity_nodes, element_pool, ft_element_pool, 
                                        proc_rank_field);
      }
    };

  }

#endif
