# Define library.

set(basiclu_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_factorize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_solve_dense.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_build_factors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_factorize_bump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_initialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_markowitz.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_setup_bump.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_solve_sparse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_get_factors.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_solve_for_update.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_condest.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_file.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_internal.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_matrix_norm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_singletons.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_solve_symbolic.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_update.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_initialize.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_solve_sparse.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_pivot.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_solve_dense.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_solve_triangular.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_object.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/basiclu_update.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_dfs.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_garbage_perm.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_residual_test.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_solve_for_update.c
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/src/lu_timer.c)

set(ipx_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/basiclu_kernel.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/basiclu_wrapper.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/basis.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/conjugate_residuals.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/control.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/crossover.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/diagonal_precond.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/forrest_tomlin.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/guess_basis.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/indexed_vector.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/info.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/ipm.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/ipx_c.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/iterate.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/kkt_solver.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/kkt_solver_basis.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/kkt_solver_diag.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/linear_operator.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/lp_solver.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/lu_factorization.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/lu_update.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/maxvolume.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/model.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/normal_matrix.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/sparse_matrix.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/sparse_utils.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/splitted_normal_matrix.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/starting_basis.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/symbolic_invert.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/timer.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/src/utils.cc)

set(ipx_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/include/ipx_c.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/include/ipx_config.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/include/ipx_info.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/include/ipx_parameters.h
    ${CMAKE_CURRENT_SOURCE_DIR}/ipx/include/ipx_status.h)

set(basiclu_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_factorize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_factorize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_solve_for_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_solve_sparse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_get_factors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_free.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_solve_sparse.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_get_factors.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_initialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_initialize.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_solve_dense.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_obj_solve_dense.h
    ${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include/basiclu_solve_for_update.h)

add_library(libipm "")

target_sources(libipm PRIVATE ${ipx_sources} ${basiclu_sources} ${ipx_headers}
                              ${basiclu_headers})

target_include_directories(
  libipm
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ipx/include>
         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/basiclu/include>
         $<INSTALL_INTERFACE:ipx/include>
         $<INSTALL_INTERFACE:basiclu/include>)

set_target_properties(
  libipm PROPERTIES OUTPUT_NAME "ipm" MACOSX_RPATH
                                      "${CMAKE_INSTALL_PREFIX}/lib")

set_target_properties(
  libipm PROPERTIES OUTPUT_NAME "ipx" MACOSX_RPATH
                                      "${CMAKE_INSTALL_PREFIX}/lib")
