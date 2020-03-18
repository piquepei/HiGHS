# Define IPM object library.

set(basiclu_sources
    basiclu/src/basiclu_factorize.c
    basiclu/src/basiclu_solve_dense.c
    basiclu/src/lu_build_factors.c
    basiclu/src/lu_factorize_bump.c
    basiclu/src/lu_initialize.c
    basiclu/src/lu_markowitz.c
    basiclu/src/lu_setup_bump.c
    basiclu/src/lu_solve_sparse.c
    basiclu/src/basiclu_get_factors.c
    basiclu/src/basiclu_solve_for_update.c
    basiclu/src/lu_condest.c
    basiclu/src/lu_file.c
    basiclu/src/lu_internal.c
    basiclu/src/lu_matrix_norm.c
    basiclu/src/lu_singletons.c
    basiclu/src/lu_solve_symbolic.c
    basiclu/src/lu_update.c
    basiclu/src/basiclu_initialize.c
    basiclu/src/basiclu_solve_sparse.c
    basiclu/src/lu_pivot.c
    basiclu/src/lu_solve_dense.c
    basiclu/src/lu_solve_triangular.c
    basiclu/src/basiclu_object.c
    basiclu/src/basiclu_update.c
    basiclu/src/lu_dfs.c
    basiclu/src/lu_garbage_perm.c
    basiclu/src/lu_residual_test.c
    basiclu/src/lu_solve_for_update.c
    basiclu/src/lu_timer.c)

set(ipx_sources
    ipx/src/basiclu_kernel.cc
    ipx/src/basiclu_wrapper.cc
    ipx/src/basis.cc
    ipx/src/conjugate_residuals.cc
    ipx/src/control.cc
    ipx/src/crossover.cc
    ipx/src/diagonal_precond.cc
    ipx/src/forrest_tomlin.cc
    ipx/src/guess_basis.cc
    ipx/src/indexed_vector.cc
    ipx/src/info.cc
    ipx/src/ipm.cc
    ipx/src/ipx_c.cc
    ipx/src/iterate.cc
    ipx/src/kkt_solver.cc
    ipx/src/kkt_solver_basis.cc
    ipx/src/kkt_solver_diag.cc
    ipx/src/linear_operator.cc
    ipx/src/lp_solver.cc
    ipx/src/lu_factorization.cc
    ipx/src/lu_update.cc
    ipx/src/maxvolume.cc
    ipx/src/model.cc
    ipx/src/normal_matrix.cc
    ipx/src/sparse_matrix.cc
    ipx/src/sparse_utils.cc
    ipx/src/splitted_normal_matrix.cc
    ipx/src/starting_basis.cc
    ipx/src/symbolic_invert.cc
    ipx/src/timer.cc
    ipx/src/utils.cc)

set(ipx_headers
    ipx/include/ipx_c.h ipx/include/ipx_config.h ipx/include/ipx_info.h
    ipx/include/ipx_parameters.h ipx/include/ipx_status.h)

set(basiclu_headers
    basiclu/include/basiclu_factorize.h
    basiclu/include/basiclu_obj_factorize.h
    basiclu/include/basiclu_obj_solve_for_update.h
    basiclu/include/basiclu_solve_sparse.h
    basiclu/include/basiclu_get_factors.h
    basiclu/include/basiclu_obj_free.h
    basiclu/include/basiclu_obj_solve_sparse.h
    basiclu/include/basiclu_update.h
    basiclu/include/basiclu.h
    basiclu/include/basiclu_obj_get_factors.h
    basiclu/include/basiclu_obj_update.h
    basiclu/include/basiclu_initialize.h
    basiclu/include/basiclu_obj_initialize.h
    basiclu/include/basiclu_solve_dense.h
    basiclu/include/basiclu_object.h
    basiclu/include/basiclu_obj_solve_dense.h
    basiclu/include/basiclu_solve_for_update.h)

add_library(libipm OBJECT "")

target_sources(libipm PRIVATE ${ipx_sources} ${basiclu_sources} ${ipx_headers}
                              ${basiclu_headers})

target_include_directories(
  libipm
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ipx/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include>
         $<INSTALL_INTERFACE:include>
  PRIVATE ipx/include basiclu/include)

set_target_properties(libipm PROPERTIES POSITION_INDEPENDENT_CODE on)

install(
  TARGETS libipm
  EXPORT highs-targets
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib
  INCLUDES
  DESTINATION include)

export(TARGETS libipm FILE "${HIGHS_BINARY_DIR}/highs-targets.cmake")
