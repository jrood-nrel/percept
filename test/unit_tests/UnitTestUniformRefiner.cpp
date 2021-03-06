// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.


#include <percept/Percept.hpp>

#include <stk_util/environment/WallTime.hpp>
#include <stk_util/diag/PrintTable.hpp>

#include <Teuchos_ScalarTraits.hpp>
#include <Teuchos_RCP.hpp>

#include <percept/Util.hpp>
#include <percept/ExceptionWatch.hpp>
#include <percept/GeometryVerifier.hpp>
#include <percept/function/StringFunction.hpp>
#include <percept/function/FieldFunction.hpp>
#include <percept/function/ConstantFunction.hpp>
#include <percept/PerceptMesh.hpp>

#include <percept/mesh/mod/smoother/SpacingFieldUtil.hpp>

#include <adapt/UniformRefinerPattern.hpp>
#include <adapt/UniformRefiner.hpp>
#include <adapt/RefinerUtil.hpp>
#include <adapt/UniformRefinerPattern_def.hpp>

#include <percept/fixtures/SingleTetFixture.hpp>
#include <percept/fixtures/PyramidFixture.hpp>

#include <stk_unit_tests/stk_mesh_fixtures/HexFixture.hpp>


#include <gtest/gtest.h>

#include <unit_tests/UnitTestSupport.hpp>

#include <stk_io/IossBridge.hpp>

#include <boost/lexical_cast.hpp>

#include <stk_util/environment/WallTime.hpp>
#include <stk_util/diag/PrintTable.hpp>

#include <percept/Percept.hpp>
#include <percept/Util.hpp>
#include <percept/ExceptionWatch.hpp>

#include <adapt/sierra_element/StdMeshObjTopologies.hpp>
#include <percept/RunEnvironment.hpp>

#include <percept/fixtures/Fixture.hpp>
#include <percept/fixtures/BeamFixture.hpp>
#include <percept/fixtures/HeterogeneousFixture.hpp>
#include <percept/fixtures/QuadFixture.hpp>
#include <percept/fixtures/WedgeFixture.hpp>

#include <stdexcept>
#include <sstream>
#include <vector>
#include <cmath>
#include <iostream>
#include <fstream>
#include <string>
#include <typeinfo>

#include <math.h>
#include <stk_util/parallel/Parallel.hpp>

  namespace percept {
    namespace unit_tests {

#define DO_TESTS 1
#if DO_TESTS
      static int print_infoLevel = 0;

      /// configuration: you can choose where to put the generated Exodus files (see variables input_files_loc, output_files_loc)
      /// The following defines where to put the input and output files created by this set of functions

#if 1
      const std::string input_files_loc="./input_files_";
      const std::string output_files_loc="./output_files_";
#else
      const std::string input_files_loc="./input_files/";
      const std::string output_files_loc="./output_files/";
#endif

#define EXTRA_PRINT 0

      /// This function either writes the given mesh to a file in Exodus format (option 0)
      ///   or, under option 1, checks if the file already exists, and if so, treats that
      ///   file as the "gold" copy and does a regression difference check.


      static void save_or_diff(PerceptMesh& eMesh, std::string filename, int option = 0)
      {
        return UnitTestSupport::save_or_diff(eMesh, filename, option);
      }

      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Creates meshes for use in later tests
      /// 1. Create hex mesh from a fixture and write it in Exodus format for use later.
      /// 2. Read the hex mesh and convert it to tet elements using adapt/UniformRefiner, write it in Exodus format

      //TEST(unit_uniformRefiner, build_meshes)
      static void fixture_setup_0()
      {
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );
        // start_demo_uniformRefiner_hex8_build
        {
          percept::PerceptMesh eMesh(3u);

          unsigned p_size = eMesh.get_parallel_size();

          // generate a 4x4x(4*p_size) mesh
          std::string gmesh_spec = std::string("4x4x")+toString(4*p_size)+std::string("|bbox:0,0,0,1,1,1");
          eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));
          eMesh.commit();
          save_or_diff(eMesh, input_files_loc+"hex_fixture.e");

          // end_demo
        }

        // start_demo_uniformRefiner_hex8_build_1
        {
          percept::PerceptMesh eMesh(3u);

          //unsigned p_size = eMesh.get_parallel_size();
          eMesh.open(input_files_loc+"hex_fixture.e");

          Hex8_Tet4_24 break_hex_to_tet(eMesh);

          int scalarDimension = 0; // a scalar
          stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

          eMesh.commit();

          UniformRefiner breaker(eMesh, break_hex_to_tet, proc_rank_field);
          breaker.doBreak();
          save_or_diff(eMesh, input_files_loc+"tet_fixture.e");
          save_or_diff(eMesh, input_files_loc+"tet_from_hex_fixture.e");
          // end_demo
        }
      }

      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Creates meshes for use in later tests - quad meshes with and without sidesets

      //TEST(unit_uniformRefiner, quad4_quad4_4_test_1)
      static void fixture_setup_1()
      {
        EXCEPTWATCH;

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            //const unsigned p_rank = stk::parallel_machine_rank( pm );
            //const unsigned p_size = stk::parallel_machine_size( pm );

            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double> fixture( pm , nx , ny, true);
            fixture.meta_data.commit();
            fixture.generate_mesh();

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data);
            eMesh.print_info("quad fixture", print_infoLevel);
            save_or_diff(eMesh, input_files_loc+"quad_fixture.e");
          }

        if (p_size <= 3)
          {
            //const unsigned p_rank = stk::parallel_machine_rank( pm );
            //const unsigned p_size = stk::parallel_machine_size( pm );

            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double> fixture( pm , nx , ny, false);
            fixture.meta_data.commit();
            fixture.generate_mesh();

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data);
            eMesh.print_info("quad fixture no sidesets", print_infoLevel);
            save_or_diff(eMesh, input_files_loc+"quad_fixture_no_sidesets.e");
          }
      }

      static void fixture_setup()
      {
        //std::cout << "tmp fixture_setup" << std::endl;
        static bool is_setup = false;
        if (is_setup) return;
        fixture_setup_0();
        fixture_setup_1();
        is_setup = true;
      }

      //=====================================================================================================================================================================================================
      //=====================================================================================================================================================================================================
      //=====================================================================================================================================================================================================

#define REPRO_ERROR_7539 0

#if REPRO_ERROR_7539
      TEST(unit1_uniformRefiner, stk_fixture)
      {
        typedef stk::mesh::Field<int> ProcIdFieldType;

        using Teuchos::RCP;
        using Teuchos::rcp;
        using Teuchos::rcpFromRef;

        int numprocs = stk::parallel_machine_size(MPI_COMM_WORLD);
        int rank = stk::parallel_machine_rank(MPI_COMM_WORLD);
        std::cout << "Running numprocs = " << numprocs << " rank = " << rank << std::endl;

        stk::mesh::fixtures::HexFixture hf(MPI_COMM_WORLD,3,3,3);
        ProcIdFieldType & processorIdField = hf.m_meta.declare_field<ProcIdFieldType>(stk::topology::ELEMENT_RANK, "PROC_ID");
        stk::mesh::put_field_on_mesh( processorIdField , stk::topology::ELEMENT_RANK, hf.m_meta.universal_part());

        percept::PerceptMesh eMesh(&hf.m_meta,&hf.m_bulk_data,false);
        percept::Hex8_Hex8_8 break_quad(eMesh);

        hf.m_meta.commit();
        hf.generate_mesh();

        const stk::mesh::BucketVector & buckets = hf.m_bulk_data.buckets(3);
        for(std::size_t i=0;i<buckets.size();++i) {
          stk::mesh::Bucket & b = *buckets[i];
          for(std::size_t j=0;j<b.size();++j) {
            stk::mesh::Entity element = b[j];
            // set processor rank
            int * procId = stk::mesh::field_data(processorIdField,element);
            procId[0] = hf.m_bulk_data.parallel_rank();
          }
        }

        eMesh.save_as("./tmp_hf1.e");
        percept::UniformRefiner breaker(eMesh, break_quad, &processorIdField);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();
      }

#endif

      TEST(unit1_uniformRefiner, stk_fixture_workaround)
      {
        if (1) return;
        typedef stk::mesh::Field<int> ProcIdFieldType;

        using Teuchos::RCP;
        using Teuchos::rcp;
        using Teuchos::rcpFromRef;

        int numprocs = stk::parallel_machine_size(MPI_COMM_WORLD);
        int rank = stk::parallel_machine_rank(MPI_COMM_WORLD);
        std::cout << "Running numprocs = " << numprocs << " rank = " << rank << std::endl;

        // local scope to make sure we don't re-use hf
        {
          stk::mesh::fixtures::HexFixture hf(MPI_COMM_WORLD,3,3,3);
          ProcIdFieldType & processorIdField = hf.m_meta.declare_field<ProcIdFieldType>(stk::topology::ELEMENT_RANK, "PROC_ID");
          stk::mesh::put_field_on_mesh( processorIdField , hf.m_meta.universal_part(), nullptr);

          // to get any output we must tell I/O about this part
          stk::io::put_io_part_attribute( hf.m_meta.universal_part());

          hf.m_meta.commit();
          hf.generate_mesh();

          const stk::mesh::BucketVector & buckets = hf.m_bulk_data.buckets(stk::topology::ELEMENT_RANK);
          for(std::size_t i=0;i<buckets.size();++i) {
            stk::mesh::Bucket & b = *buckets[i];
            for(std::size_t j=0;j<b.size();++j) {
              stk::mesh::Entity element = b[j];
              // set processor rank
              int * procId = stk::mesh::field_data(processorIdField,element);
              procId[0] = hf.m_bulk_data.parallel_rank();
            }
          }

          // save and then re-open - this is a workaround until MetaData is changed to natively support adaptivity
          //   by either initializing always to use rank-4 entities and/or allowing a re-init
          percept::PerceptMesh eMesh_TMP(&hf.m_meta,&hf.m_bulk_data,true);
          eMesh_TMP.save_as("./hex_init.e");
        }

        percept::PerceptMesh eMesh;
        eMesh.open("./hex_init.e");
        percept::Hex8_Hex8_8 break_quad(eMesh);
        eMesh.commit();

        stk::mesh::FieldBase& processorIdField_1 = *eMesh.get_field(stk::topology::ELEMENT_RANK, "PROC_ID");
        percept::UniformRefiner breaker(eMesh, break_quad, &processorIdField_1);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();
        eMesh.save_as("./hex_refined.e");
      }


      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================


#if 1

      /// Refine a quad mesh

      TEST(unit1_uniformRefiner, break_quad_to_quad_sierra_1_test)
      {
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 2)
          {
            const unsigned n = 2;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = 1 , ny = n;

            bool createEdgeSets = false;
            percept::QuadFixture<double > fixture( pm , nx , ny, createEdgeSets);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Quad4_Quad4_4 break_quad_to_quad_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);

            //             std::cout << "proc_rank_field rank= " << proc_rank_field->rank() << std::endl;
            //             std::cout << "proc_rank_field_edge rank= " << proc_rank_field_edge->rank() << std::endl;

            //fixture.meta_data.commit();
            eMesh.commit();

            if (0)
              {
                percept::GeometryVerifier gv(true);
                std::cout << "tmp GeometryVerifier= " << eMesh.get_bulk_data() << std::endl;
                bool igb = gv.isGeometryBad(*eMesh.get_bulk_data(), true);
                std::cout << "tmp isGeometryBad= " << igb << std::endl;
              }

            fixture.generate_mesh();

            UniformRefiner breaker(eMesh, break_quad_to_quad_4, proc_rank_field);
            breaker.setRemoveOldElements(false);
            breaker.doBreak();
            //MPI_Barrier( MPI_COMM_WORLD );

            //eMesh.dump_elements_compact();

            //MPI_Barrier( MPI_COMM_WORLD );
            //exit(123);


            // end_demo
          }

      }

      /// Refine a triangle mesh

      TEST(unit1_uniformRefiner, break_tri_to_tri_sierra_1_test)
      {
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 2)
          {
            const unsigned n = 2;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool createEdgeSets = false;
            percept::QuadFixture<double, shards::Triangle<3> > fixture( pm , nx , ny, createEdgeSets);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Tri3_Tri3_4 break_tri_to_tri_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);

            //             std::cout << "proc_rank_field rank= " << proc_rank_field->rank() << std::endl;
            //             std::cout << "proc_rank_field_edge rank= " << proc_rank_field_edge->rank() << std::endl;

            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            UniformRefiner breaker(eMesh, break_tri_to_tri_4, proc_rank_field);
            breaker.setRemoveOldElements(false);
            breaker.doBreak();
            eMesh.save_as("quad_ref.e");
            //eMesh.dump_elements_compact();

            //MPI_Barrier( MPI_COMM_WORLD );
            //exit(123);


            // end_demo
          }

      }

#endif


      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Refine quad elements
      /// uses the Sierra-ported tables from framework/{element,mesh_modification}
      TEST(unit_uniformRefiner, break_quad_to_quad_sierra)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            //const unsigned p_rank = stk::parallel_machine_rank( pm );
            //const unsigned p_size = stk::parallel_machine_size( pm );

            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double> fixture( pm , nx , ny, true);

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, false);

            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_quad_to_quad_4(eMesh);
            // FIXME
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            eMesh.print_info("quad mesh");

            UniformRefiner breaker(eMesh, break_quad_to_quad_4, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            breaker.doBreak();

            //!!eMesh, "./square_quad4_ref_sierra_out.e");
            // end_demo
          }

      }

      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Refine quad elements with beam elements for the "side sets"
      TEST(unit_uniformRefiner, break_quad_to_quad_sierra_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            //const unsigned p_rank = stk::parallel_machine_rank( pm );
            //const unsigned p_size = stk::parallel_machine_size( pm );

            const unsigned n = 2;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool debug_geom_side_sets_as_blocks = true;
            percept::QuadFixture<double> fixture( pm , nx , ny, true, debug_geom_side_sets_as_blocks);

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, false);

            //UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_pattern(eMesh);
            URP_Heterogeneous_3D break_pattern(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            fixture.generate_mesh();

            eMesh.print_info("quad mesh");
            eMesh.save_as("./quad_mesh_count_0.e");
            {
              std::vector<size_t> count ;
              stk::mesh::Selector selector(eMesh.get_fem_meta_data()->universal_part());
              stk::mesh::count_entities( selector, *eMesh.get_bulk_data(), count );

              std::cout << "{ Node = " << count[  0 ] ;
              std::cout << " Edge = " << count[  1 ] ;
              std::cout << " Face = " << count[  2 ] ;
              std::cout << " Elem = " << count[  3 ] ;
              std::cout << " }" << std::endl ;
            }

            UniformRefiner breaker(eMesh, break_pattern, proc_rank_field);
            breaker.setRemoveOldElements(true);
            breaker.doBreak();

            eMesh.save_as("./quad_mesh_count_1.e");

            {
              std::vector<size_t> count ;
              stk::mesh::Selector selector(eMesh.get_fem_meta_data()->universal_part());
              stk::mesh::count_entities( selector, *eMesh.get_bulk_data(), count );

              std::cout << "{ Node = " << count[  0 ] ;
              std::cout << " Edge = " << count[  1 ] ;
              std::cout << " Face = " << count[  2 ] ;
              std::cout << " Elem = " << count[  3 ] ;
              std::cout << " }" << std::endl ;
            }
            //exit(123);

            // end_demo
          }

      }

      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Create a triangle mesh using the QuadFixture with the option of breaking the quads into triangles
      /// Refine the triangle mesh, write the results.

      TEST(unit_uniformRefiner, break_tri_to_tri_sierra)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double, shards::Triangle<3> > fixture( pm , nx , ny, true);

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, false);

            //             UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_tri_to_tri_4(eMesh);
            //             // FIXME
            //             int scalarDimension = 0; // a scalar
            //             FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            eMesh.print_info("tri mesh");

            //             UniformRefiner breaker(eMesh, break_tri_to_tri_4, proc_rank_field);
            //             breaker.doBreak();

            save_or_diff(eMesh, input_files_loc+"quad_fixture_tri3.e");
            // end_demo
          }

      }

      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Refine a hex8 mesh
      TEST(unit_uniformRefiner, hex8_hex8_8_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex8_hex8_8_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();

        // generate a 4x4x(4*p_size) mesh
        std::string gmesh_spec = std::string("4x4x")+toString(4*p_size)+std::string("|bbox:0,0,0,1,1,1");
        eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));
        //eMesh.commit();
        //eMesh.reopen();

        Hex8_Hex8_8 break_hex_to_hex(eMesh);

        int scalarDimension = 0; // a scalar
        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();

        UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
        breaker.doBreak();
        // end_demo
      }

      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Create and write a wedge mesh using the WedgeFixture

      TEST(unit_uniformRefiner, wedge6_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_wedge6_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();
        if (p_size == 1)  // this fixture only works in serial mode
          {
            percept::WedgeFixture wedgeFixture;
            wedgeFixture.createMesh(MPI_COMM_WORLD,
                                    4, 3, 2,
                                    0, 1,
                                    0, 1,
                                    0, 1,
                                    std::string(input_files_loc+"swept-wedge_0.e") );
          }
      }


      //=============================================================================================================================================================
      //=============================================================================================================================================================
      //=============================================================================================================================================================

      /// Create a Beam mesh and enrich it

      TEST(unit1_uniformRefiner, beam_enrich)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size(pm);

        if (p_size <= 1)
          {
            // create the mesh
            {

              percept::BeamFixture mesh(pm, false);
              stk::io::put_io_part_attribute(  mesh.m_block_beam );
              mesh.m_metaData.commit();
              mesh.populate();

              bool isCommitted = true;
              percept::PerceptMesh em1(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);
              save_or_diff(em1, input_files_loc+"beam_enrich_0.e");

            }

            // enrich
            {
              percept::PerceptMesh eMesh(3u);
              eMesh.open(input_files_loc+"beam_enrich_0.e");
              //URP_Heterogeneous_3D break_pattern(eMesh);
              Beam2_Beam3_1 break_pattern(eMesh);
              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
              eMesh.commit();

              save_or_diff(eMesh, output_files_loc+"beam_enrich_0.e");

              eMesh.print_info("beam", print_infoLevel);

              UniformRefiner breaker(eMesh, break_pattern, proc_rank_field);
              //breaker.setRemoveOldElements(false);
              breaker.setIgnoreSideSets(true);
              breaker.doBreak();

              save_or_diff(eMesh, output_files_loc+"beam_enrich_1.e");

            }
          }
      }
      //=============================================================================
      //=============================================================================
      //=============================================================================

      /// Create a beam mesh and refine it

      TEST(unit1_uniformRefiner, beam_refine)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size(pm);

        if (p_size <= 1)
          {
            // create the mesh
            {

              percept::BeamFixture mesh(pm, false);
              stk::io::put_io_part_attribute(  mesh.m_block_beam );
              mesh.m_metaData.commit();
              mesh.populate();

              bool isCommitted = true;
              percept::PerceptMesh em1(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);
              save_or_diff(em1, input_files_loc+"beam_0.e");

            }

            // refine
            {
              percept::PerceptMesh eMesh(3u);
              eMesh.open(input_files_loc+"beam_0.e");
              //URP_Heterogeneous_3D break_pattern(eMesh);
              Beam2_Beam2_2 break_pattern(eMesh);
              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
              eMesh.commit();

              save_or_diff(eMesh, output_files_loc+"beam_0.e");

              eMesh.print_info("beam", print_infoLevel);

              UniformRefiner breaker(eMesh, break_pattern, proc_rank_field);
              //breaker.setRemoveOldElements(false);
              breaker.setIgnoreSideSets(true);
              breaker.doBreak();

              save_or_diff(eMesh, output_files_loc+"beam_1.e");

            }
          }
      }


      //======================================================================================================================
      //======================================================================================================================
      //===================== Table generation
      //======================================================================================================================

      /// This code generates C++ tables used by adapt - tables contain node numbering, parametric coordinates consistent
      ///    with Intrepid, and related information needed by UniformRefiner.  The generated code should be compared with
      ///    and merged into <adapt/sierra_element/GeneratedRefinementTable.hpp> as appropriate if there is a change
      ///    in the tables in that package, or an additional element type is added.
      /** This comment is from the generated code and tells how to bootstrap this process.
       * Bootstrapping this file: to create this file, run the regression test RegressionTestUniformRefiner.cpp :: generate_tables after putting in
       *   a dummy entry in ./sierra_element/GeneratedRefinementTable.hpp.  The run will produce a local file, generated_refinement_tables.hpp
       *   which can be checked against the gold copy of GeneratedRefinementTable.hpp, then copied over it.  Add a call below to generate the
       *   actual new table data.
       */


      TEST(unit1_uniformRefiner, generate_tables)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );
        Elem::StdMeshObjTopologies::bootstrap();

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1)
          {
            std::ofstream file("./generated_refinement_tables.hpp");

            file << "#ifndef STK_ADAPT_GENERATED_REFINEMENT_TABLES_HPP" << std::endl;
            file << "#define STK_ADAPT_GENERATED_REFINEMENT_TABLES_HPP" << std::endl;

            file <<
              "/**  New ref topo info \n"
              "*  ------------------\n"
              "*\n"
              "*  {Ord, Rnk-assoc, Ord-rnk-assoc, Ord-node-on-subcell, num-rnk-assoc, param-coord}\n"
              "*\n"
              "*   struct RefinementTopologyExtraEntry\n"
              "*   {\n"
              "*     unsigned ordinal_of_node;               // ordinal of node in the total list of nodes - corresponds to the shards node ordinal\n"
              "*     unsigned rank_of_subcell;               // rank of the subcell this node is associated with                                   \n"
              "*     unsigned ordinal_of_subcell;            // ordinal of the subcell in the shards numbering (e.g. edge # 3)\n"
              "*     unsigned ordinal_of_node_on_subcell;    // ordinal of the node on the subcell (whcih node it is on a subcell that has multiple nodes)\n"
              "*     unsigned num_nodes_on_subcell;          // how many nodes exist on the subcell                                                       \n"
              "*     double parametric_coordinates[3];\n"
              "*   };\n"
              "*       \n"
              "* Bootstrapping this file: to create this file, run the regression test RegressionTestUniformRefiner.cpp :: generate_tables after putting in\n"
              "*   a dummy entry in ./sierra_element/GeneratedRefinementTable.hpp.  The run will produce a local file, generated_refinement_tables.hpp \n"
              "*   which can be checked against the gold copy of GeneratedRefinementTable.hpp, then copied over it.  Add a call below to generate the \n"
              "*   actual new table data. \n"
              "*/\n\n"
                 << std::endl;

            // FIXME
#if !(defined(__PGI) && defined(USE_PGI_7_1_COMPILER_BUG_WORKAROUND))

            Line2_Line2_2            :: printRefinementTopoX_Table(file);

            Beam2_Beam2_2            :: printRefinementTopoX_Table(file);

            ShellLine2_ShellLine2_2  :: printRefinementTopoX_Table(file);
            ShellLine3_ShellLine3_2  :: printRefinementTopoX_Table(file);
            Quad4_Quad4_4            :: printRefinementTopoX_Table(file);
            Tri3_Tri3_4              :: printRefinementTopoX_Table(file);
            ShellTri3_ShellTri3_4    :: printRefinementTopoX_Table(file);
            ShellTri6_ShellTri6_4    :: printRefinementTopoX_Table(file);
            ShellQuad4_ShellQuad4_4  :: printRefinementTopoX_Table(file);
            ShellQuad8_ShellQuad8_4  :: printRefinementTopoX_Table(file);
            Tet4_Tet4_8              :: printRefinementTopoX_Table(file);
            Hex8_Hex8_8              :: printRefinementTopoX_Table(file);
            Wedge6_Wedge6_8          :: printRefinementTopoX_Table(file);
            Wedge15_Wedge15_8        :: printRefinementTopoX_Table(file);

            // Not supported by Sierra
            // Wedge18_Wedge18_8        :: printRefinementTopoX_Table(file);

            Line3_Line3_2            :: printRefinementTopoX_Table(file);
            Beam3_Beam3_2            :: printRefinementTopoX_Table(file);

            Tri6_Tri6_4              :: printRefinementTopoX_Table(file);
            Quad8_Quad8_4            :: printRefinementTopoX_Table(file);
            Quad9_Quad9_4            :: printRefinementTopoX_Table(file);
            Hex27_Hex27_8            :: printRefinementTopoX_Table(file);
            Hex20_Hex20_8            :: printRefinementTopoX_Table(file);
            Tet10_Tet10_8            :: printRefinementTopoX_Table(file);
#endif
            file << "#endif" << std::endl;
          }

      }


      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Code to generate Dot/Graphviz files representing the topology of element refinement based on the internal tables

      static void output_draw(std::string filename, std::string toFile)
      {
        std::ofstream file(filename.c_str());
        file << toFile;
      }

      TEST(unit_uniformRefiner, draw1)
      {
        fixture_setup();
        //std::cout << Quad4_Quad4_4::draw() << std::endl;
        std::string dir = "./";
        output_draw(dir+"quad4.dot", Quad4_Quad4_4::draw(true) );
        output_draw(dir+"tet4.dot",  Tet4_Tet4_8::draw() );
        output_draw(dir+"hex8.dot",  Hex8_Hex8_8::draw(true) );
        output_draw(dir+"hex27.dot",  Hex27_Hex27_8::draw(true, true) );
        output_draw(dir+"hex20.dot",  Hex20_Hex20_8::draw(true, true) );
        output_draw(dir+"wedge6.dot",  Wedge6_Wedge6_8::draw() );

        output_draw(dir+"quad9.dot", Quad9_Quad9_4::draw(true, true));
      }

      TEST(unit_uniformRefiner, draw)
      {
        fixture_setup();
        //std::cout << Quad4_Quad4_4::draw() << std::endl;
        std::string dir = "./";
        output_draw(dir+"quad4.dot", Quad4_Quad4_4::draw(true) );
        output_draw(dir+"tet4.dot",  Tet4_Tet4_8::draw() );
        output_draw(dir+"hex8.dot",  Hex8_Hex8_8::draw(true) );
        output_draw(dir+"wedge6.dot",  Wedge6_Wedge6_8::draw() );

        output_draw(dir+"quad9.dot", Quad9_Quad9_4::draw(true));

        // refine
#if 0
        std::cout << Line2_Line2_2::draw() << std::endl;
        std::cout << Tri3_Tri3_4::draw() << std::endl;
        std::cout << Tet4_Tet4_8::draw() << std::endl;
        std::cout << Hex8_Hex8_8::draw() << std::endl;
#endif

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Convert a quad mesh to triangles with 6 triangles per quad

      TEST(unit1_uniformRefiner, break_quad_to_tri_6)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_quad_to_tri_6
            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture.e");

            typedef  UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>, 6 > Quad4_Tri3_6;

            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>, 6 > break_quad_to_tri_6(eMesh);

            int scalarDimension = 0; // a scalar
            //         int vectorDimension = 3;

            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            eMesh.print_info("quad mesh");

            //UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>, 6 > break_quad_to_tri_6;
            UniformRefiner breaker(eMesh, break_quad_to_tri_6, proc_rank_field);
            //breaker.setIgnoreSideSets(true);
            //breaker.setRemoveOldElements(false);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"square_quad4_out.e");
            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Convert a quad mesh to triangles with 4 triangles per quad

      TEST(unit1_uniformRefiner, break_quad_to_tri_4)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {

            // start_demo_uniformRefiner_break_quad_to_tri_4
            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture.e");

            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>, 4, Specialization > break_quad_to_tri_4(eMesh);

            int scalarDimension = 0; // a scalar

            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            eMesh.print_info("quad mesh");

            //UniformRefinerPattern<shards::Quadrilateral<4>, shards::Triangle<3>, 6 > break_quad_to_tri_6;
            UniformRefiner breaker(eMesh, break_quad_to_tri_4, proc_rank_field);
            breaker.setRemoveOldElements(false);
            breaker.setIgnoreSideSets(true);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"square_quad4_tri3_4_out.e");
            // end_demo
          }
      }


      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad mesh using the "standalone" refinement pattern:
      ///      UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4 >
      /// This pattern is an example (like the convert-type patterns) showing how to write a new pattern with no dependencies
      //     on other (say tabular) data/info.

      TEST(unit1_uniformRefiner, break_quad_to_quad)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_quad_to_quad
            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture_no_sidesets.e");

            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4 > break_quad_to_quad_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
            eMesh.commit();

            eMesh.print_info("quad mesh");

            UniformRefiner breaker(eMesh, break_quad_to_quad_4, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            breaker.setIgnoreSideSets(true);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"square_quad4_ref_out.e");
            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad mesh
      /// uses the Sierra-ported tables from framework/{element,mesh_modification}

      TEST(unit1_uniformRefiner, break_quad_to_quad_sierra)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );
        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_quad_to_quad_sierra
            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture.e");

            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_quad_to_quad_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
            eMesh.commit();

            eMesh.print_info("quad mesh");

            UniformRefiner breaker(eMesh, break_quad_to_quad_4, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"square_quad4_ref_sierra_out.e");
            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad mesh with sidesets
      /// uses the Sierra-ported tables from framework/{element,mesh_modification}

      TEST(unit1_uniformRefiner, break_quad_to_quad_sierra_sidesets)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );

        if (p_size == 1 || p_size == 2)
          {
            // start_demo_uniformRefiner_break_quad_to_quad_sierra_sidesets
            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture.e");

            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_quad_to_quad_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
            eMesh.commit();

            eMesh.print_info("quad mesh");

            UniformRefiner breaker(eMesh, break_quad_to_quad_4, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            breaker.doBreak();

            eMesh.print_info("after refinement break_quad_to_quad_sierra_sidesets");

            save_or_diff(eMesh, output_files_loc+"quad_sidesets_sierra_out.e");
          }
        // end_demo
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Convert a hex mesh to tets using 24 tets per hex

      TEST(unit1_uniformRefiner, hex8_tet4_24_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex8_tet4_24_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();
        //unsigned p_rank = eMesh.get_rank();
        Util::setRank(eMesh.get_rank());

        std::string gmesh_spec = std::string("1x1x")+toString(p_size)+std::string("|bbox:0,0,0,1,1,"+toString(p_size) );
        eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));

        UniformRefinerPattern<shards::Hexahedron<8>, shards::Tetrahedron<4>, 24 > break_hex_to_tet(eMesh);

        int scalarDimension = 0; // a scalar
        //         int vectorDimension = 3;

        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
        //         eMesh.add_field("velocity", stk::mesh::Node, vectorDimension);
        //         eMesh.add_field("element_volume", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        eMesh.print_info();
        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex_tet_24_cube1x1x")+toString(p_size)+std::string("-orig.e"));

        UniformRefiner breaker(eMesh, break_hex_to_tet, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex_tet_24_cube1x1x")+toString(p_size)+std::string(".e"));

        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Convert a pyr mesh using 2 tets per pyr

      void test_refine_pyr5_tet4_2(unsigned nnodes, unsigned npyr, double coord[][3], stk::mesh::EntityIdVector *nid, const std::string& outfilename)
      {
        // sidesets in fixture don't work for second case below, turned off for now
        //PyramidFixture mesh(MPI_COMM_WORLD, false, false);
        HeterogeneousFixture mesh(MPI_COMM_WORLD, false, false);

        bool isCommitted = false;
        PerceptMesh eMesh(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);

        Pyramid5_Tet4_2 break_pyr_to_tet(eMesh);

        int scalarDimension = 0; // a scalar
        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        //mesh.init(nnodes, npyr, coord, nid);
        mesh.init(nnodes, coord, 0, NULL, 0, NULL, 0, NULL, npyr, nid);
        mesh.populate();

        eMesh.save_as(outfilename+".0.e");

        UniformRefiner breaker(eMesh, break_pyr_to_tet, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        eMesh.save_as(outfilename+".1.e");
      }

      TEST(unit1_uniformRefiner, pyr5_tet4_2)
      {
        {
          std::string outfilename("pyr5_tet4_2_2pyr_shared_tri_face");
          
          unsigned nnodes = 7;
          unsigned npyr = 2; 
          double coord[ 7 ][ 3 ] = {
            
            { 0 , 0 , -1 } , { 1 , 0 , -1 } ,  { 2 , 0 , -1 } ,
            
            { 0 , 1 , -1 } , { 1 , 1 , -1 } , { 2 , 1 , -1 } ,
            
            { 1 , 1 , -2 }
          };
         stk::mesh::EntityIdVector nid[2] {
           { 1 , 4 , 5 , 2 , 7 } ,
           { 2 , 5 , 6 , 3 , 7 } 
         };
          
          test_refine_pyr5_tet4_2(nnodes, npyr, coord, nid, outfilename);
        }

        {
          std::string outfilename("pyr5_tet4_2_2pyr_shared_quad_face");

          unsigned nnodes = 6;
          unsigned npyr = 2; 
          double coord[ 6 ][ 3 ] = {

            { 0 , 0 , 0 } , { 1 , 0 , 0 } ,
            
            { 1 , 1 , 0 } , { 0 , 1 , 0 } ,
            
            { 0 , 0 , 1 } , { 1 , 1 , -1 }
          };
          stk::mesh::EntityIdVector nid[2] {
            { 1 , 2 , 3 , 4 , 5 } ,
            { 4 , 3 , 2 , 1 , 6 } 
          };
          
          test_refine_pyr5_tet4_2(nnodes, npyr, coord, nid, outfilename);
        }
      }
      
      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      // test convert wedge6 into 3 tet4

      void test_refine_wedge6_tet4_3(unsigned nnodes, unsigned nwedge, double coord[][3], stk::mesh::EntityIdVector *nid, const std::string& outfilename)
      {
        // sidesets in fixture don't work for second case below, turned off for now
        HeterogeneousFixture mesh(MPI_COMM_WORLD, false, false);

        bool isCommitted = false;
        PerceptMesh eMesh(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);
        
        Wedge6_Tet4_3 break_wedge_to_tet(eMesh);

        int scalarDimension = 0; // a scalar
        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

        //eMesh.setProperty("FindValidCentroid_off", "true");
        eMesh.commit();
        mesh.init(nnodes, coord, 0, NULL, 0, NULL, nwedge, nid, 0, NULL);
        mesh.populate();

        eMesh.save_as(outfilename+".0.e");

        UniformRefiner breaker(eMesh, break_wedge_to_tet, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        eMesh.save_as(outfilename+".1.e");
      }

      TEST(unit1_uniformRefiner, wedge6_tet4_3)
      {
        {
          std::string outfilename("wedge5_tet4_3_1wedge");
          
          unsigned nnodes = 6;
          unsigned nwedge = 1; 
          double coord[ 6 ][ 3 ] = {
            
            { 0 , 0 , 0 } , { 1 , 0 , 0 } ,  { 0 , 1 , 0 } ,
            
            { 0 , 0 , 1 } , { 1 , 0 , 1 } ,  { 0 , 1 , 1 }

          };
          stk::mesh::EntityIdVector nid[1] {
            { 6 , 5 , 4 , 3 , 2 , 1 }
          };
          
          test_refine_wedge6_tet4_3(nnodes, nwedge, coord, nid, outfilename);
        }
        
        {
          std::string outfilename("wedge5_tet4_3_2wedge_shared_quad_face");
          
          unsigned nnodes = 8;
          unsigned nwedge = 2; 
          double coord[ 8 ][ 3 ] = {
            
            { 0 , 0 , 0 } , { 1 , 0 , 0 } ,  { 0 , 1 , 0 } ,
            
            { 0 , 0 , 1 } , { 1 , 0 , 1 } ,  { 0 , 1 , 1 } ,
            
            { -1 , 1 , 0 } , { -1 , 1 , 1 }
          };
          stk::mesh::EntityIdVector nid[2] {
            { 3 , 1 , 2 , 6 , 4 , 5 } ,
            { 3 , 7 , 1 , 6 , 8 , 4 } 
          };
          
          test_refine_wedge6_tet4_3(nnodes, nwedge, coord, nid, outfilename);
        }
        
        {
          std::string outfilename("wedge5_tet4_3_2wedge_shared_tri_face");
          
          unsigned nnodes = 9;
          unsigned nwedge = 2; 
          double coord[ 9 ][ 3 ] = {
            
            { 0 , 0 , -1 } , { 1 , 0 , -1 } ,  { 0 , 1 , -1 } ,

            { 0 , 0 , 0 } , { 1 , 0 , 0 } ,  { 0 , 1 , 0 } ,
            
            { 0 , 0 , 1 } , { 1 , 0 , 1 } ,  { 0 , 1 , 1 }             
          };
          stk::mesh::EntityIdVector nid[2] {
            { 5 , 4 , 6 , 2 , 1 , 3 } ,
            { 9 , 8 , 7 , 6 , 5 , 4 } 
          };
          
          test_refine_wedge6_tet4_3(nnodes, nwedge, coord, nid, outfilename);
        }
        
        {
          std::string outfilename("wedge5_tet4_3_3wedge_shared_quad_faces");
          
          unsigned nnodes = 10;
          unsigned nwedge = 3; 
          double coord[ 10 ][ 3 ] = {
            
            { 0 , 0 , 0 } , { 1 , 0 , 0 } ,  { 0 , 1 , 0 } ,
            
            { 0 , 0 , 1 } , { 1 , 0 , 1 } ,  { 0 , 1 , 1 } ,
            
            { -1 , 1 , 0 } , { -1 , 1 , 1 } ,
            
            { 1 , -1 , 0 } , { 1 , -1 , 1 }
          };
          stk::mesh::EntityIdVector nid[3] {
            { 2 , 3 , 1 , 5 , 6 , 4 } ,
            { 7 , 1 , 3 , 8 , 4 , 6 } ,
            { 1 , 9 , 2 , 4 , 10 , 5 }
          };
          
          test_refine_wedge6_tet4_3(nnodes, nwedge, coord, nid, outfilename);
        }
        
       }

      void test_convert_pyramid5_wedge6_tet4(unsigned nnodes, 
                                             unsigned nwedge, stk::mesh::EntityIdVector *nid_wedge, 
                                             unsigned npyramid, stk::mesh::EntityIdVector *nid_pyramid, 
                                             double coord[][3], const std::string& outfilename)
      {
        // sidesets in fixture don't work for second case below, turned off for now
        HeterogeneousFixture mesh(MPI_COMM_WORLD, false, false);

        bool isCommitted = false;
        PerceptMesh eMesh(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);
        
        Hex8_Wedge6_Pyramid5_Tet4 break_pattern(eMesh);

        int scalarDimension = 0; // a scalar
        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        mesh.init(nnodes, coord, 0, NULL, 0, NULL, nwedge, nid_wedge, npyramid, nid_pyramid);
        mesh.populate();

        eMesh.save_as(outfilename+".0.e");

        UniformRefiner breaker(eMesh, break_pattern, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        eMesh.save_as(outfilename+".1.e");
      }

      TEST(unit1_uniformRefiner, convert_pyramid5_wedge6_tet4)
      {
        {
          std::string outfilename("convert_pyramid5_wedge5_tet4_1_2");
          
          unsigned nnodes = 10;
          unsigned nwedge = 2; 
          unsigned npyramid = 1; 
          double coord[ 10 ][ 3 ] = {
            
            { 0 , 0 , -1 } , { 1 , 0 , -1 } ,  { 0 , 1 , -1 } ,

            { 0 , 0 , 0 } , { 1 , 0 , 0 } ,  { 0 , 1 , 0 } ,
            
            { 0 , 0 , 1 } , { 1 , 0 , 1 } ,  { 0 , 1 , 1 } ,

            { -1 , 0.5 , 0.5 }
          };
          stk::mesh::EntityIdVector nid_wedge[2] {
            { 5 , 4 , 6 , 2 , 1 , 3 } ,
            { 9 , 8 , 7 , 6 , 5 , 4 } 
          };
          stk::mesh::EntityIdVector nid_pyramid[1] {
            { 4 , 7 , 9 , 6 , 10 }
          };
          
          test_convert_pyramid5_wedge6_tet4(nnodes, nwedge, nid_wedge, npyramid, nid_pyramid, coord, outfilename);
        }

       }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Convert a hex mesh using 6 tets per hex

      TEST(unit1_uniformRefiner, hex8_tet4_6_12_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex8_tet4_6_12_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();

        std::string gmesh_spec = std::string("1x1x")+toString(p_size)+std::string("|bbox:0,0,0,1,1,"+toString(p_size) );
        eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));

        Hex8_Tet4_6_12 break_hex_to_tet(eMesh);

        int scalarDimension = 0; // a scalar
        //         int vectorDimension = 3;

        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
        //         eMesh.add_field("velocity", stk::mesh::Node, vectorDimension);
        //         eMesh.add_field("element_volume", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        eMesh.print_info();
        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex_tet_6_12_cube1x1x")+toString(p_size)+std::string("-orig.e"));

        UniformRefiner breaker(eMesh, break_hex_to_tet, proc_rank_field);
        breaker.setRemoveOldElements(true);
        //breaker.setIgnoreSideSets(true);

        breaker.doBreak();

        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex_tet_6_12_cube1x1x")+toString(p_size)+std::string(".e"));

        // end_demo
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Convert a hex mesh using 6 tets per hex

      TEST(unit1_uniformRefiner, hex8_tet4_6_12_2)
      {
        fixture_setup();

        EXCEPTWATCH;

        // start_demo_uniformRefiner_hex8_tet4_6_12_2
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            percept::PerceptMesh eMesh(3u);

            eMesh.open(input_files_loc+"hex_fixture.e");

            Hex8_Tet4_6_12 break_hex_to_tet(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            //eMesh.print_info("test",2);
            eMesh.print_info();
            save_or_diff(eMesh, output_files_loc+"hex8_tet4_6_12_0.e");

            UniformRefiner breaker(eMesh, break_hex_to_tet, proc_rank_field);
            //breaker.setIgnoreSideSets(true);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"hex8_tet4_6_12_1.e");

            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad mesh

      TEST(unit1_uniformRefiner, quad4_quad4_4_test_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            //const unsigned p_rank = stk::parallel_machine_rank( pm );
            //const unsigned p_size = stk::parallel_machine_size( pm );

            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double> fixture( pm , nx , ny, true);
            fixture.meta_data.commit();
            fixture.generate_mesh();

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data);
            eMesh.print_info("quad fixture");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_test_1.e");
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad mesh; test the multiple refinement feature

      TEST(unit1_uniformRefiner, break_quad_to_quad_sierra_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        bool doGenSideSets = true;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double> fixture( pm , nx , ny, doGenSideSets);

            // Adopt the meta/bulk data
            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            eMesh.commit();

            fixture.generate_mesh();

            //eMesh.print_info("quad mesh");

            save_or_diff(eMesh, output_files_loc+"quad_fixture_0.e");
            save_or_diff(eMesh, input_files_loc+"quad_fixture_0.e");
            eMesh.close();

            for (int iBreak = 0; iBreak < 2; iBreak++)
              {
                std::cout << "\n\n\n ================ tmp Refine Pass = " << iBreak << std::endl;

                percept::PerceptMesh eMesh1(2);
                std::string fileName = std::string(input_files_loc+"quad_fixture_")+toString(iBreak)+std::string(".e");
                eMesh1.open(fileName);
                UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_quad_to_quad_4(eMesh1);
                int scalarDimension = 0; // a scalar
                stk::mesh::FieldBase* proc_rank_field = eMesh1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
                eMesh1.commit();

                //                 if (iBreak != 0)
                //                   proc_rank_field = eMesh1.get_field(stk::topology::ELEMENT_RANK, "proc_rank");

                UniformRefiner breaker(eMesh1, break_quad_to_quad_4, proc_rank_field);

                //breaker.setRemoveOldElements(false);
                breaker.doBreak();
                std::string fileName1 = std::string(output_files_loc+"quad_fixture_")+toString( iBreak + 1 )+std::string(".e");
                std::string fileName2 = std::string(input_files_loc+"quad_fixture_")+toString( iBreak + 1 )+std::string(".e");

                //eMesh1.print_info("quad_fixture_1.e");

                save_or_diff(eMesh1, fileName2);
                save_or_diff(eMesh1, fileName1);
                eMesh1.close();

                if (0 && iBreak==0)
                  {
                    percept::PerceptMesh e1(2);
                    std::cout << "\n\n\n ================ tmp eMesh1.open_read_only(quad_fixture_1.e) \n\n\n " << std::endl;
                    e1.open_read_only(input_files_loc+"quad_fixture_1.e");
                    e1.print_info("quad_fixture_1_read.e");
                    e1.close();
                  }
              }

            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad mesh; test the multiple refinement feature

      TEST(unit1_uniformRefiner, break_quad_to_quad_sierra_2)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool doGenSideSets = true;
            percept::QuadFixture<double> fixture( pm , nx , ny, doGenSideSets);

            // Adopt the meta/bulk data
            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            eMesh.commit();

            fixture.generate_mesh();

            //eMesh.print_info("quad mesh");

            save_or_diff(eMesh, output_files_loc+"quad_fixture_mbreak_0.e");
            save_or_diff(eMesh, input_files_loc+"quad_fixture_mbreak_0.e");
            eMesh.close();


            percept::PerceptMesh eMesh1(2);
            std::string fileName = std::string(input_files_loc+"quad_fixture_mbreak_0.e");
            eMesh1.open(fileName);
            UniformRefinerPattern<shards::Quadrilateral<4>, shards::Quadrilateral<4>, 4, SierraPort > break_quad_to_quad_4(eMesh1);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
            eMesh1.commit();

            UniformRefiner breaker(eMesh1, break_quad_to_quad_4, proc_rank_field);

            for (int iBreak = 0; iBreak < 2; iBreak++)
              {
                std::cout << "\n\n\n ================ tmp Refine Pass = " << iBreak << std::endl;

                breaker.doBreak();
                std::string fileName1 = std::string(output_files_loc+"quad_fixture_mbreak_")+toString(iBreak+1)+std::string(".e");
                save_or_diff(eMesh1, fileName1);
              }

            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a quad mesh (convert linear Quad4 elements to quadratic Quad9)

      TEST(unit1_uniformRefiner, break_quad4_to_quad9)
      {
        fixture_setup();
        EXCEPTWATCH;


        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool doGenSideSets = true;
            percept::QuadFixture<double> fixture( pm , nx , ny, doGenSideSets);

            // Adopt the meta/bulk data
            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Quad4_Quad9_1 break_quad4_to_quad9_1(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            fixture.generate_mesh();

            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad9_0.e");

            UniformRefiner breaker(eMesh, break_quad4_to_quad9_1, proc_rank_field);

            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad9_1.e");

            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a quad mesh (convert linear Quad4 elements to serendepity Quad8)

      TEST(unit1_uniformRefiner, break_quad4_to_quad8)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            const unsigned nx = n , ny = n;

            bool doGenSideSets = true;
            percept::QuadFixture<double> fixture( pm , nx , ny, doGenSideSets);

            // Adopt the meta/bulk data
            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Quad4_Quad8_1 break_quad4_to_quad8_1(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            fixture.generate_mesh();

            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad8_0.e");

            UniformRefiner breaker(eMesh, break_quad4_to_quad8_1, proc_rank_field);

            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad8_1.e");
            save_or_diff(eMesh, input_files_loc+"quad_fixture_quad8_quad8_0.e");

            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quad8/serendepity mesh

      TEST(unit1_uniformRefiner, break_quad8_to_quad8)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture_quad8_quad8_0.e");

            Quad8_Quad8_4 break_quad8_to_quad8_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            UniformRefiner breaker(eMesh, break_quad8_to_quad8_4, proc_rank_field);
            breaker.setIgnoreSideSets(false);

            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad8_quad8_1.e");

          }
      }


      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a quad4 mesh to quad9 then refine it

      TEST(unit1_uniformRefiner, break_quad4_to_quad9_to_quad9_0)
      {
        fixture_setup();
        EXCEPTWATCH;

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        bool doGenSideSets = false;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 1)
          {
            {
              const unsigned n = 1;
              //const unsigned nx = n , ny = n , nz = p_size*n ;
              const unsigned nx = n , ny = n;

              percept::QuadFixture<double> fixture( pm , nx , ny, doGenSideSets);

              // Adopt the meta/bulk data
              bool isCommitted = false;
              percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

              Quad4_Quad9_1 break_quad4_to_quad9_1(eMesh);
              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

              eMesh.commit();

              fixture.generate_mesh();


              UniformRefiner breaker(eMesh, break_quad4_to_quad9_1, proc_rank_field);

              breaker.doBreak();
              save_or_diff(eMesh, input_files_loc+"quad_1x1_quad9_quad9_0.e");
            }

            {
              percept::PerceptMesh em1(2);
              em1.open(input_files_loc+"quad_1x1_quad9_quad9_0.e");
              Quad9_Quad9_4 break_q9_q9(em1);
              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = em1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

              em1.commit();


              UniformRefiner breaker(em1, break_q9_q9, proc_rank_field);
              breaker.setIgnoreSideSets(!doGenSideSets);

              breaker.doBreak();
              save_or_diff(em1, output_files_loc+"quad_1x1_quad9_quad9_1.e");

            }
            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a quad4 mesh to quad9 then refine it

      TEST(unit1_uniformRefiner, break_quad4_to_quad9_to_quad9)
      {
        fixture_setup();
        EXCEPTWATCH;

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        bool doGenSideSets = true;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        //if (p_size == 1 || p_size == 3)
        if (p_size <= 3)
          {
            {
              //FIXME const unsigned n = 12;
              const unsigned n = 2;
              //const unsigned nx = n , ny = n , nz = p_size*n ;
              const unsigned nx = n , ny = n;

              percept::QuadFixture<double> fixture( pm , nx , ny, doGenSideSets);

              // Adopt the meta/bulk data
              bool isCommitted = false;
              percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

              Quad4_Quad9_1 break_quad4_to_quad9_1(eMesh);
              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

              eMesh.commit();

              fixture.generate_mesh();


              UniformRefiner breaker(eMesh, break_quad4_to_quad9_1, proc_rank_field);
              std::cout << "break_quad4_to_quad9_1.fixSurfaceAndEdgeSetNamesMap().size()= "
                        << break_quad4_to_quad9_1.fixSurfaceAndEdgeSetNamesMap().size() << std::endl;

              breaker.doBreak();
              save_or_diff(eMesh, input_files_loc+"quad_fixture_quad9_quad9_0.e");
              //eMesh.print_info("quad_fixture_quad9_quad9_0.e", 2);
            }

            {
              percept::PerceptMesh em1(2);
              em1.open(input_files_loc+"quad_fixture_quad9_quad9_0.e");
              Quad9_Quad9_4 break_q9_q9(em1);
              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = em1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

              em1.commit();


              UniformRefiner breaker(em1, break_q9_q9, proc_rank_field);
              breaker.setIgnoreSideSets(!doGenSideSets);

              breaker.doBreak();
              save_or_diff(em1, output_files_loc+"quad_fixture_quad9_quad9_1.e");

            }
            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a triangle mesh

      TEST(unit1_uniformRefiner, break_tri_to_tri_sierra_0)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            percept::QuadFixture<double, shards::Triangle<3> > fixture( pm , nx , ny, true);

            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, false);

            eMesh.commit();

            fixture.generate_mesh();

            eMesh.print_info("tri mesh");

            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3.e");
            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a triangle mesh

      TEST(unit1_uniformRefiner, break_tri_to_tri_sierra_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool createEdgeSets = true;
            percept::QuadFixture<double, shards::Triangle<3> > fixture( pm , nx , ny, createEdgeSets);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Tri3_Tri3_4 break_tri_to_tri_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);

            //             std::cout << "proc_rank_field rank= " << proc_rank_field->rank() << std::endl;
            //             std::cout << "proc_rank_field_edge rank= " << proc_rank_field_edge->rank() << std::endl;

            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            //eMesh.print_info("tri mesh", 5);
            eMesh.print_info("tri mesh");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3_0.e");

            UniformRefiner breaker(eMesh, break_tri_to_tri_4, proc_rank_field);
            breaker.doBreak();

            //eMesh.print_info("tri mesh refined", 5);
            eMesh.print_info("tri mesh refined");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3_1.e");

            if (0)
              {
                percept::PerceptMesh e1(2);
                e1.open_read_only(input_files_loc+"quad_fixture_tri3_1.e");
                e1.print_info("after read", 3);
              }
            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

#if defined ( STK_PERCEPT_HAS_GEOMETRY )

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a triangle mesh with spacing
      TEST(unit1_uniformRefiner, break_quad_to_quad_sierra_spc)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool createEdgeSets = true;
            percept::QuadFixture<double > fixture( pm , nx , ny, createEdgeSets);
            fixture.set_bounding_box(0,1,0,1);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Quad4_Quad4_4 break_quad_to_quad_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);

            //             std::cout << "proc_rank_field rank= " << proc_rank_field->rank() << std::endl;
            //             std::cout << "proc_rank_field_edge rank= " << proc_rank_field_edge->rank() << std::endl;

            eMesh.add_spacing_fields();
            eMesh.set_respect_spacing(true);
            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            const stk::mesh::BucketVector & buckets = eMesh.get_bulk_data()->buckets( stk::topology::NODE_RANK );

            for ( stk::mesh::BucketVector::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
              {
                  {
                    stk::mesh::Bucket & bucket = **k ;

                    const unsigned num_elements_in_bucket = bucket.size();

                    for (unsigned iEntity = 0; iEntity < num_elements_in_bucket; iEntity++)
                      {
                        stk::mesh::Entity node = bucket[iEntity];

                        double * data = stk::mesh::field_data( *eMesh.get_coordinates_field() , node );
                        double iy = data[1]; // /double(nele);
                        iy = iy*iy;
                        data[1] = iy; // *double(nele);
                      }
                  }
              }
#if 0
            Math::Matrix rmz = Math::rotationMatrix(2, 30);
            eMesh.transform_mesh(rmz);
#endif

            SpacingFieldUtil sfu(eMesh);
            sfu.compute_spacing_field();

            //eMesh.print_info("quad mesh", 5);
            eMesh.print_info("quad mesh");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad3_spc_0.e");

            UniformRefiner breaker(eMesh, break_quad_to_quad_4, proc_rank_field);
            breaker.doBreak();

            //eMesh.print_info("quad mesh refined", 5);
            eMesh.print_info("quad mesh refined");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_quad3_spc_1.e");

            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a triangle mesh with spacing
      TEST(unit1_uniformRefiner, break_tri_to_tri_sierra_spc)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool createEdgeSets = true;
            percept::QuadFixture<double, shards::Triangle<3> > fixture( pm , nx , ny, createEdgeSets);
            fixture.set_bounding_box(0,1,0,1);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Tri3_Tri3_4 break_tri_to_tri_4(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);

            //             std::cout << "proc_rank_field rank= " << proc_rank_field->rank() << std::endl;
            //             std::cout << "proc_rank_field_edge rank= " << proc_rank_field_edge->rank() << std::endl;

            eMesh.add_spacing_fields();
            eMesh.set_respect_spacing(true);
            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            const stk::mesh::BucketVector & buckets = eMesh.get_bulk_data()->buckets( stk::topology::NODE_RANK );

            for ( stk::mesh::BucketVector::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
              {
                  {
                    stk::mesh::Bucket & bucket = **k ;

                    const unsigned num_elements_in_bucket = bucket.size();

                    for (unsigned iEntity = 0; iEntity < num_elements_in_bucket; iEntity++)
                      {
                        stk::mesh::Entity node = bucket[iEntity];

                        double * data = stk::mesh::field_data( *eMesh.get_coordinates_field() , node );
                        double iy = data[1]; // /double(nele);
                        iy = iy*iy;
                        data[1] = iy; // *double(nele);
                      }
                  }
              }
#if 1
            Math::Matrix rmz = Math::rotationMatrix(2, 30);
            eMesh.transform_mesh(rmz);
#endif

            SpacingFieldUtil sfu(eMesh);
            sfu.compute_spacing_field();

            //eMesh.print_info("tri mesh", 5);
            eMesh.print_info("tri mesh");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3_spc_0.e");

            UniformRefiner breaker(eMesh, break_tri_to_tri_4, proc_rank_field);
            breaker.doBreak();

            //eMesh.print_info("tri mesh refined", 5);
            eMesh.print_info("tri mesh refined");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3_spc_1.e");

            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      // A cube with an indented bump on the bottom

      TEST(unit1_uniformRefiner, hex_4)
      {
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        MPI_Barrier( MPI_COMM_WORLD );

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            unsigned n = 12;
            std::string gmesh_spec = toString(n)+"x"+toString(n)+"x"+toString(n)+std::string("|bbox:0,0,0,1,1,1|sideset:xXyYzZ");
            PerceptMesh eMesh(3);
            eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));
            Hex8_Hex8_8 break_hex_to_hex(eMesh);
            eMesh.add_spacing_fields();
            eMesh.set_respect_spacing(true);
            eMesh.commit();

            const stk::mesh::BucketVector & buckets = eMesh.get_bulk_data()->buckets( stk::topology::NODE_RANK );

            // cluster the mesh towards the bump
            for ( stk::mesh::BucketVector::const_iterator k = buckets.begin() ; k != buckets.end() ; ++k )
              {
                  {
                    stk::mesh::Bucket & bucket = **k ;

                    const unsigned num_elements_in_bucket = bucket.size();

                    for (unsigned iEntity = 0; iEntity < num_elements_in_bucket; iEntity++)
                      {
                        stk::mesh::Entity entity = bucket[iEntity];

                        double * data = stk::mesh::field_data( *eMesh.get_coordinates_field() , entity );
                        data[2] = data[2]*data[2];
                      }
                  }
              }
            SpacingFieldUtil sfu(eMesh);
            sfu.compute_spacing_field();

            save_or_diff(eMesh, output_files_loc+"hex_4_spc.0.e");

            UniformRefiner breaker(eMesh, break_hex_to_hex, 0);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"hex_4_spc.1.e");

          }
      }

#endif
      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a triangle mesh

      TEST(unit1_uniformRefiner, break_tri3_to_tri6_sierra)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            const unsigned n = 12;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = n , ny = n;

            bool createEdgeSets = true;
            percept::QuadFixture<double, shards::Triangle<3> > fixture( pm , nx , ny, createEdgeSets);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            Tri3_Tri6_1 break_tri3_to_tri6(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);

            //             std::cout << "proc_rank_field rank= " << proc_rank_field->rank() << std::endl;
            //             std::cout << "proc_rank_field_edge rank= " << proc_rank_field_edge->rank() << std::endl;

            //fixture.meta_data.commit();
            eMesh.commit();

            fixture.generate_mesh();

            //eMesh.print_info("tri mesh", 5);
            eMesh.print_info("tri mesh tri6");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3_tri6_0.e");

            UniformRefiner breaker(eMesh, break_tri3_to_tri6, proc_rank_field);
            breaker.doBreak();

            //eMesh.print_info("tri mesh refined", 5);
            eMesh.print_info("tri mesh enriched");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri3_tri6_1.e");
            save_or_diff(eMesh, input_files_loc+"quad_fixture_tri6_tri6_0.e");

            if (0)
              {
                percept::PerceptMesh e1(2);
                e1.open_read_only(input_files_loc+"quad_fixture_tri3_1.e");
                e1.print_info("after read", 3);
              }
            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a triangle mesh then refine it

      TEST(unit1_uniformRefiner, break_tri3_to_tri6_to_tri6_sierra)
      {
        fixture_setup();
        EXCEPTWATCH;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size <= 3)
          {
            // start_demo_break_tri3_to_tri6_to_tri6_sierra

            percept::PerceptMesh eMesh(2u);
            eMesh.open(input_files_loc+"quad_fixture_tri6_tri6_0.e");

            Tri6_Tri6_4 break_tri6_to_tri6(eMesh);
            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            //stk::mesh::FieldBase* proc_rank_field_edge =
            eMesh.add_field("proc_rank_edge", eMesh.edge_rank(), scalarDimension);
            eMesh.commit();

            eMesh.print_info("tri mesh tri6");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri6_tri6_0.e");

            UniformRefiner breaker(eMesh, break_tri6_to_tri6, proc_rank_field);
            breaker.doBreak();

            eMesh.print_info("tri mesh refined");
            save_or_diff(eMesh, output_files_loc+"quad_fixture_tri6_tri6_1.e");
            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a linear tet mesh

      TEST(unit1_uniformRefiner, break_tet4_tet4_0)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_tet4_tet4_0
            percept::PerceptMesh eMesh(3u);
            eMesh.open_read_only(input_files_loc+"tet_from_hex_fixture.e");
            save_or_diff(eMesh, input_files_loc+"tet_from_hex_fixture_0.e");
            // end_demo

          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a linear tet mesh

      TEST(unit1_uniformRefiner, break_tet4_tet4_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_tet4_tet4_1
            percept::PerceptMesh eMesh(3u);
            eMesh.open(input_files_loc+"tet_from_hex_fixture_0.e");

            Tet4_Tet4_8 break_tet_tet(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            eMesh.print_info("tet mesh");

            UniformRefiner breaker(eMesh, break_tet_tet, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            //breaker.setIgnoreSideSets(true);
            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"tet4_refined_1.e");

            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"tet4_refined_2.e");
            // end_demo

          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear tet mesh

      TEST(unit1_uniformRefiner, break_tet4_tet10_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_tet4_tet10_1
            percept::PerceptMesh eMesh(3u);
            eMesh.open(input_files_loc+"tet_from_hex_fixture_0.e");

            Tet4_Tet10_1 break_tet_tet(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            eMesh.print_info("tet mesh");

            UniformRefiner breaker(eMesh, break_tet_tet, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            //breaker.setIgnoreSideSets(true);
            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"tet10_1.e");
            // end_demo


          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear tet mesh then refine it

      TEST(unit1_uniformRefiner, break_tet4_tet10_tet10_1)
      {
        fixture_setup();
        // FIXME
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_tet4_tet10_tet10_1
            percept::PerceptMesh eMesh(3u);
            eMesh.open(input_files_loc+"tet_from_hex_fixture_0.e");

            Tet4_Tet10_1 break_tet_tet(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            eMesh.print_info("tet mesh");

            UniformRefiner breaker(eMesh, break_tet_tet, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            //breaker.setIgnoreSideSets(true);
            breaker.doBreak();
            save_or_diff(eMesh, input_files_loc+"tet10_1.e");
            eMesh.print_info("tet10_1");
            // end_demo

          }

        if (p_size == 1 || p_size == 3)
          {
            // start_demo_uniformRefiner_break_tet4_tet10_tet10_2
            percept::PerceptMesh eMesh(3u);
            eMesh.open(input_files_loc+"tet10_1.e");

            Tet10_Tet10_8 break_tet_tet(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            //eMesh.print_info("tet mesh");

            UniformRefiner breaker(eMesh, break_tet_tet, proc_rank_field);
            //breaker.setRemoveOldElements(false);
            //breaker.setIgnoreSideSets(true);
            //breaker.doBreak();

            unsigned numRefines = 1;
            for (unsigned iBreak = 0; iBreak < numRefines; iBreak++)
              {
                breaker.doBreak();
              }

            save_or_diff(eMesh, output_files_loc+"tet10_tet10_"+toString(numRefines)+".e");
            // end_demo


          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a linear hex mesh

      TEST(unit1_uniformRefiner, hex8_hex8_8_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex8_hex8_8_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();

        std::string gmesh_spec = std::string("1x1x")+toString(p_size)+std::string("|bbox:0,0,0,1,1,"+toString(p_size) );
        eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));

        Hex8_Hex8_8 break_hex_to_hex(eMesh);

        int scalarDimension = 0; // a scalar
        //         int vectorDimension = 3;

        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
        //         eMesh.add_field("velocity", stk::mesh::Node, vectorDimension);
        //         eMesh.add_field("element_volume", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        eMesh.print_info();
        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex_hex_cube1x1x")+toString(p_size)+std::string("-orig.e"));

        UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex_hex_cube1x1x")+toString(p_size)+std::string(".e"));

        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a linear hex mesh

      TEST(unit1_uniformRefiner, hex8_hex8_8_2)
      {
        fixture_setup();
        EXCEPTWATCH;

        // start_demo_uniformRefiner_hex8_hex8_8_2
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            percept::PerceptMesh eMesh(3u);

            eMesh.open(input_files_loc+"hex_fixture.e");

            Hex8_Hex8_8 break_hex_to_hex(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            eMesh.print_info();
            save_or_diff(eMesh, output_files_loc+"hex8_0.e");

            UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"hex8_1.e");

            breaker.doBreak();
            save_or_diff(eMesh, output_files_loc+"hex8_2.e");

            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear hex mesh

      TEST(unit1_uniformRefiner, hex8_hex27_1_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex8_hex27_1_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();

        std::string gmesh_spec = std::string("1x1x")+toString(p_size)+std::string("|bbox:0,0,0,1,1,"+toString(p_size) );
        eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));

        Hex8_Hex27_1 break_hex_to_hex(eMesh);

        int scalarDimension = 0; // a scalar
        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        eMesh.print_info();
        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex8_hex27_cube1x1x")+toString(p_size)+std::string("-orig.e"));

        UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex8_hex27_cube1x1x")+toString(p_size)+std::string(".e"));

        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear hex mesh

      TEST(unit1_uniformRefiner, hex8_hex27_1_2)
      {
        fixture_setup();
        EXCEPTWATCH;

        // start_demo_uniformRefiner_hex8_hex27_1_2
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            percept::PerceptMesh eMesh(3u);

            eMesh.open(input_files_loc+"hex_fixture.e");

            Hex8_Hex27_1 break_hex_to_hex(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            eMesh.print_info();
            save_or_diff(eMesh, output_files_loc+"hex27_0.e");

            UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"hex27_1.e");


            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear hex mesh to serendepity hex20 elements

      TEST(unit1_uniformRefiner, hex8_hex20_1_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex8_hex20_1_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();

        std::string gmesh_spec = std::string("1x1x")+toString(p_size)+std::string("|bbox:0,0,0,1,1,"+toString(p_size) );
        eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));

        Hex8_Hex20_1 break_hex_to_hex(eMesh);

        int scalarDimension = 0; // a scalar
        stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

        eMesh.commit();
        eMesh.print_info();
        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex8_hex20_cube1x1x")+toString(p_size)+std::string("-orig.e"));

        UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
        breaker.setRemoveOldElements(true);

        breaker.doBreak();

        save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex8_hex20_cube1x1x")+toString(p_size)+std::string(".e"));
        save_or_diff(eMesh, std::string(input_files_loc+"")+std::string("hex20_hex20_cube1x1x")+toString(p_size)+std::string("_0.e"));


        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear hex mesh to serendepity hex20 elements

      TEST(unit1_uniformRefiner, hex8_hex20_1_2)
      {
        fixture_setup();
        EXCEPTWATCH;

        // start_demo_uniformRefiner_hex8_hex20_1_2
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            percept::PerceptMesh eMesh(3u);

            eMesh.open(input_files_loc+"hex_fixture.e");

            Hex8_Hex20_1 break_hex_to_hex(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            eMesh.print_info();
            save_or_diff(eMesh, output_files_loc+"hex20_0.e");

            UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"hex20_1.e");
            save_or_diff(eMesh, input_files_loc+"hex20_hex20_0.e");


            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a serendepity hex20 mesh

      TEST(unit1_uniformRefiner, hex20_hex20_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex20_hex20_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();

        if (p_size <= 3)
          {
            eMesh.open(std::string(input_files_loc+"")+std::string("hex20_hex20_cube1x1x")+toString(p_size)+std::string("_0.e"));

            Hex20_Hex20_8 break_hex_to_hex(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex20_hex20_cube1x1x")+toString(p_size)+std::string("_0.e"));

            UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
            breaker.setRemoveOldElements(true);

            breaker.doBreak();

            save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex20_hex20_cube1x1x")+toString(p_size)+std::string("_1.e"));
          }

        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a serendepity hex20 mesh

      TEST(unit1_uniformRefiner, hex20_hex20_1_2)
      {
        fixture_setup();
        EXCEPTWATCH;

        // start_demo_uniformRefiner_hex20_hex20_1_2
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            percept::PerceptMesh eMesh(3u);

            eMesh.open(input_files_loc+"hex20_hex20_0.e");

            Hex20_Hex20_8 break_hex_to_hex(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            save_or_diff(eMesh, output_files_loc+"hex20_hex20_0.e");

            UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
            //breaker.setIgnoreSideSets(true);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"hex20_hex20_1.e");


            // end_demo
          }
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quadratic hex27 mesh

      TEST(unit1_uniformRefiner, hex27_hex27_0)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_uniformRefiner_hex27_hex27_0

        int scalarDimension = 0; // a scalar
        {
          percept::PerceptMesh eMesh(3u);

          unsigned p_size = eMesh.get_parallel_size();

          std::string gmesh_spec = std::string("1x1x")+toString(p_size)+std::string("|bbox:0,0,0,1,1,"+toString(p_size) );
          eMesh.new_mesh(percept::GMeshSpec(gmesh_spec));

          Hex8_Hex27_1 break_hex_to_hex(eMesh);

          stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

          eMesh.commit();
          eMesh.print_info();
          save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex27_hex27_cube1x1x")+toString(p_size)+std::string("-orig.e"));

          UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
          breaker.setRemoveOldElements(true);

          breaker.doBreak();

          save_or_diff(eMesh, std::string(input_files_loc+"")+std::string("hex27_hex27_cube1x1x")+toString(p_size)+std::string("_0.e"));
        }


        {
          percept::PerceptMesh eMesh(3u);

          unsigned p_size = eMesh.get_parallel_size();
          eMesh.open(std::string(input_files_loc+"")+std::string("hex27_hex27_cube1x1x")+toString(p_size)+std::string("_0.e"));

          Hex27_Hex27_8 break_hex_to_hex(eMesh);

          //stk::mesh::FieldBase* proc_rank_field = eMesh.get_field(stk::topology::ELEMENT_RANK, "proc_rank");
          stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

          eMesh.commit();
          //eMesh.print_info();

          UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
          // FIXME
          breaker.setIgnoreSideSets(true);
          breaker.setRemoveOldElements(true);

          breaker.doBreak();

          save_or_diff(eMesh, std::string(output_files_loc+"")+std::string("hex27_hex27_cube1x1x")+toString(p_size)+std::string("_1.e"));
        }

        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a quadratic hex27 mesh

      TEST(unit1_uniformRefiner, hex8_hex27_hex27_1)
      {
        fixture_setup();
        EXCEPTWATCH;

        // start_demo_uniformRefiner_hex8_hex27_hex27_1
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1 || p_size == 3)
          {
            {
              percept::PerceptMesh eMesh(3u);

              eMesh.open(input_files_loc+"hex_fixture.e");

              Hex8_Hex27_1 break_hex_to_hex(eMesh);

              int scalarDimension = 0; // a scalar
              stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

              eMesh.commit();
              eMesh.print_info();
              save_or_diff(eMesh, output_files_loc+"hex8_hex27_0.e");

              UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
              breaker.doBreak();

              save_or_diff(eMesh, input_files_loc+"hex8_hex27_1.e");
            }

            {
              percept::PerceptMesh eMesh(3u);

              eMesh.open(input_files_loc+"hex8_hex27_1.e");

              Hex27_Hex27_8 break_hex_to_hex(eMesh);

              //stk::mesh::FieldBase* proc_rank_field = eMesh.get_field(stk::topology::ELEMENT_RANK, "proc_rank");
              int scalarDimension = 0;
              stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

              eMesh.commit();

              UniformRefiner breaker(eMesh, break_hex_to_hex, proc_rank_field);
              //FIXME breaker.setIgnoreSideSets(false);
              breaker.setRemoveOldElements(true);

              breaker.doBreak();

              save_or_diff(eMesh, output_files_loc+"hex8_hex27_hex27_1.e");

            }

            // end_demo
          }

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine a linear wedge mesh

      TEST(unit1_uniformRefiner, wedge6_2)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_unit1_uniformRefiner_wedge6_2

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();
        if (p_size == 1)
          {

            //         void createMesh(stk::ParallelMachine parallel_machine,
            //                         unsigned n_nodes_x, unsigned n_nodes_y, unsigned n_nodes_z,
            //                         double xmin, double xmax,
            //                         double ymin, double ymax,
            //                         double zmin, double zmax,
            //                         std::string output_filename
            //                         )
            percept::WedgeFixture wedgeFixture;

            wedgeFixture.createMesh(MPI_COMM_WORLD,
                                    4, 3, 2,
                                    0, 1,
                                    0, 1,
                                    0, 1,
                                    std::string(input_files_loc+"swept-wedge_0.e") );

            eMesh.open(input_files_loc+"swept-wedge_0.e");

            Wedge6_Wedge6_8 break_wedge(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            UniformRefiner breaker(eMesh, break_wedge, proc_rank_field);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"swept-wedge_1.e");

          }
        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear wedge mesh to serendepity Wedge15

      TEST(unit1_uniformRefiner, wedge6_enrich_1)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_unit1_uniformRefiner_wedge6_enrich_1

        percept::PerceptMesh eMesh(3u);

        unsigned p_size = eMesh.get_parallel_size();
        if (p_size == 1)
          {
            //         void createMesh(stk::ParallelMachine parallel_machine,
            //                         unsigned n_nodes_x, unsigned n_nodes_y, unsigned n_nodes_z,
            //                         double xmin, double xmax,
            //                         double ymin, double ymax,
            //                         double zmin, double zmax,
            //                         std::string output_filename
            //                         )
            percept::WedgeFixture wedgeFixture;

            wedgeFixture.createMesh(MPI_COMM_WORLD,
                                    4, 3, 2,
                                    0, 1,
                                    0, 1,
                                    0, 1,
                                    std::string(input_files_loc+"swept-wedge_enrich_0.e") );

            eMesh.open(input_files_loc+"swept-wedge_enrich_0.e");

            Wedge6_Wedge15_1 break_wedge(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            UniformRefiner breaker(eMesh, break_wedge, proc_rank_field);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"swept-wedge_enrich_1.e");
            save_or_diff(eMesh, input_files_loc+"swept-wedge_enrich_refine_0.e");

          }
        // end_demo
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a linear wedge mesh to serendepity Wedge15 then refine it

      TEST(unit1_uniformRefiner, wedge6_enrich_refine)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_unit1_uniformRefiner_wedge6_enrich_refine

        const unsigned p_size = stk::parallel_machine_size( MPI_COMM_WORLD );

        if (p_size == 1)
          {
            PerceptMesh eMesh(3u);
            percept::WedgeFixture wedgeFixture;

            wedgeFixture.createMesh(MPI_COMM_WORLD,
                                    4, 2, 2,
                                    0, 1,
                                    0, 1,
                                    0, 1,
                                    std::string(input_files_loc+"tmp-swept-wedge_enrich_0.e") );

            eMesh.open(input_files_loc+"tmp-swept-wedge_enrich_0.e");

            Wedge6_Wedge15_1 break_wedge(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
            eMesh.commit();
            UniformRefiner breaker(eMesh, break_wedge, proc_rank_field);
            breaker.doBreak();
            save_or_diff(eMesh, input_files_loc+"swept-wedge_2_enrich_refine_0.e");
          }

        percept::PerceptMesh eMesh(3u);

        if (p_size == 1)
          {
            eMesh.open(input_files_loc+"swept-wedge_2_enrich_refine_0.e");

            Wedge15_Wedge15_8 break_wedge(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();

            UniformRefiner breaker(eMesh, break_wedge, proc_rank_field);
            breaker.setIgnoreSideSets(true);
            breaker.doBreak();

            save_or_diff(eMesh, output_files_loc+"swept-wedge_2_enrich_refine_1.e");

          }
        // end_demo

      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Generate a heterogeneous mesh (tet, hex, wedge elements) then refine it

      TEST(unit1_uniformRefiner, heterogeneous_mesh)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        //stk::ParallelMachine pm = MPI_COMM_WORLD ;

        // start_demo_heterogeneous_mesh

        //const unsigned p_rank = stk::parallel_machine_rank( MPI_COMM_WORLD);
        const unsigned p_size = stk::parallel_machine_size( MPI_COMM_WORLD);

        if (p_size <= 1)
          {
            // create the mesh
            {
              percept::HeterogeneousFixture mesh(MPI_COMM_WORLD, false);
              // stk::io::put_io_part_attribute(  mesh.m_block_hex );
              // stk::io::put_io_part_attribute(  mesh.m_block_wedge );
              // stk::io::put_io_part_attribute(  mesh.m_block_tet );

#if HET_FIX_INCLUDE_EXTRA_ELEM_TYPES
              // stk::io::put_io_part_attribute(  mesh.m_block_pyramid );
              // stk::io::put_io_part_attribute(  mesh.m_block_quad_shell );
              // stk::io::put_io_part_attribute(  mesh.m_block_tri_shell );
#endif

              mesh.m_metaData.commit();
              mesh.populate();

              bool isCommitted = true;
              percept::PerceptMesh em1(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);

              //em1.print_info("heterogeneous", 4);

              save_or_diff(em1, input_files_loc+"heterogeneous_0.e");
              em1.close();
            }

            std::string input_mesh = input_files_loc+"heterogeneous_0.e";
            // refine the mesh
            if (1)
              {
                percept::PerceptMesh eMesh1(3);

                if (p_size > 1)
                  {
                    eMesh1.set_ioss_read_options("auto-decomp:yes");
                  }

                eMesh1.open(input_files_loc+"heterogeneous_0.e");

                URP_Heterogeneous_3D break_pattern(eMesh1);
                int scalarDimension = 0; // a scalar
                stk::mesh::FieldBase* proc_rank_field = eMesh1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
                eMesh1.commit();

                UniformRefiner breaker(eMesh1, break_pattern, proc_rank_field);

                //breaker.setRemoveOldElements(false);
                breaker.setIgnoreSideSets(true);
                breaker.doBreak();

                breaker.getRefinementInfo().printTable(std::cout, 0, true );
                breaker.getRefinementInfo().printTable(std::cout, 0, false );

                save_or_diff(eMesh1, output_files_loc+"heterogeneous_1.e");
                eMesh1.close();
              }
          }
        // end_demo
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a heterogeneous mesh

      TEST(unit1_uniformRefiner, heterogeneous_mesh_enrich)
      {
        fixture_setup();

        MPI_Barrier( MPI_COMM_WORLD );

        //stk::ParallelMachine pm = MPI_COMM_WORLD ;

        // start_demo_heterogeneous_mesh_enrich

        //const unsigned p_rank = stk::parallel_machine_rank( MPI_COMM_WORLD);
        const unsigned p_size = stk::parallel_machine_size( MPI_COMM_WORLD);

        if (p_size <= 1)
          {
            // create the mesh
            {
              percept::HeterogeneousFixture mesh(MPI_COMM_WORLD, false);

              mesh.m_metaData.commit();
              mesh.populate();

              bool isCommitted = true;
              percept::PerceptMesh em1(&mesh.m_metaData, &mesh.m_bulkData, isCommitted);

              //em1.print_info("hetero_enrich", 4);

              save_or_diff(em1, input_files_loc+"heterogeneous_enrich_0.e");
              em1.close();
            }

            std::string input_mesh = input_files_loc+"heterogeneous_enrich_0.e";

            // enrich the mesh
            if (1)
              {
                percept::PerceptMesh eMesh1(3);

                if (p_size > 1)
                  {
                    eMesh1.set_ioss_read_options("auto-decomp:yes");
                  }

                eMesh1.open(input_files_loc+"heterogeneous_enrich_0.e");

                URP_Heterogeneous_Enrich_3D break_pattern(eMesh1);
                //int scalarDimension = 0; // a scalar
                stk::mesh::FieldBase* proc_rank_field = 0;      //eMesh1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
                eMesh1.commit();
                //eMesh1.print_info("hetero_enrich_2", 4);

                UniformRefiner breaker(eMesh1, break_pattern, proc_rank_field);

                //breaker.setRemoveOldElements(false);
                breaker.setIgnoreSideSets(true);
                breaker.doBreak();

                save_or_diff(eMesh1, output_files_loc+"heterogeneous_enrich_1.e");
                save_or_diff(eMesh1, input_files_loc+"heterogeneous_quadratic_refine_0.e");

                eMesh1.close();
              }
          }
        // end_demo
      }

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Refine the enriched heterogeneous mesh

      TEST(unit1_uniformRefiner, heterogeneous_quadratic_refine)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        // start_demo_heterogeneous_quadratic_refine

        //const unsigned p_rank = stk::parallel_machine_rank( MPI_COMM_WORLD);
        const unsigned p_size = stk::parallel_machine_size(pm);

        if (p_size <= 1)
          {
            // refine the mesh
            if (1)
              {
                percept::PerceptMesh eMesh1(3);

                eMesh1.open(input_files_loc+"heterogeneous_quadratic_refine_0.e");

                URP_Heterogeneous_QuadraticRefine_3D break_pattern(eMesh1);
                int scalarDimension = 0; // a scalar
                stk::mesh::FieldBase* proc_rank_field = eMesh1.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);
                eMesh1.commit();

                UniformRefiner breaker(eMesh1, break_pattern, proc_rank_field);

                //breaker.setRemoveOldElements(false);
                breaker.setIgnoreSideSets(true);
                breaker.doBreak();

                save_or_diff(eMesh1, output_files_loc+"heterogeneous_quadratic_refine_1.e");
                eMesh1.close();
              }
          }
        // end_demo
      }

      //here

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      /// Enrich a wedge6 mesh to wedge18

      TEST(unit1_uniformRefiner, wedge6_wedge18_enrich)
      {
        fixture_setup();
        EXCEPTWATCH;
        MPI_Barrier( MPI_COMM_WORLD );

        // start_demo_unit1_uniformRefiner_wedge6_wedge18_enrich

        stk::ParallelMachine pm = MPI_COMM_WORLD ;

        //const unsigned p_rank = stk::parallel_machine_rank( MPI_COMM_WORLD);
        const unsigned p_size = stk::parallel_machine_size(pm);


        //unsigned p_size = eMesh.get_parallel_size();
        if (p_size == 1)
          {
            //         void createMesh(stk::ParallelMachine parallel_machine,
            //                         unsigned n_nodes_x, unsigned n_nodes_y, unsigned n_nodes_z,
            //                         double xmin, double xmax,
            //                         double ymin, double ymax,
            //                         double zmin, double zmax,
            //                         std::string output_filename
            //                         )
            percept::WedgeFixture wedgeFixture;

            stk::mesh::BulkData *bulk =
              wedgeFixture.createMesh(MPI_COMM_WORLD,
                                      4, 3, 2,
                                      0, 1,
                                      0, 1,
                                      0, 1,
                                      std::string(""));
            //std::string("swept-wedge6_18_enrich_0.e") );

            percept::PerceptMesh eMesh(wedgeFixture.getMetaData(), bulk, false);
            //percept::PerceptMesh eMesh;
            //eMesh.open("swept-wedge6_18_enrich_0.e");

            Wedge6_Wedge18_1 break_wedge(eMesh);

            int scalarDimension = 0; // a scalar
            stk::mesh::FieldBase* proc_rank_field = eMesh.add_field("proc_rank", stk::topology::ELEMENT_RANK, scalarDimension);

            eMesh.commit();
            //eMesh.print_info();

            wedgeFixture.createBulkAfterMetaCommit(MPI_COMM_WORLD);

            UniformRefiner breaker(eMesh, break_wedge, proc_rank_field);
            breaker.doBreak();

          }
        // end_demo
      }


#endif

      //======================================================================================================================
      //======================================================================================================================
      //======================================================================================================================

      // unit tests of block names handling
      static void test_block_names(PerceptMesh& eMesh, std::string rbar_blocks, std::string blocks, bool debug_print)
      {
        std::string block_name_inc = blocks;
        int p_rank = eMesh.get_rank();

        BlockNamesType block_names(percept::EntityRankEnd+1u);
        BlockNamesType rbar_block_names(percept::EntityRankEnd+1u);
        block_names = RefinerUtil::getBlockNames(blocks, p_rank, eMesh);
        rbar_block_names = RefinerUtil::getBlockNames(rbar_blocks, p_rank, eMesh);
        if (debug_print) std::cout << "blocks= " << blocks << " block_names= " << block_names << std::endl;
        if (debug_print) std::cout << "rbar_blocks= " << rbar_blocks << " rbar_block_names= " << rbar_block_names << std::endl;

        if (rbar_blocks.length())
          {
            BlockNamesType block_names_rbar(percept::EntityRankEnd+1u);
            BlockNamesType rbar_names(percept::EntityRankEnd+1u);
            std::string block_name_inc_orig = block_name_inc;
            block_names_rbar = RefinerUtil::getBlockNames(block_name_inc, eMesh.get_rank(), eMesh);
            rbar_names = RefinerUtil::getBlockNames(rbar_blocks, eMesh.get_rank(), eMesh);
            if (debug_print) std::cout << "rbar_blocks= " << rbar_blocks << " rbar_names= " << rbar_names << std::endl;
            for (unsigned ii=0; ii < rbar_names[eMesh.element_rank()].size(); ii++)
              {
                std::string srb = rbar_names[eMesh.element_rank()][ii];
                Util::replace(srb, "+", "-");
                block_names_rbar[eMesh.element_rank()].push_back(srb);
              }
            block_name_inc = "";
            for (unsigned ii=0; ii < block_names_rbar[eMesh.element_rank()].size(); ii++)
              {
                block_name_inc += (ii ? "," : "") + block_names_rbar[eMesh.element_rank()][ii];
              }
            if (!eMesh.get_rank())
              std::cout << "rbar: original block_name option = " << block_name_inc_orig << " new = " << block_name_inc << std::endl;
          }

        if (block_name_inc.length())
          {
            //std::cout << "block_names: original (or after rbar) block_name option = " << block_name_inc << std::endl;
            block_names = RefinerUtil::getBlockNames(block_name_inc, eMesh.get_rank(), eMesh);
            if (debug_print) std::cout << "block_names: after getBlockNames block_name option = " << block_names << std::endl;
            EXPECT_TRUE(block_name_inc.find(",+block_910,") != std::string::npos);
          }
      }

      TEST(unit1_uniformRefiner, block_names)
      {
        EXCEPTWATCH;
        if (PerceptMesh::is_percept_lite())
          return;
        stk::ParallelMachine pm = MPI_COMM_WORLD ;
        bool debug_print = false;
        //const unsigned p_rank = stk::parallel_machine_rank( pm );
        const unsigned p_size = stk::parallel_machine_size( pm );
        if (p_size == 1)
          {
            const unsigned n = 2;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = 1 , ny = n;

            bool createEdgeSets = false;
            percept::QuadFixture<double > fixture( pm , nx , ny, createEdgeSets);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            // add some empty parts
            for (unsigned ii=905; ii <= 920; ++ii)
              {
                std::string part_name = "block_"+toString(ii);
                stk::mesh::Part& part = eMesh.get_fem_meta_data()->declare_part(part_name, stk::topology::ELEMENT_RANK);
                if (debug_print) std::cout << "added part= " << part.name() << std::endl;
              }

            for (unsigned ii=9090; ii <= 9110; ++ii)
              {
                std::string part_name = "block_"+toString(ii);
                stk::mesh::Part& part = eMesh.get_fem_meta_data()->declare_part(part_name, stk::topology::ELEMENT_RANK);
                if (debug_print) std::cout << "added part= " << part.name() << std::endl;
              }
            eMesh.commit();
            if (debug_print) eMesh.print_info("parts", 2);

            std::string rbar_blocks = "9100,9102,9104,9106";
            std::string blocks = "-9101,-9103,-9105,-9107";
            test_block_names(eMesh, rbar_blocks, blocks, debug_print);
          }

        // slightly different, in case the name is at the end of the list
        if (p_size == 1)
          {
            const unsigned n = 2;
            //const unsigned nx = n , ny = n , nz = p_size*n ;
            const unsigned nx = 1 , ny = n;

            bool createEdgeSets = false;
            percept::QuadFixture<double > fixture( pm , nx , ny, createEdgeSets);

            bool isCommitted = false;
            percept::PerceptMesh eMesh(&fixture.meta_data, &fixture.bulk_data, isCommitted);

            // add some empty parts
            for (unsigned ii=905; ii <= 910; ++ii)
              {
                std::string part_name = "block_"+toString(ii);
                stk::mesh::Part& part = eMesh.get_fem_meta_data()->declare_part(part_name, stk::topology::ELEMENT_RANK);
                if (debug_print) std::cout << "added part= " << part.name() << std::endl;
              }

            for (unsigned ii=9090; ii <= 9100; ++ii)
              {
                std::string part_name = "block_"+toString(ii);
                stk::mesh::Part& part = eMesh.get_fem_meta_data()->declare_part(part_name, stk::topology::ELEMENT_RANK);
                if (debug_print) std::cout << "added part= " << part.name() << std::endl;
              }
            eMesh.commit();

            if (debug_print) eMesh.print_info("parts", 2);

            std::string rbar_blocks = "9100";
            std::string blocks = "-9090";
            test_block_names(eMesh, rbar_blocks, blocks, debug_print);
          }
      }

    } // namespace unit_tests
  } // namespace percept


