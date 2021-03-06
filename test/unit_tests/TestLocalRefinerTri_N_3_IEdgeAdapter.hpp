// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef adapt_TestLocalRefinerTri_N_3_IEdgeAdapter_hpp
#define adapt_TestLocalRefinerTri_N_3_IEdgeAdapter_hpp

#include <adapt/IEdgeAdapter.hpp>

  namespace percept {

    //========================================================================================================================
    //========================================================================================================================
    //========================================================================================================================
    /**
     * A test implementation as a use case for IEdgeAdapter
     */
    class TestLocalRefinerTri_N_3_IEdgeAdapter : public IEdgeAdapter
    {
    public:
      TestLocalRefinerTri_N_3_IEdgeAdapter(percept::PerceptMesh& eMesh, UniformRefinerPatternBase & bp, stk::mesh::FieldBase *proc_rank_field=0);

      //virtual ElementUnrefineCollection  buildUnrefList();

    protected:

      /// Client supplies these methods - given an element, which edge, and the nodes on the edge, return instruction on what to do to the edge,
      ///    DO_NOTHING (nothing), DO_REFINE (refine), DO_UNREFINE
      virtual int markEdge(const stk::mesh::Entity element, unsigned which_edge, stk::mesh::Entity node0, stk::mesh::Entity node1,
                           double *coord0, double *coord1, std::vector<int>* existing_edge_marks) ;

    };

    // This is a very specialized test that is used in unit testing only (see unit_localRefiner/break_tri_to_tri_N_3_IEdgeAdapter in UnitTestLocalRefiner.cpp)

    TestLocalRefinerTri_N_3_IEdgeAdapter::TestLocalRefinerTri_N_3_IEdgeAdapter(percept::PerceptMesh& eMesh, UniformRefinerPatternBase &  bp, stk::mesh::FieldBase *proc_rank_field) : 
      IEdgeAdapter(eMesh, bp, proc_rank_field)
    {
    }


    int TestLocalRefinerTri_N_3_IEdgeAdapter::
    markEdge(const stk::mesh::Entity element, unsigned which_edge, stk::mesh::Entity node0, stk::mesh::Entity node1,
             double *coord0, double *coord1, std::vector<int>* existing_edge_marks)
    {
      int mark=0;
      
      // refine test
      {
        // vertical line position
        const double vx = 0.21;

        // horizontal line position
        const double vy = 1.21;

        // choose to refine or not 
        if (
            ( std::fabs(coord0[0]-coord1[0]) > 1.e-3 &&
              ( (coord0[0] < vx && vx < coord1[0]) || (coord1[0] < vx && vx < coord0[0]) )
              )
            ||
            ( std::fabs(coord0[1]-coord1[1]) > 1.e-3 &&
              ( (coord0[1] < vy && vy < coord1[1]) || (coord1[1] < vy && vy < coord0[1]) )
              )
            )
          {
            mark |= DO_REFINE;
          }
      }

      // choose to unrefine or not
      {
        if (coord0[0] > 1.0001 || coord0[1] > 1.0001 || coord1[0] > 1.0001 || coord1[1] > 1.0001)
          {
          }
        else
          mark |= DO_UNREFINE;
      }

      return mark;
    }


  }

#endif
