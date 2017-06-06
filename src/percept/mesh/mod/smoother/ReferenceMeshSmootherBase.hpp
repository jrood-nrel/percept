// Copyright 2014 Sandia Corporation. Under the terms of
// Contract DE-AC04-94AL85000 with Sandia Corporation, the
// U.S. Government retains certain rights in this software.
//
// Use of this source code is governed by a BSD-style
// license that can be found in the LICENSE file.

#ifndef ReferenceMeshSmootherBase_hpp
#define ReferenceMeshSmootherBase_hpp

#include <percept/Percept.hpp>
#if !defined(NO_GEOM_SUPPORT)

#include <percept/mesh/mod/smoother/MeshSmoother.hpp>
#include <percept/mesh/mod/smoother/SmootherMetric.hpp>
#include <percept/mesh/mod/smoother/SmootherMetricUntangleGen.hpp>
#include <percept/mesh/mod/smoother/SmootherMetricShapeB1Gen.hpp>
#include <boost/unordered_map.hpp>
#include <stk_mesh/base/FieldParallel.hpp>
#include <percept/PerceptUtils.hpp>

namespace percept {

/// A smoother based on a reference mesh - tries to make the new mesh the same local size as original

template<typename MeshType>
class ReferenceMeshSmootherBaseImpl : public MeshSmootherImpl<MeshType> {

public:

	using Base = MeshSmootherImpl<MeshType>;

	typedef std::vector<double> Vector;
	typedef boost::unordered_map<typename MeshType::MTNode , Vector  > NodeMap;

	ReferenceMeshSmootherBaseImpl(PerceptMesh *eMesh,
			typename MeshType::MTSelector *boundary_selector=0,
			typename MeshType::MTMeshGeometry *meshGeometry=0,
			int inner_iterations = 100,
			double grad_norm =1.e-8,
			int parallel_iterations = 20);

protected:

	virtual int get_num_stages() { return 2; }
	virtual void run_algorithm();
	virtual double run_one_iteration();

	void sync_fields(int iter=0);
	virtual bool check_convergence();

	void project_all_delta_to_tangent_plane(typename MeshType::MTField *field);
	void check_project_all_delta_to_tangent_plane(typename MeshType::MTField *field);

	template<typename T>
	void check_equal(T& val)
	{
		T global_min = val, global_max=val;
		stk::all_reduce( Base::m_eMesh->get_bulk_data()->parallel() , stk::ReduceMax<1>( & global_max ) );
		stk::all_reduce( Base::m_eMesh->get_bulk_data()->parallel() , stk::ReduceMax<1>( & global_max ) );
		VERIFY_OP_ON( global_max, ==, val , "bad parallel val");
		VERIFY_OP_ON( global_min, ==, val , "bad parallel val");
		VERIFY_OP_ON( global_max, ==, global_min , "bad parallel val");
	}

public:
	NodeMap m_current_position;
	NodeMap m_delta;
	NodeMap m_weight;
	NodeMap m_nweight;

	double m_scale;
	typedef long double Double;
	Double m_dmax;
	Double m_dmax_relative;
	Double m_dnew, m_dold, m_d0, m_dmid, m_dd, m_alpha, m_alpha_0, m_grad_norm, m_grad_norm_scaled;
	Double m_total_metric;
	int m_stage;
	double m_omega;
	double m_omega_prev;
	int m_iter;

	size_t m_num_invalid;
	Double m_global_metric;
	bool m_untangled;
	size_t m_num_nodes;

	typename MeshType::MTField *m_coord_field_original;
	typename MeshType::MTField *m_coord_field_projected;
	typename MeshType::MTField *m_coord_field_current;
	typename MeshType::MTField *m_coord_field_lagged;

	SmootherMetricImpl<MeshType> *m_metric;
public:
	bool m_use_ref_mesh;
	int m_do_animation;
};

using ReferenceMeshSmootherBase = ReferenceMeshSmootherBaseImpl<STKMesh>;
}

#endif
#endif
