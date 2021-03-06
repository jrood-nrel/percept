// Copyright 2002 - 2008, 2010, 2011 National Technology Engineering
// Solutions of Sandia, LLC (NTESS). Under the terms of Contract
// DE-NA0003525 with NTESS, the U.S. Government retains certain rights
// in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

// MWG: deoptimize this only on ARM with ARM's HPC Clang compiler
// Needed with ARM CLANG 18.1.0. Re-evaluate for later versions
#if defined(__clang__) && defined(__aarch64__)
#pragma clang optimize off
#endif

#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/MetaData.hpp>

#include <percept/Percept.hpp>
#include <percept/function/StringFunction.hpp>
#include <percept/function/FieldFunction.hpp>
#include <percept/function/FunctionOperator.hpp>
#include <percept/function/ConstantFunction.hpp>
#include <percept/Util.hpp>
#include <percept/norm/Norm.hpp>
#include <percept/norm/H1Norm.hpp>
#include <percept/math/Math.hpp>
#include <percept/PerceptMesh.hpp>
#include <percept/fixtures/Fixture.hpp>

#include <stk_util/environment/WallTime.hpp>
#include <stk_util/diag/PrintTable.hpp>
#include <stk_util/parallel/Parallel.hpp>
#include <gtest/gtest.h>

#include <Teuchos_ScalarTraits.hpp>

#include <stdexcept>
#include <sstream>
#include <vector>
#include <cmath>
#include <iostream>
#include <string>
#include <typeinfo>
#include <math.h>

namespace percept {
namespace unit_tests {


#if 0
static stk::diag::Writer &
dw()
{
  //static stk::diag::Writer s_diagWriter(dwout().rdbuf(), 0);
  int dw_enabled = 1;
  static stk::diag::Writer s_diagWriter(std::cout.rdbuf(), dw_enabled);

  s_diagWriter.setPrintMask(LOG_NORM+LOG_ALWAYS);
  return s_diagWriter;
}
#endif

#define EXTRA_PRINT 0

//=============================================================================
//=============================================================================
//=============================================================================
struct LocalFixture
{
  PerceptMesh eMesh;
  int bogus_init;
  stk::mesh::FieldBase *coords_field;
  StringFunction sfx;
  ConstantFunction sfx_res;

  LocalFixture(size_t num_xyz = 4, size_t num_y=0, size_t num_z=0, bool sidesets=false, bool commit=true) : eMesh(3u), bogus_init(init(num_xyz, num_y, num_z, sidesets, commit)),
                                                                     sfx("x", Name("sfx"), Dimensions(3), Dimensions(1) ),
                                                                     sfx_res (0.0, "sfx_res")

  {

  }

  int init(size_t num_xyz, size_t num_y_arg, size_t num_z_arg, bool sidesets=false, bool commit=true)
  {
    // Need a symmetric mesh around the origin for some of the tests below to work correctly (i.e. have analytic solutions)
    const size_t num_x = num_xyz;
    const size_t num_y = num_y_arg? num_y_arg : num_xyz;
    const size_t num_z = num_z_arg? num_z_arg : num_xyz;
    std::string config_mesh =
      Ioss::Utils::to_string(num_x) + "x" +
      Ioss::Utils::to_string(num_y) + "x" +
      Ioss::Utils::to_string(num_z) + "|bbox:-0.5,-0.5,-0.5,0.5,0.5,0.5";
    if (sidesets) config_mesh += "|sideset:xXyYzZ";

    eMesh.new_mesh(GMeshSpec(config_mesh));
    if (commit) eMesh.commit();
    coords_field = eMesh.get_fem_meta_data()->get_field(stk::topology::NODE_RANK, "coordinates");
    return 1;
  }

};

//=============================================================================
//=============================================================================
//=============================================================================

//stk::diag::WriterThrowSafe _write_throw_safe(dw());
//dw().setPrintMask(dw_option_mask.parse(vm["dw"].as<std::string>().c_str()));
//dw().setPrintMask(LOG_NORM+LOG_ALWAYS);

TEST(norm, volume)
{
  EXCEPTWATCH;
  MPI_Barrier( MPI_COMM_WORLD );

  dw().m(LOG_NORM) << "TEST::norm::volume " << stk::diag::dendl;

  LocalFixture fix(3,3,12);
  stk::mesh::MetaData* metaData = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData* bulkData = fix.eMesh.get_bulk_data();
  PerceptMesh& eMesh = fix.eMesh;

  stk::mesh::FieldBase *coords_field = fix.coords_field;

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated - here it is just the identity, and when integrated should produce the volume
  ConstantFunction identity(1.0, "identity");

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction sqrt_volume(0.0, "sqrt_volume");

  {
    bool expected_1 = false;
    bool expected_2 = false;
    Norm<2> l2Norm_test(*bulkData, &metaData->universal_part(), TURBO_NONE, true);
    expected_1 = !l2Norm_test.get_is_surface_norm();
    Norm<2> l2Norm_test1(*bulkData, &metaData->universal_part(), TURBO_NONE, false);
    expected_2 = !l2Norm_test1.get_is_surface_norm();
    EXPECT_TRUE(expected_1);
    EXPECT_TRUE(expected_2);
  }


  /// Create the operator that will do the work
  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  /// get the l2 norm of identity
  l2Norm(identity, sqrt_volume);

  Teuchos::ScalarTraits<double>::seedrandom(12345);
  EXPECT_DOUBLE_EQ_APPROX(1.0, sqrt_volume.getValue());

  int niter=1;
  for (int iter=0; iter < niter; iter++)
  {
    EXPECT_DOUBLE_EQ_APPROX(1.0, eval(Math::random01(), Math::random01(), Math::random01(), 0.0, sqrt_volume));
  }

  //// rotate the mesh

#define DO_IO_TESTING 1

#if DO_IO_TESTING
  if (EXTRA_PRINT) std::cout << "TEST.norm.volume: writing gmesh_hex8_original_out.e ..." << std::endl;
  eMesh.save_as("./gmesh_hex8_original_out.e");
  if (EXTRA_PRINT) std::cout << "TEST.norm.volume: writing gmesh_hex8_original_out.e done" << std::endl;
#endif

  if (1)
  {
    Math::Matrix rmx = Math::rotationMatrix(0, 30);
    Math::Matrix rmy = Math::rotationMatrix(1, -45);
    Math::Matrix rmz = Math::rotationMatrix(2, 30);
    Math::Matrix rm;
    rm =  rmy * rmz;
    rm =  rmx * rm;
    eMesh.transform_mesh(rm);

    // for testing
#if DO_IO_TESTING
    if (EXTRA_PRINT) std::cout << "TEST.norm.volume: writing gmesh_hex8_rotated_out.e ..." << std::endl;
    eMesh.save_as("./gmesh_hex8_rotated_out.e");
    if (EXTRA_PRINT) std::cout << "TEST.norm.volume: writing gmesh_hex8_rotated_out.e done" << std::endl;
#endif

    l2Norm(identity, sqrt_volume);

    EXPECT_DOUBLE_EQ_APPROX(1.0, sqrt_volume.getValue());
  }

  //// scale the mesh
  if (1)
  {

    double scx = M_PI;
    double scy = M_E;
    double scz = std::sqrt(3.0);
    double sc=scx*scy*scz;
    Math::Matrix smx = Math::scalingMatrix(0, scx);
    Math::Matrix smy = Math::scalingMatrix(1, scy);
    Math::Matrix smz = Math::scalingMatrix(2, scz);
    Math::Matrix sm;
    sm =  smy * smz;
    sm =  smx * sm;
    //std::cout << "sm= " << sm << std::endl;
    eMesh.transform_mesh(sm);

    // for testing
#if DO_IO_TESTING
    if (EXTRA_PRINT) std::cout << "TEST.norm.volume: writing gmesh_hex8_scaled_out.e ..." << std::endl;
    eMesh.save_as("./gmesh_hex8_scaled_out.e");
    if (EXTRA_PRINT) std::cout << "TEST.norm.volume: writing gmesh_hex8_scaled_out.e done" << std::endl;
#endif

    l2Norm(identity, sqrt_volume);

    EXPECT_DOUBLE_EQ_APPROX(std::sqrt(sc), sqrt_volume.getValue());
  }

  //Function coords_l2_norm = ff_coords.norm_l2();

#if 0
  /// OPTION 1 - separate classes for operations on fields
  Norm_L2<VectorField> coords_l2_norm("coords_l2_norm");
  FieldFunction result = coords_l2_norm(ff_coords);


  /// OPTION 1a - just showing generic base class
  /// A FunctionOperator takes a FieldFunction and returns a FieldFunction
  FunctionOperator& coords_l2_norm_op = Norm_L2<VectorField>("coords_l2_norm");
  FieldFunction coords_l2_norm = coords_l2_norm_op(ff_coords);

  /// OPTION 2 - simple set of methods on FieldFunction or Function (depending on genericity)
  FieldFunction coords_l2_norm                           = ff_coords.norm_l2();
  FieldFunction coords_gradient_tensor_field_nodewise    = ff_coords.nodewise_gradient();
  FieldFunction coords_mag_nodewise_field                = ff_coords.nodewise_magnitude();
  FieldFunction coords_gradient_tensor_field_elementwise = ff_coords.elementwise_gradient();
#endif
  /// Note: need to create new fields each time, which requires a change to the meta data

}

//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, surface_area)
{
  EXCEPTWATCH;
  MPI_Barrier( MPI_COMM_WORLD );

  bool sidesets=true;
  LocalFixture fix(3,3,12,sidesets);
  //mesh::MetaData& metaData = fix.metaData;
  stk::mesh::BulkData* bulkData = fix.eMesh.get_bulk_data();
  PerceptMesh& eMesh = fix.eMesh;
  //eMesh.save_as("junk.123.e");

  stk::mesh::FieldBase *coords_field = fix.coords_field;

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated - here it is just the identity, and when integrated should produce the area of faces
  ConstantFunction identity(1.0, "identity");

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction sqrt_area(0.0, "sqrt_area");
  ConstantFunction sqrt_area1(0.0, "sqrt_area1");

  /// Create the operator that will do the work
  stk::mesh::Part *surface_part = eMesh.get_part("surface_1");
  stk::mesh::Part *block_part = eMesh.get_part("block_1");
  stk::mesh::Selector selector(*surface_part);
  stk::mesh::Selector selector_test(*block_part);
  selector_test = selector_test | selector;
  bool is_surface_norm = true;

  {
    bool expected_1 = false;
    bool expected_2 = false;
    bool expected_3 = false;
    Norm<2> l2Norm_test(*bulkData, &selector, TURBO_NONE, true);
    expected_1 = l2Norm_test.get_is_surface_norm();
    Norm<2> l2Norm_test1(*bulkData, &selector, TURBO_NONE, false);
    expected_2 = l2Norm_test1.get_is_surface_norm();
    EXPECT_TRUE(expected_1);
    EXPECT_TRUE(expected_2);
    try { Norm<2> l2Norm_test2(*bulkData, &selector_test, TURBO_NONE, true); }
    catch ( const std::exception & X ) {
      std::cout << "expected exception: " << X.what() << std::endl;
      expected_3 = true;
    }
    EXPECT_TRUE(expected_3);
  }

  Norm<2> l2Norm(*bulkData, &selector, TURBO_NONE, is_surface_norm);
  Norm<2> l2Norm1(*bulkData, "surface_1", TURBO_NONE);
  l2Norm1.set_is_surface_norm(true);

  /// get the l2 norm of identity
  l2Norm(identity, sqrt_area);
  l2Norm1(identity, sqrt_area1);

  Teuchos::ScalarTraits<double>::seedrandom(12345);
  EXPECT_DOUBLE_EQ_APPROX(1.0, sqrt_area.getValue());
  EXPECT_DOUBLE_EQ_APPROX(1.0, sqrt_area1.getValue());

  int niter=1;
  for (int iter=0; iter < niter; iter++)
  {
    EXPECT_DOUBLE_EQ_APPROX(1.0, eval(Math::random01(), Math::random01(), Math::random01(), 0.0, sqrt_area));
  }

  //// rotate the mesh

#if DO_IO_TESTING
  eMesh.save_as("./gmesh_hex8_area_original_out.e");
#endif

  if (1)
  {
    Math::Matrix rmx = Math::rotationMatrix(0, 30);
    Math::Matrix rmy = Math::rotationMatrix(1, -45);
    Math::Matrix rmz = Math::rotationMatrix(2, 30);
    Math::Matrix rm;
    rm =  rmy * rmz;
    rm =  rmx * rm;
    eMesh.transform_mesh(rm);

    // for testing
#if DO_IO_TESTING
    eMesh.save_as("./gmesh_hex8_area_rotated_out.e");
#endif

    l2Norm(identity, sqrt_area);

    EXPECT_DOUBLE_EQ_APPROX(1.0, sqrt_area.getValue());
  }

  //// scale the mesh
  if (1)
  {

    double scx = M_PI;
    double scy = M_PI;
    double scz = M_PI;
    double sc=scx*scy;
    Math::Matrix smx = Math::scalingMatrix(0, scx);
    Math::Matrix smy = Math::scalingMatrix(1, scy);
    Math::Matrix smz = Math::scalingMatrix(2, scz);
    Math::Matrix sm;
    sm =  smy * smz;
    sm =  smx * sm;
    //std::cout << "sm= " << sm << std::endl;
    eMesh.transform_mesh(sm);

    // for testing
#if DO_IO_TESTING
    eMesh.save_as("./gmesh_hex8_area_scaled_out.e");
#endif

    l2Norm(identity, sqrt_area);

    EXPECT_DOUBLE_EQ_APPROX(std::sqrt(sc), sqrt_area.getValue());
  }
  //Function coords_l2_norm = ff_coords.norm_l2();

}

//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, string_function)
{
  EXCEPTWATCH;
  //stk::diag::WriterThrowSafe _write_throw_safe(dw());
  //dw().setPrintMask(dw_option_mask.parse(vm["dw"].as<std::string>().c_str()));
  //dw().setPrintMask(LOG_NORM+LOG_ALWAYS);

  dw().m(LOG_NORM) << "TEST.norm.string_function " << stk::diag::dendl;

  LocalFixture     fix(4);
  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  PerceptMesh&        eMesh     = fix.eMesh;
  //mesh::FieldBase*       coords_field = fix.coords_field;
  StringFunction   sfx          = fix.sfx;
  ConstantFunction sfx_res      = fix.sfx_res;

  /// Create the operator that will do the work
  /// get the l2 norm
  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  l2Norm(sfx, sfx_res);

  double sfx_expect = std::sqrt(0.25/3.);
  Teuchos::ScalarTraits<double>::seedrandom(12345);
  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res.getValue());

  int niter=1;
  for (int iter=0; iter < niter; iter++)
  {
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, eval(Math::random01(), Math::random01(), Math::random01(), 0.0, sfx_res));
  }

  /// the function to be integrated:  (Integral[ abs(x), dxdydz]) =?= (2 * |x|^2/2 @ [0, 0.5]) ==> .25)
  Norm<1> l1Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  l1Norm(sfx, sfx_res);

  sfx_expect = 0.25;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());

  /// the function to be integrated:  (Max[ x^2+y^3+z^4, dxdydz]) =?= (@ [-0.5, 0.5]^3 ) ==> .5^2+.5^3+.5^4)
  StringFunction sfmax("x^2 + y^3 + z^4", Name("sfmax"), Dimensions(3), Dimensions(1) );
  Norm<-1> lInfNorm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  lInfNorm.setCubDegree(10);
  lInfNorm(sfmax, sfx_res);
  double sf1=eval(.5,.5,.5,0.0, sfmax);
  sfx_expect = 0.5*0.5 + 0.5*0.5*0.5 + 0.5*0.5*0.5*0.5;
  std::cout << "sfmax= " << sf1 << " sfx_expect= " << sfx_expect << " sfx_res= " << sfx_res.getValue() << std::endl;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());

  /// indirection
  StringFunction sfmax_1("sfmax", Name("sfmax_1"), Dimensions(3), Dimensions(1) );
  double sf1_1=eval(.5,.5,.5,0.0, sfmax_1);
  std::cout << "sfmax_1= " << sf1_1 << " sfx_expect= " << sfx_expect << std::endl;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sf1_1);

  /// the function to be integrated:  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest1.py)
  StringFunction sfxyz("x*y*z", Name("sfxyz"), Dimensions(3), Dimensions(1) );
  l2Norm(sfxyz, sfx_res);
  sfx_expect = 0.0240562612162344;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());

  /// indirection
  std::cout << "tmp srk start..." << std::endl;
  StringFunction sfxyz_2("sfxyz", Name("sfxyz_2"), Dimensions(3), Dimensions(1) );
  l2Norm(sfxyz_2, sfx_res);
  sfx_expect = 0.0240562612162344;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());

  /// the function to be integrated (but over a rotated domain):  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest2.py)
  /// now rotate the mesh
  Math::Matrix rmz = Math::rotationMatrix(2, 30);
  Math::Matrix rm = rmz;
  eMesh.transform_mesh(rm);

  l2Norm(sfxyz, sfx_res);
  sfx_expect = 0.0178406008037016;
  // NOTE: we need extra quadrature accuracy to reproduce this result (cubDegree==4 in IntegratedOp almost gets it right)
  //   for now, we are satisfied with 2-3 digits
  //EXPECT_DOUBLE_EQ(sfx_res.getValue(), sfx_expect);
  if (std::fabs(sfx_res.getValue()-sfx_expect) > 0.01*sfx_expect)
  {
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
    EXPECT_TRUE(false);
  }
}

//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, string_function_1)
{
  EXCEPTWATCH;
  LocalFixture     fix(4);
  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  //PerceptMesh&        eMesh     = fix.eMesh;
  //mesh::FieldBase*       coords_field = fix.coords_field;
  StringFunction   sfx          = fix.sfx;
  ConstantFunction sfx_res      = fix.sfx_res;

  /// Create the operator that will do the work
  /// get the l2 norm
  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  if (0) l2Norm(sfx, sfx_res);

  /// the function to be integrated:  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest1.py)
  StringFunction sfxyz("x*y*z", Name("sfxyz"), Dimensions(3), Dimensions(1) );
  //l2Norm(sfxyz, sfx_res);
  //sfx_expect = 0.0240562612162344;
  //EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());

  /// indirection
  std::cout << "tmp srk start..." << std::endl;
  StringFunction sfxyz_first("sfxyz", Name("sfxyz_first"), Dimensions(3), Dimensions(1) );
  l2Norm(sfxyz_first, sfx_res);
  double sfx_expect = 0.0240562612162344;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
}

//=============================================================================
//=============================================================================
//=============================================================================

/// This test uses a back door to the function that passes in the element to avoid the lookup of the element when the
///  StringFunction contains references to FieldFunctions
void TEST_norm_string_function_turbo_verify_correctness(TurboOption turboOpt)
{
  EXCEPTWATCH;
  //stk::diag::WriterThrowSafe _write_throw_safe(dw());
  //dw().setPrintMask(dw_option_mask.parse(vm["dw"].as<std::string>().c_str()));
  //dw().setPrintMask(LOG_NORM+LOG_ALWAYS);

  dw().m(LOG_NORM) << "TEST.norm.string_function " << stk::diag::dendl;

  LocalFixture     fix(4);
  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  PerceptMesh&        eMesh     = fix.eMesh;
  stk::mesh::FieldBase*       coords_field = fix.coords_field;
  StringFunction   sfx          = fix.sfx;
  ConstantFunction sfx_res      = fix.sfx_res;

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated:  sqrt(Integral[x^2, dxdydz]) =?= sqrt(x^3/3 @ [-0.5, 0.5]) ==> sqrt(0.25/3)
  //StringFunction sfx("x", Name("sfx"), Dimensions(3), Dimensions(1) );

  ff_coords.add_alias("mc");
  //StringFunction sfcm("sqrt(mc[0]*mc[0]+mc[1]*mc[1]+mc[2]*mc[2])", Name("sfcm"), Dimensions(3), Dimensions(1));
  StringFunction sfx_mc("mc[0]", Name("sfx_mc"), Dimensions(3), Dimensions(1) );
  StringFunction sfx_mc1("mc[0]", Name("sfx_mc1"), Dimensions(3), Dimensions(1) );

  if (1)
  {


    double x = -0.49+0.98*Math::random01();
    double y = -0.49+0.98*Math::random01();
    double z = -0.49+0.98*Math::random01();

    EXPECT_DOUBLE_EQ_APPROX(eval(x,y,z,0.0, sfx), eval(x,y,z,0.0, sfx_mc));
    EXPECT_DOUBLE_EQ_APPROX(eval(.034,0,0,0.0, sfx), eval(.034,0,0,0.0, sfx_mc));
  }

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction sfx_res_turbo(0.0, "sfx_res_turbo");
  ConstantFunction sfx_res_slow(0.0, "sfx_res_slow");
  ConstantFunction sfx_res_fast(0.0, "sfx_res_fast");
  ConstantFunction sfx_res_bucket(0.0, "sfx_res_bucket");

  // STATE m_element....

  /// Create the operator that will do the work
  /// get the l2 norm
  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), TURBO_ELEMENT);
  l2Norm(sfx, sfx_res);
  Norm<2> l2Norm_turbo(*bulkData, &metaData->universal_part(), turboOpt);
  Norm<2> l2Norm_turbo_bucket(*bulkData, &metaData->universal_part(), TURBO_BUCKET);
  Norm<2> l2Norm_turbo1(*bulkData, &metaData->universal_part(), turboOpt);
  l2Norm_turbo(sfx, sfx_res_turbo);

  l2Norm_turbo1(sfx_mc1, sfx_res_fast);
  l2Norm_turbo_bucket(sfx_mc1, sfx_res_bucket);

  l2Norm(sfx_mc, sfx_res_slow);
  EXPECT_DOUBLE_EQ_APPROX(eval(.023,0,0,0.0, sfx), eval(.023,0,0,0.0, sfx_mc));

  double sfx_expect = std::sqrt(0.25/3.);
  std::cout << "sfx_expect= " << sfx_expect << std::endl;

  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_bucket.getValue());

  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res.getValue());

  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_turbo.getValue());
  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_slow.getValue());
  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_fast.getValue()); //!
  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res.getValue());

  //Util::pause(true, "13a");
  //EXPECT_DOUBLE_EQ_APPROX(sfx_res_turbo.getValue(), sfx_expect);
  EXPECT_DOUBLE_EQ_APPROX_TOL(sfx_expect, sfx_res_turbo.getValue(),  1.e-8);
  //Util::pause(true, "13");

  /// the function to be integrated:  (Integral[ abs(x), dxdydz]) =?= (2 * |x|^2/2 @ [0, 0.5]) ==> .25)
  Norm<1> l1Norm(*bulkData, &metaData->universal_part(), TURBO_ELEMENT);
  l1Norm(sfx, sfx_res);

  Norm<1> l1Norm_turbo(*bulkData, &metaData->universal_part(), TURBO_ELEMENT);
  l1Norm_turbo(sfx, sfx_res_turbo);
  l1Norm_turbo(sfx_mc, sfx_res_fast);
  l1Norm(sfx_mc, sfx_res_slow);

  sfx_expect = 0.25;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_turbo.getValue());

  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_slow.getValue());
  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_fast.getValue());

  //// ----- here
  /// the function to be integrated:  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest1.py)
  StringFunction sfxyz("x*y*z", Name("sfxyz"), Dimensions(3), Dimensions(1) );
  l2Norm(sfxyz, sfx_res);
  l2Norm_turbo(sfxyz, sfx_res_turbo);
  sfx_expect = 0.0240562612162344;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_turbo.getValue());


  /// the function to be integrated (but over a rotated domain):  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest2.py)
  /// now rotate the mesh
  Math::Matrix rmz = Math::rotationMatrix(2, 30);
  Math::Matrix rm = rmz;
  eMesh.transform_mesh(rm);

  l2Norm(sfxyz, sfx_res);
  l2Norm_turbo(sfxyz, sfx_res_turbo);
  sfx_expect = 0.0178406008037016;
  // NOTE: we need extra quadrature accuracy to reproduce this result (cubDegree==4 in IntegratedOp almost gets it right)
  //   for now, we are satisfied with 3 digits
  //EXPECT_DOUBLE_EQ_APPROX(sfx_res.getValue(), sfx_expect);
  if (std::fabs(sfx_res.getValue()-sfx_expect) > 0.01*sfx_expect)
  {
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
    EXPECT_TRUE(false);
  }
  if (std::fabs(sfx_res_turbo.getValue()-sfx_expect) > 0.01*sfx_expect)

  {
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_turbo.getValue());
    EXPECT_TRUE(false);
  }
}

TEST(norm, string_function_turbo_verify_correctness_element)
{
  EXCEPTWATCH;

  TEST_norm_string_function_turbo_verify_correctness(TURBO_ELEMENT);
}

TEST(norm, string_function_turbo_verify_correctness_bucket)
{
  EXCEPTWATCH;

  TEST_norm_string_function_turbo_verify_correctness(TURBO_BUCKET);
}

//=============================================================================
//=============================================================================
//=============================================================================

/// This test uses a back door to the function that passes in the element to avoid the lookup of the element when the
///  StringFunction contains references to FieldFunctions
void TEST_norm_string_function_turbo_timings(TurboOption turboOpt)
{
  EXCEPTWATCH;
  //stk::diag::WriterThrowSafe _write_throw_safe(dw());
  //dw().setPrintMask(dw_option_mask.parse(vm["dw"].as<std::string>().c_str()));
  //dw().setPrintMask(LOG_NORM+LOG_ALWAYS);

  dw().m(LOG_NORM) << "TEST.norm.string_function " << stk::diag::dendl;

  /// create a meta data/bulk data empty pair
  PerceptMesh eMesh(3u);

  if (1)
  {
    // Need a symmetric mesh around the origin for some of the tests below to work correctly (i.e. have analytic solutions)
    const size_t nxyz = 4;
    const size_t num_x = nxyz;
    const size_t num_y = nxyz;
    const size_t num_z = nxyz;
    std::string config_mesh =
      Ioss::Utils::to_string(num_x) + "x" +
      Ioss::Utils::to_string(num_y) + "x" +
      Ioss::Utils::to_string(num_z) + "|bbox:-0.5,-0.5,-0.5,0.5,0.5,0.5";

    eMesh.new_mesh(GMeshSpec(config_mesh));

    eMesh.commit();
  }

  stk::mesh::MetaData* metaData = eMesh.get_fem_meta_data();
  stk::mesh::BulkData* bulkData = eMesh.get_bulk_data();

  /// the coordinates field is always created by the PerceptMesh read operation, here we just get the field
  stk::mesh::FieldBase *coords_field = metaData->get_field(stk::topology::NODE_RANK, "coordinates");

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated:  sqrt(Integral[x^2, dxdydz]) =?= sqrt(x^3/3 @ [-0.5, 0.5]) ==> sqrt(0.25/3)
  StringFunction sfx("x", Name("sfx"), Dimensions(3), Dimensions(1) );

  ff_coords.add_alias("mc");
  //StringFunction sfcm("sqrt(mc[0]*mc[0]+mc[1]*mc[1]+mc[2]*mc[2])", Name("sfcm"), Dimensions(3), Dimensions(1));
  StringFunction sfx_mc("mc[0]", Name("sfx_mc"), Dimensions(3), Dimensions(1) );

  /// the function to be integrated:  sqrt(Integral[x^2, dxdydz]) =?= sqrt(x^3/3 @ [-0.5, 0.5]) ==> sqrt(0.25/3)

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction sfx_res(0.0, "sfx_res");
  ConstantFunction sfx_res_turbo(0.0, "sfx_res_turbo");
  ConstantFunction sfx_res_slow(0.0, "sfx_res_slow");
  ConstantFunction sfx_res_fast(0.0, "sfx_res_fast");

#define COL_SEP "|"
#define EXPR_CELL_WIDTH (80)

#define TIME_IT2(expr_none,expr_turbo,msg,topt)                         \
  {                                                                     \
    double TURBO_NONE_time    = 0;                                      \
    double TURBO_ON_time = 0;                                           \
    TIME_IT(expr_none,TURBO_NONE_time);                                 \
    TIME_IT(expr_turbo,TURBO_ON_time);                                  \
    if (1) std::cout << msg << #topt << "for expression= " << QUOTE(expr_none) << " timings= " << std::endl; \
    if (1) std::cout << "TURBO_NONE_time= " << TURBO_NONE_time << " "   \
                     << ( turboOpt==TURBO_ELEMENT?"TURBO_ELEMENT_time":"TURBO_BUCKET_time") <<"= " << TURBO_ON_time \
                     << " ratio= " << TURBO_NONE_time/TURBO_ON_time << std::endl; \
  }

  int numIter = 1;
  for (int iter = 0; iter < numIter; iter++)
  {
    /// Create the operator that will do the work
    /// get the l2 norm
    Norm<2> l2Norm      (*bulkData, &metaData->universal_part(), TURBO_ELEMENT);
    Norm<2> l2Norm_turbo(*bulkData, &metaData->universal_part(), turboOpt);

    //double TURBO_ELEMENT_time=0;
    TIME_IT2(l2Norm(sfx, sfx_res); , l2Norm_turbo(sfx, sfx_res_turbo);, "Should be the same turboOpt= ", turboOpt );
    TIME_IT2(l2Norm(sfx_mc, sfx_res_slow); , l2Norm_turbo(sfx, sfx_res_fast); , "StringFunction with ref to FF, slow vs. fast" ,turboOpt );

    double sfx_expect = std::sqrt(0.25/3.);
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
    EXPECT_DOUBLE_EQ_APPROX_TOL( sfx_expect, sfx_res_turbo.getValue(), 1.e-8);
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_slow.getValue());
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_fast.getValue());

    /// the function to be integrated:  (Integral[ abs(x), dxdydz]) =?= (2 * |x|^2/2 @ [0, 0.5]) ==> .25)
    Norm<1> l1Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
    l1Norm(sfx, sfx_res);

    Norm<1> l1Norm_turbo(*bulkData, &metaData->universal_part(), TURBO_NONE);
    l1Norm_turbo(sfx, sfx_res_turbo);

    sfx_expect = 0.25;
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_turbo.getValue());

    /// the function to be integrated:  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest1.py)
    StringFunction sfxyz("x*y*z", Name("sfxyz"), Dimensions(3), Dimensions(1) );

    TIME_IT2(l2Norm(sfxyz, sfx_res); ,  l2Norm_turbo(sfxyz, sfx_res_turbo); ,  "should be the same", turboOpt );

    sfx_expect = 0.0240562612162344;
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
    EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_turbo.getValue());

    /// the function to be integrated (but over a rotated domain):  sqrt(Integral[(x*y*z)^2, dxdydz]) =?= (see unitTest2.py)
    /// now rotate the mesh
    Math::Matrix rmz = Math::rotationMatrix(2, 30);
    Math::Matrix rm = rmz;
    eMesh.transform_mesh(rm);

    TIME_IT2( l2Norm(sfxyz, sfx_res); , l2Norm_turbo(sfxyz, sfx_res_turbo); , "should be the same", turboOpt );

    sfx_expect = 0.0178406008037016;
    // NOTE: we need extra quadrature accuracy to reproduce this result (cubDegree==4 in IntegratedOp almost gets it right)
    //   for now, we are satisfied with 3 digits
    //EXPECT_DOUBLE_EQ_APPROX(sfx_res.getValue(), sfx_expect);
    if (std::fabs(sfx_res.getValue()-sfx_expect) > 0.01*sfx_expect)
    {
      EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res.getValue());
      EXPECT_TRUE(false);
    }
    if (std::fabs(sfx_res_turbo.getValue()-sfx_expect) > 0.01*sfx_expect)
    {
      EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_turbo.getValue());
      EXPECT_TRUE(false);
    }
  }
}

//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, string_function_turbo_timings)
{
  EXCEPTWATCH;
  TEST_norm_string_function_turbo_timings(TURBO_ELEMENT);
}
TEST(norm, string_function_turbo_timings_bucket)
{
  EXCEPTWATCH;
  TEST_norm_string_function_turbo_timings(TURBO_BUCKET);
}

//=============================================================================
//=============================================================================
//=============================================================================

static void TEST_norm_field_function(TurboOption turboOpt)
{
  EXCEPTWATCH;
  //stk::diag::WriterThrowSafe _write_throw_safe(dw());
  //dw().setPrintMask(dw_option_mask.parse(vm["dw"].as<std::string>().c_str()));
  //dw().setPrintMask(LOG_NORM+LOG_ALWAYS);

  dw().m(LOG_NORM) << "TEST.norm.field_function " << stk::diag::dendl;

  LocalFixture fix(4,4,12, false, false);
  PerceptMesh& eMesh = fix.eMesh;

  int vectorDimension = 0;  // signifies a scalar field
  stk::mesh::FieldBase *scalar_field = eMesh.add_field("scalar", stk::topology::NODE_RANK, vectorDimension);
  eMesh.commit();

  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  //PerceptMesh&        eMesh     = fix.eMesh;
  stk::mesh::FieldBase*       coords_field = fix.coords_field;

  /// Create the operator that will do the work
  /// get the l2 norm
  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), turboOpt);

  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  std::vector<double> vals(3,0.0);
  ConstantFunctionVec sfx_res_vec(vals, "sfx_res_vec");

  Util::setFlag(0, false);
  l2Norm(ff_coords, sfx_res_vec);

  double sfx_expect = std::sqrt(0.25/3.);
  //std::cout << "tmp vec" << sfx_res_vec.getValue() << std::endl;
  EXPECT_DOUBLE_EQ_APPROX(sfx_expect, sfx_res_vec.getValue()[0]);

  /// the function to be integrated:  (Integral[ abs(x), dxdydz]) =?= (2 * |x|^2/2 @ [0, 0.5]) ==> .25)
  Norm<1> l1Norm(*bulkData, &metaData->universal_part(), turboOpt);
  l1Norm(ff_coords, sfx_res_vec);

  sfx_expect = 0.25;
  EXPECT_DOUBLE_EQ_APPROX( sfx_expect, sfx_res_vec.getValue()[0]);

  // bcarnes: test using scalar field function
  StringFunction sf_scalar( "x*y*z" , Name("sf_scalar"), 3, 1);
  FieldFunction ff_scalar("scalar_field_function", scalar_field, bulkData,
			  Dimensions(3), Dimensions(1));
  ff_scalar.interpolateFrom(sf_scalar);

  l2Norm(ff_scalar, fix.sfx_res);

  sfx_expect = pow(1./12., 1.5);
  EXPECT_DOUBLE_EQ_APPROX_TOL(sfx_expect, fix.sfx_res.getValue(), 1e-12);

  Util::setFlag(0, false);
}

TEST(norm, field_function)
{
  TEST_norm_field_function(TURBO_ELEMENT);
}
TEST(norm, field_function_bucket)
{
  TEST_norm_field_function(TURBO_BUCKET);
}
//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, h1_volume)
{
  EXCEPTWATCH;
  bool ret=false;
  if (ret) return;
  MPI_Barrier( MPI_COMM_WORLD );

  LocalFixture fix(3,3,12);
  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  //PerceptMesh& eMesh = fix.eMesh;

  stk::mesh::FieldBase *coords_field = fix.coords_field;

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated - here it is just the identity, and when integrated should produce the volume
  StringFunction identity("1.0", Name("identity"));
  std::string grad[] = {"0", "0", "0"};
  identity.set_gradient_strings(grad, 3);

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction sqrt_volume(0.0, "sqrt_volume");

  /// Create the operator that will do the work
  H1Norm h1Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  /// get the l2 norm of identity
  h1Norm(identity, sqrt_volume);

  EXPECT_DOUBLE_EQ_APPROX(1.0, sqrt_volume.getValue());
}

//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, h1_volume_1)
{
  EXCEPTWATCH;
  bool ret=false;
  if (ret) return;
  MPI_Barrier( MPI_COMM_WORLD );

  LocalFixture fix(3,3,12);
  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  PerceptMesh& eMesh = fix.eMesh;

  stk::mesh::FieldBase *coords_field = fix.coords_field;

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated - here it is just the identity, and when integrated should produce the volume
  StringFunction plane("x+2.0*y+3.0*z", Name("plane"));
  std::string grad[] = {"1.0", "2.0", "3.0"};
  plane.set_gradient_strings(grad, 3);

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction result1(0.0, "result1");

  /// Create the operator that will do the work
  H1Norm h1Norm(*bulkData, &metaData->universal_part(), TURBO_NONE);
  /// get the l2 norm of plane
  h1Norm(plane, result1);

  EXPECT_DOUBLE_EQ_APPROX(3.89444048184931, result1.getValue());

  //// rotate the mesh (result should be the same)

  if (1)
  {
    Math::Matrix rm = Math::rotationMatrix(0, 30);
    eMesh.transform_mesh(rm);
    eMesh.save_as("h1Norm_rotate.e");
    h1Norm(plane, result1);

    EXPECT_DOUBLE_EQ_APPROX(3.89444048184931, result1.getValue());
  }

}
//=============================================================================
//=============================================================================
//=============================================================================

TEST(norm, h1_volume_2)
{
  EXCEPTWATCH;
  bool ret=false;
  if (ret) return;
  MPI_Barrier( MPI_COMM_WORLD );

  LocalFixture fix(3,3,12, false, false);
  PerceptMesh& eMesh = fix.eMesh;

  int vectorDimension = 0;  // signifies a scalar field
  stk::mesh::FieldBase *f_test = eMesh.add_field("test", stk::topology::NODE_RANK, vectorDimension);
  eMesh.commit();
  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();

  stk::mesh::FieldBase *coords_field = fix.coords_field;

  /// create a field function from the existing coordinates field
  FieldFunction ff_coords("ff_coords", coords_field, bulkData,
                          Dimensions(3), Dimensions(3), FieldFunction::SIMPLE_SEARCH );

  /// the function to be integrated - here it is just the identity, and when integrated should produce the volume
  StringFunction plane("x+2.0*y+3.0*z", Name("plane"));
  std::string grad_str[] = {"1", "2", "3"};
  plane.set_gradient_strings(grad_str, 3);

  FieldFunction ff_plane("ff_plane", f_test, eMesh, 3, 1);
  ff_plane.interpolateFrom(plane);

  {
    MDArray in(3), out(1), grad(3);
    double x=0.21, y=0.32, z=0.43;
    in(0) = x; in(1) = y; in(2) = z;
    //ff_plane.gradient()
    ff_plane(in, out);
    std::cout << "in= " << in << std::endl;
    std::cout << "out= " << out << std::endl;
    (*(ff_plane.gradient()))(in, grad);
    std::cout << "grad= " << grad << std::endl;
    EXPECT_DOUBLE_EQ_APPROX(x+2*y+3*z, out(0));
    EXPECT_DOUBLE_EQ_APPROX(1.0, grad(0));
    EXPECT_DOUBLE_EQ_APPROX(2.0, grad(1));
    EXPECT_DOUBLE_EQ_APPROX(3.0, grad(2));
  }

  /// A place to hold the result.
  /// This is a "writable" function (we may want to make this explicit - StringFunctions are not writable; FieldFunctions are
  /// since we interpolate values to them from other functions).
  ConstantFunction result1(0.0, "result1");
  ConstantFunction result2(0.0, "result2");
  ConstantFunction result3(0.0, "result3");

  /// Create the operator that will do the work

  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), TURBO_ELEMENT);
  H1Norm h1Norm(*bulkData, &metaData->universal_part(), TURBO_ELEMENT);
  /// get the l2 norm of plane
  h1Norm(plane, result2);
  h1Norm(ff_plane, result1);

  EXPECT_DOUBLE_EQ_APPROX(result1.getValue(), result2.getValue());
  EXPECT_DOUBLE_EQ_APPROX(3.89444048184931, result1.getValue());

  //// rotate the mesh (result should be the same)

  if (1)
  {
    Math::Matrix rm = Math::rotationMatrix(0, 30);
    eMesh.transform_mesh(rm);
    ff_plane.interpolateFrom(plane);
    eMesh.save_as("h1Norm_rotate.e");
    h1Norm(ff_plane, result1);
    l2Norm(plane, result3);

    //EXPECT_DOUBLE_EQ_APPROX(3.89444048184931, result3.getValue());
    //if (!eMesh.get_rank()) std::cout << "RESULT3= " << result3.getValue() << std::endl;
    //if (!eMesh.get_rank()) std::cout << "RESULT1= " << result1.getValue() << std::endl;

    EXPECT_DOUBLE_EQ_APPROX(3.89444048184931, result1.getValue());
  }

}

//=============================================================================
//=============================================================================
//=============================================================================
TEST(norm, string_ff_diff)
{
  EXCEPTWATCH;

  LocalFixture fix(3,3,12, false, false);
  PerceptMesh& eMesh = fix.eMesh;

  int vectorDimension = 0;  // signifies a scalar field
  stk::mesh::FieldBase *scalar_field = eMesh.add_field("scalar", stk::topology::NODE_RANK, vectorDimension);
  eMesh.commit();

  stk::mesh::MetaData*        metaData     = fix.eMesh.get_fem_meta_data();
  stk::mesh::BulkData*        bulkData     = fix.eMesh.get_bulk_data();
  //PerceptMesh&        eMesh     = fix.eMesh;

  /// Create the operator that will do the work
  /// get the l2 norm
  Norm<2> l2Norm(*bulkData, &metaData->universal_part(), TURBO_ELEMENT);

  StringFunction sf_scalar( "x*y*z" , Name("sf_scalar"), 3, 1);
  FieldFunction ff_scalar("scalar_field_function", scalar_field, bulkData,
			  Dimensions(3), Dimensions(1));
  ff_scalar.interpolateFrom(sf_scalar);
  ff_scalar.add_alias("ffs");

  double sfx_expect = pow(1./12., 1.5);
  if (0)
    {
      l2Norm(ff_scalar, fix.sfx_res);
      EXPECT_DOUBLE_EQ_APPROX_TOL(sfx_expect, fix.sfx_res.getValue(), 1e-12);
      l2Norm(sf_scalar, fix.sfx_res);
      EXPECT_DOUBLE_EQ_APPROX_TOL(sfx_expect, fix.sfx_res.getValue(), 1e-12);
    }

  StringFunction sf_diff("sf_scalar - ffs", Name("sf_diff"), 3, 1);

  if (0)
    {
      std::cout << "sf_scalar(.1,.2,.3) = " << eval(0.1,0.2,0.3,0,sf_scalar) << " actual= " << (0.1*0.2*0.3)
                << "\nff_scalar(.1,.2,.3) = " << eval(0.1,0.2,0.3,0,ff_scalar) << " actual= " << (0.1*0.2*0.3)
                << "\nsf_diff(.1,.2,.3) = " << eval(0.1,0.2,0.3,0,sf_diff) 
                << std::endl;
    }

  l2Norm(sf_diff, fix.sfx_res);
  sfx_expect = 0.0;
  EXPECT_TRUE(std::fabs(sfx_expect - fix.sfx_res.getValue()) < 1e-12);
}

}
}
