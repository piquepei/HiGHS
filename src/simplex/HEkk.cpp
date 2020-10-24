/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/*                                                                       */
/*    This file is part of the HiGHS linear optimization suite           */
/*                                                                       */
/*    Written and engineered 2008-2020 at the University of Edinburgh    */
/*                                                                       */
/*    Available as open-source under the MIT License                     */
/*                                                                       */
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/**@file simplex/HEkk.cpp
 * @brief
 * @author Julian Hall, Ivet Galabova, Qi Huangfu and Michael Feldmeier
 */
//#include <cassert>
////#include <iostream>

#include "simplex/HEkk.h"

#include "io/HighsIO.h"
#include "lp_data/HighsModelUtils.h"
#include "simplex/HEkkPrimal.h"
#include "simplex/HFactorDebug.h"
#include "simplex/HighsSimplexAnalysis.h"
#include "simplex/SimplexTimer.h"
#include "util/HighsRandom.h"

using std::cout;
using std::endl;

HighsStatus HEkk::passLp(const HighsLp& lp) {
  simplex_lp_ = lp;
  initialise();
  return HighsStatus::OK;
}

HighsStatus HEkk::initialiseSimplexLpBasisAndFactor() {
  setSimplexOptions();
  initialiseSimplexLpRandomVectors();
  initialiseAnalysis();
  setBasis();
  const int rank_deficiency = getFactor();
  if (rank_deficiency) return HighsStatus::Error;
  setNonbasicMove();
  simplex_lp_status_.has_basis = true;
  return HighsStatus::OK;
}

HighsStatus HEkk::solve() {
  HighsLogMessage(
      options_.logfile, HighsMessageType::INFO,
      "HEkk::solve called for LP with %d columns, %d rows and %d entries",
      simplex_lp_.numCol_, simplex_lp_.numRow_,
      simplex_lp_.Astart_[simplex_lp_.numCol_]);

  if (initialise() == HighsStatus::Error) return HighsStatus::Error;
  assert(simplex_lp_status_.has_basis);
  assert(simplex_lp_status_.has_invert);
  assert(simplex_lp_status_.valid);

  HighsLogMessage(options_.logfile, HighsMessageType::INFO,
                  "Initial basis has"
                  " Primal: objective = %g; Infeasibilities %d / %g /%g;"
                  " Dual: objective = %g; Infeasibilities %d / %g /%g",
                  simplex_info_.primal_objective_value,
                  simplex_info_.num_primal_infeasibilities,
                  simplex_info_.max_primal_infeasibility,
                  simplex_info_.sum_primal_infeasibilities,
                  simplex_info_.dual_objective_value,
                  simplex_info_.num_dual_infeasibilities,
                  simplex_info_.max_dual_infeasibility,
                  simplex_info_.sum_dual_infeasibilities);
  if (scaled_model_status_ == HighsModelStatus::OPTIMAL) return HighsStatus::OK;
  HEkkPrimal primal(*this);
  HighsStatus return_status = primal.solve();
  return return_status;
}

HighsSolutionParams HEkk::getSolutionParams() {
  HighsSolutionParams solution_params;
  solution_params.primal_feasibility_tolerance =
      options_.primal_feasibility_tolerance;
  solution_params.dual_feasibility_tolerance =
      options_.dual_feasibility_tolerance;
  if (scaled_model_status_ == HighsModelStatus::OPTIMAL) {
    solution_params.primal_status = PrimalDualStatus::STATUS_FEASIBLE_POINT;
    solution_params.dual_status = PrimalDualStatus::STATUS_FEASIBLE_POINT;
  } else {
    solution_params.primal_status = PrimalDualStatus::STATUS_NOTSET;
    solution_params.dual_status = PrimalDualStatus::STATUS_NOTSET;
  }
  // Output from solution analysis method
  solution_params.objective_function_value =
      simplex_info_.primal_objective_value;
  solution_params.num_primal_infeasibilities =
      simplex_info_.num_primal_infeasibilities;
  solution_params.max_primal_infeasibility =
      simplex_info_.max_primal_infeasibility;
  solution_params.sum_primal_infeasibilities =
      simplex_info_.sum_primal_infeasibilities;
  solution_params.num_dual_infeasibilities =
      simplex_info_.num_dual_infeasibilities;
  solution_params.max_dual_infeasibility = simplex_info_.max_dual_infeasibility;
  solution_params.sum_dual_infeasibilities =
      simplex_info_.sum_dual_infeasibilities;
  return solution_params;
}

// Private methods

HighsStatus HEkk::initialise() {
  if (initialiseSimplexLpBasisAndFactor() == HighsStatus::Error)
    return HighsStatus::Error;
  initialiseMatrix();
  allocateWorkAndBaseArrays();
  initialiseCost();
  initialiseBound();
  initialiseNonbasicWorkValue();
  computePrimal();
  computeDual();
  computeSimplexInfeasible();
  computeDualObjectiveValue();
  computePrimalObjectiveValue();
  simplex_lp_status_.valid = true;

  bool primal_feasible = simplex_info_.num_primal_infeasibilities == 0;
  bool dual_feasible = simplex_info_.num_dual_infeasibilities == 0;
  scaled_model_status_ = HighsModelStatus::NOTSET;
  if (primal_feasible && dual_feasible)
    scaled_model_status_ = HighsModelStatus::OPTIMAL;

  //  if (debugSimplexBasicSolution("After transition", ekk_instance_) ==
  //  HighsDebugStatus::LOGICAL_ERROR) return HighsStatus::Error;
  return HighsStatus::OK;
}

void HEkk::setSimplexOptions() {
  // Copy values of HighsOptions for the simplex solver
  // Currently most of these options are straight copies, but they
  // will become valuable when "choose" becomes a HiGHS strategy value
  // that will need converting into a specific simplex strategy value.
  //
  simplex_info_.simplex_strategy = options_.simplex_strategy;
  simplex_info_.dual_edge_weight_strategy =
      options_.simplex_dual_edge_weight_strategy;
  simplex_info_.price_strategy = options_.simplex_price_strategy;
  simplex_info_.dual_simplex_cost_perturbation_multiplier =
      options_.dual_simplex_cost_perturbation_multiplier;
  simplex_info_.factor_pivot_threshold = options_.factor_pivot_threshold;
  simplex_info_.update_limit = options_.simplex_update_limit;

  // Set values of internal options
  simplex_info_.store_squared_primal_infeasibility = true;
  // Option for analysing the LP solution
#ifdef HiGHSDEV
  bool useful_analysis = false;  // true;  //
  bool full_timing = false;
  // Options for reporting timing
  simplex_info_.report_simplex_inner_clock = useful_analysis;  // true;  //
  simplex_info_.report_simplex_outer_clock = full_timing;
  simplex_info_.report_simplex_phases_clock = full_timing;
  simplex_info_.report_HFactor_clock = useful_analysis;  // full_timing;//
  // Options for analysing the LP and simplex iterations
  simplex_info_.analyse_lp = useful_analysis;  // false;  //
  simplex_info_.analyse_iterations = useful_analysis;
  simplex_info_.analyse_invert_form = useful_analysis;
  //  simplex_info_.analyse_invert_condition = useful_analysis;
  simplex_info_.analyse_invert_time = full_timing;
  simplex_info_.analyse_rebuild_time = full_timing;
#endif
}

void HEkk::initialiseSimplexLpRandomVectors() {
  const int numCol = simplex_lp_.numCol_;
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  if (!numTot) return;
  // Instantiate and (re-)initialise the random number generator
  HighsRandom random;
  random.initialise();

  if (numCol) {
    // Generate a random permutation of the column indices
    simplex_info_.numColPermutation_.resize(numCol);
    vector<int>& numColPermutation = simplex_info_.numColPermutation_;
    for (int i = 0; i < numCol; i++) numColPermutation[i] = i;
    for (int i = numCol - 1; i >= 1; i--) {
      int j = random.integer() % (i + 1);
      std::swap(numColPermutation[i], numColPermutation[j]);
    }
  }

  // Re-initialise the random number generator and generate the
  // random vectors in the same order as hsol to maintain repeatable
  // performance
  random.initialise();
  //
  // Generate a random permutation of all the indices
  simplex_info_.numTotPermutation_.resize(numTot);
  vector<int>& numTotPermutation = simplex_info_.numTotPermutation_;
  for (int i = 0; i < numTot; i++) numTotPermutation[i] = i;
  for (int i = numTot - 1; i >= 1; i--) {
    int j = random.integer() % (i + 1);
    std::swap(numTotPermutation[i], numTotPermutation[j]);
  }

  // Generate a vector of random reals
  simplex_info_.numTotRandomValue_.resize(numTot);
  vector<double>& numTotRandomValue = simplex_info_.numTotRandomValue_;
  for (int i = 0; i < numTot; i++) {
    numTotRandomValue[i] = random.fraction();
  }
}

bool HEkk::setBasis() {
  int num_col = simplex_lp_.numCol_;
  int num_row = simplex_lp_.numRow_;
  int num_tot = num_col + num_row;
  simplex_basis_.nonbasicFlag_.resize(num_tot);
  simplex_basis_.nonbasicMove_.resize(num_tot);
  simplex_basis_.basicIndex_.resize(num_row);
  for (int iCol = 0; iCol < num_col; iCol++)
    simplex_basis_.nonbasicFlag_[iCol] = NONBASIC_FLAG_TRUE;
  for (int iRow = 0; iRow < num_row; iRow++) {
    int iVar = num_col + iRow;
    simplex_basis_.nonbasicFlag_[iVar] = NONBASIC_FLAG_FALSE;
    simplex_basis_.basicIndex_[iRow] = iVar;
  }
  simplex_info_.num_basic_logicals = num_row;
  simplex_lp_status_.has_basis = true;
  return true;
}

int HEkk::getFactor() {
  if (!simplex_lp_status_.has_factor_arrays) {
    assert(simplex_info_.factor_pivot_threshold >=
           options_.factor_pivot_threshold);
    factor_.setup(simplex_lp_.numCol_, simplex_lp_.numRow_,
                  &simplex_lp_.Astart_[0], &simplex_lp_.Aindex_[0],
                  &simplex_lp_.Avalue_[0], &simplex_basis_.basicIndex_[0],
                  options_.highs_debug_level, options_.logfile, options_.output,
                  options_.message_level, simplex_info_.factor_pivot_threshold,
                  options_.factor_pivot_tolerance);
    simplex_lp_status_.has_factor_arrays = true;
  }
  if (!simplex_lp_status_.has_invert) {
    analysis_.simplexTimerStart(InvertClock);
    const int rank_deficiency = computeFactor();
    analysis_.simplexTimerStop(InvertClock);
    if (rank_deficiency) {
      // Basis is rank deficient
      return rank_deficiency;
      /*
        if (only_from_known_basis) {
          // If only this basis should be used, then return error
          HighsLogMessage(options_.logfile, HighsMessageType::ERROR,
                          "Supposed to be a full-rank basis, but incorrect");
          return rank_deficiency;
        } else {
          // Account for rank deficiency by correcing nonbasicFlag
          simplexHandleRankDeficiency();
          updateSimplexLpStatus(simplex_lp_status_, LpAction::NEW_BASIS);
          simplex_lp_status_.has_invert = true;
          simplex_lp_status_.has_fresh_invert = true;
        }
      */
    }
    assert(simplex_lp_status_.has_invert);
  }
  return 0;
}

void HEkk::computePrimalObjectiveValue() {
  analysis_.simplexTimerStart(ComputePrObjClock);
  simplex_info_.primal_objective_value = 0;
  for (int iRow = 0; iRow < simplex_lp_.numRow_; iRow++) {
    int iVar = simplex_basis_.basicIndex_[iRow];
    if (iVar < simplex_lp_.numCol_) {
      simplex_info_.primal_objective_value +=
          simplex_info_.baseValue_[iRow] * simplex_lp_.colCost_[iVar];
    }
  }
  for (int iCol = 0; iCol < simplex_lp_.numCol_; iCol++) {
    if (simplex_basis_.nonbasicFlag_[iCol])
      simplex_info_.primal_objective_value +=
          simplex_info_.workValue_[iCol] * simplex_lp_.colCost_[iCol];
  }
  simplex_info_.primal_objective_value *= cost_scale_;
  // Objective value calculation is done using primal values and
  // original costs so offset is vanilla
  simplex_info_.primal_objective_value += simplex_lp_.offset_;
  // Now have primal objective value
  simplex_lp_status_.has_primal_objective_value = true;
  analysis_.simplexTimerStop(ComputePrObjClock);
}

void HEkk::computeDualObjectiveValue(const int phase) {
  analysis_.simplexTimerStart(ComputeDuObjClock);
  simplex_info_.dual_objective_value = 0;
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  for (int i = 0; i < numTot; i++) {
    if (simplex_basis_.nonbasicFlag_[i]) {
      const double term =
          simplex_info_.workValue_[i] * simplex_info_.workDual_[i];
      if (term) {
        simplex_info_.dual_objective_value +=
            simplex_info_.workValue_[i] * simplex_info_.workDual_[i];
      }
    }
  }
  simplex_info_.dual_objective_value *= cost_scale_;
  if (phase != 1) {
    // In phase 1 the dual objective has no objective
    // shift. Otherwise, if minimizing the shift is added. If
    // maximizing, workCost (and hence workDual) are negated, so the
    // shift is subtracted. Hence the shift is added according to the
    // sign implied by sense_
    simplex_info_.dual_objective_value +=
        ((int)simplex_lp_.sense_) * simplex_lp_.offset_;
  }
  // Now have dual objective value
  simplex_lp_status_.has_dual_objective_value = true;
  analysis_.simplexTimerStop(ComputeDuObjClock);
}

int HEkk::computeFactor() {
  HighsTimerClock* factor_timer_clock_pointer = NULL;
  // TODO Understand why handling noPvC and noPvR in what seem to be
  // different ways ends up equivalent.
#ifdef HiGHSDEV
  int thread_id = 0;
#ifdef OPENMP
  thread_id = omp_get_thread_num();
  //  printf("Hello world from computeFactor: thread %d\n", thread_id);
#endif
  factor_timer_clock_pointer =
      analysis_.getThreadFactorTimerClockPtr(thread_id);
#endif
  const int rank_deficiency = factor_.build(factor_timer_clock_pointer);
#ifdef HiGHSDEV
  if (simplex_info_.analyse_invert_form) {
    const bool report_kernel = false;
    simplex_info_.num_invert++;
    assert(factor_.basis_matrix_num_el);
    double invert_fill_factor =
        ((1.0 * factor_.invert_num_el) / factor_.basis_matrix_num_el);
    if (report_kernel) printf("INVERT fill = %6.2f", invert_fill_factor);
    simplex_info_.sum_invert_fill_factor += invert_fill_factor;
    simplex_info_.running_average_invert_fill_factor =
        0.95 * simplex_info_.running_average_invert_fill_factor +
        0.05 * invert_fill_factor;

    double kernel_relative_dim =
        (1.0 * factor_.kernel_dim) / simplex_lp_.numRow_;
    if (report_kernel) printf("; kernel dim = %11.4g", kernel_relative_dim);
    if (factor_.kernel_dim) {
      simplex_info_.num_kernel++;
      simplex_info_.max_kernel_dim =
          max(kernel_relative_dim, simplex_info_.max_kernel_dim);
      simplex_info_.sum_kernel_dim += kernel_relative_dim;
      simplex_info_.running_average_kernel_dim =
          0.95 * simplex_info_.running_average_kernel_dim +
          0.05 * kernel_relative_dim;

      int kernel_invert_num_el =
          factor_.invert_num_el -
          (factor_.basis_matrix_num_el - factor_.kernel_num_el);
      assert(factor_.kernel_num_el);
      double kernel_fill_factor =
          (1.0 * kernel_invert_num_el) / factor_.kernel_num_el;
      simplex_info_.sum_kernel_fill_factor += kernel_fill_factor;
      simplex_info_.running_average_kernel_fill_factor =
          0.95 * simplex_info_.running_average_kernel_fill_factor +
          0.05 * kernel_fill_factor;
      if (report_kernel) printf("; fill = %6.2f", kernel_fill_factor);
      if (kernel_relative_dim >
          simplex_info_.major_kernel_relative_dim_threshold) {
        simplex_info_.num_major_kernel++;
        simplex_info_.sum_major_kernel_fill_factor += kernel_fill_factor;
        simplex_info_.running_average_major_kernel_fill_factor =
            0.95 * simplex_info_.running_average_major_kernel_fill_factor +
            0.05 * kernel_fill_factor;
      }
    }
    if (report_kernel) printf("\n");
  }
  if (simplex_info_.analyse_invert_condition) {
    analysis_.simplexTimerStart(BasisConditionClock);
    simplex_info_.invert_condition = computeBasisCondition();
    analysis_.simplexTimerStop(BasisConditionClock);
  }
#endif

  const bool force = rank_deficiency;
  debugCheckInvert(options_, factor_, force);

  if (rank_deficiency) {
    // Have an invertible representation, but of B with column(s)
    // replacements due to singularity. So no (fresh) representation of
    // B^{-1}
    simplex_lp_status_.has_invert = false;
    simplex_lp_status_.has_fresh_invert = false;
  } else {
    // Now have a representation of B^{-1}, and it is fresh!
    simplex_lp_status_.has_invert = true;
    simplex_lp_status_.has_fresh_invert = true;
  }
  // Set the update count to zero since the corrected invertible
  // representation may be used for an initial basis. In any case the
  // number of updates shouldn't be positive
  simplex_info_.update_count = 0;

  return rank_deficiency;
}

void HEkk::initialiseMatrix() {
  if (!simplex_lp_status_.has_matrix_col_wise ||
      !simplex_lp_status_.has_matrix_row_wise) {
    analysis_.simplexTimerStart(matrixSetupClock);
    matrix_.setup(simplex_lp_.numCol_, simplex_lp_.numRow_,
                  &simplex_lp_.Astart_[0], &simplex_lp_.Aindex_[0],
                  &simplex_lp_.Avalue_[0], &simplex_basis_.nonbasicFlag_[0]);
    simplex_lp_status_.has_matrix_col_wise = true;
    simplex_lp_status_.has_matrix_row_wise = true;
    analysis_.simplexTimerStop(matrixSetupClock);
  }
}

void HEkk::setNonbasicMove() {
  simplex_basis_.nonbasicMove_.resize(simplex_lp_.numCol_ +
                                      simplex_lp_.numRow_);
  const bool have_solution = false;
  // Don't have a simplex basis since nonbasicMove is not set up.
  const int illegal_move_value = -99;

  // Assign nonbasicMove using as much information as is available
  double lower;
  double upper;
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;

  for (int iVar = 0; iVar < numTot; iVar++) {
    if (!simplex_basis_.nonbasicFlag_[iVar]) {
      // Basic variable
      simplex_basis_.nonbasicMove_[iVar] = 0;
      continue;
    }
    // Nonbasic variable
    if (iVar < simplex_lp_.numCol_) {
      lower = simplex_lp_.colLower_[iVar];
      upper = simplex_lp_.colUpper_[iVar];
    } else {
      int iRow = iVar - simplex_lp_.numCol_;
      lower = -simplex_lp_.rowUpper_[iRow];
      upper = -simplex_lp_.rowLower_[iRow];
    }
    int move = illegal_move_value;
    if (lower == upper) {
      // Fixed
      move = NONBASIC_MOVE_ZE;
    } else if (!highs_isInfinity(-lower)) {
      // Finite lower bound so boxed or lower
      if (!highs_isInfinity(upper)) {
        // Finite upper bound so boxed
        //
        // Determine the bound to set the value to according to, in order of
        // priority
        //
        // 1. Any solution value
        if (have_solution) {
          double midpoint = 0.5 * (lower + upper);
          double value = simplex_info_.workValue_[iVar];
          if (value < midpoint) {
            move = NONBASIC_MOVE_UP;
          } else {
            move = NONBASIC_MOVE_DN;
          }
        }
        // 2. Bound of original LP that is closer to zero
        if (move == illegal_move_value) {
          if (fabs(lower) < fabs(upper)) {
            move = NONBASIC_MOVE_UP;
          } else {
            move = NONBASIC_MOVE_DN;
          }
        }
      } else {
        // Lower (since upper bound is infinite)
        move = NONBASIC_MOVE_UP;
      }
    } else if (!highs_isInfinity(upper)) {
      // Upper
      move = NONBASIC_MOVE_DN;
    } else {
      // FREE
      move = NONBASIC_MOVE_ZE;
    }
    assert(move != illegal_move_value);
    simplex_basis_.nonbasicMove_[iVar] = move;
  }
}

void HEkk::allocateWorkAndBaseArrays() {
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  simplex_info_.workCost_.resize(numTot);
  simplex_info_.workDual_.resize(numTot);
  simplex_info_.workShift_.resize(numTot);

  simplex_info_.workLower_.resize(numTot);
  simplex_info_.workUpper_.resize(numTot);
  simplex_info_.workRange_.resize(numTot);
  simplex_info_.workValue_.resize(numTot);

  // Feel that it should be possible to resize this with in dual
  // solver, and only if Devex is being used, but a pointer to it
  // needs to be set up when constructing HDual
  simplex_info_.devex_index_.resize(numTot);

  simplex_info_.baseLower_.resize(simplex_lp_.numRow_);
  simplex_info_.baseUpper_.resize(simplex_lp_.numRow_);
  simplex_info_.baseValue_.resize(simplex_lp_.numRow_);
}

void HEkk::initialisePhase2ColBound() {
  for (int iCol = 0; iCol < simplex_lp_.numCol_; iCol++) {
    simplex_info_.workLower_[iCol] = simplex_lp_.colLower_[iCol];
    simplex_info_.workUpper_[iCol] = simplex_lp_.colUpper_[iCol];
    simplex_info_.workRange_[iCol] =
        simplex_info_.workUpper_[iCol] - simplex_info_.workLower_[iCol];
  }
}

void HEkk::initialisePhase2RowBound() {
  for (int row = 0; row < simplex_lp_.numRow_; row++) {
    int var = simplex_lp_.numCol_ + row;
    simplex_info_.workLower_[var] = -simplex_lp_.rowUpper_[row];
    simplex_info_.workUpper_[var] = -simplex_lp_.rowLower_[row];
    simplex_info_.workRange_[var] =
        simplex_info_.workUpper_[var] - simplex_info_.workLower_[var];
  }
}

void HEkk::initialiseBound(const int phase) {
  initialisePhase2ColBound();
  initialisePhase2RowBound();
  if (phase == 2) return;

  // The dual objective is the sum of products of primal and dual
  // values for nonbasic variables. For dual simplex phase 1, the
  // primal bounds are set so that when the dual value is feasible, the
  // primal value is set to zero. Otherwise the value is +1/-1
  // according to the required sign of the dual, except for free
  // variables, where the bounds are [-1000, 1000]. Hence the dual
  // objective is the negation of the sum of infeasibilities, unless there are
  // free In Phase 1: change to dual phase 1 bound.
  const double inf = HIGHS_CONST_INF;
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  for (int i = 0; i < numTot; i++) {
    if (simplex_info_.workLower_[i] == -inf &&
        simplex_info_.workUpper_[i] == inf) {
      // Don't change for row variables: they should never become
      // nonbasic when starting from a logical basis, and no crash
      // should make a free row nonbasic, but could an advanced basis
      // make a free row nonbasic.
      // But what it it happened?
      if (i >= simplex_lp_.numCol_) continue;
      simplex_info_.workLower_[i] = -1000,
      simplex_info_.workUpper_[i] = 1000;  // FREE
    } else if (simplex_info_.workLower_[i] == -inf) {
      simplex_info_.workLower_[i] = -1,
      simplex_info_.workUpper_[i] = 0;  // UPPER
    } else if (simplex_info_.workUpper_[i] == inf) {
      simplex_info_.workLower_[i] = 0,
      simplex_info_.workUpper_[i] = 1;  // LOWER
    } else {
      simplex_info_.workLower_[i] = 0,
      simplex_info_.workUpper_[i] = 0;  // BOXED or FIXED
    }
    simplex_info_.workRange_[i] =
        simplex_info_.workUpper_[i] - simplex_info_.workLower_[i];
  }
}

void HEkk::initialisePhase2ColCost() {
  for (int col = 0; col < simplex_lp_.numCol_; col++) {
    int var = col;
    simplex_info_.workCost_[var] =
        (int)simplex_lp_.sense_ * simplex_lp_.colCost_[col];
    simplex_info_.workShift_[var] = 0;
  }
}

void HEkk::initialisePhase2RowCost() {
  for (int iVar = simplex_lp_.numCol_;
       iVar < simplex_lp_.numCol_ + simplex_lp_.numRow_; iVar++) {
    simplex_info_.workCost_[iVar] = 0;
    simplex_info_.workShift_[iVar] = 0;
  }
}

void HEkk::initialiseCost(const int perturb) {
  // Copy the cost
  initialisePhase2ColCost();
  initialisePhase2RowCost();
  // See if we want to skip perturbation
  simplex_info_.costs_perturbed = 0;
  if (perturb == 0 ||
      simplex_info_.dual_simplex_cost_perturbation_multiplier == 0)
    return;
  simplex_info_.costs_perturbed = 1;

  // Perturb the original costs, scale down if is too big
#ifdef HiGHSDEV
  printf("grep_DuPtrb: Cost perturbation for %s\n",
         simplex_lp_.model_name_.c_str());
  int num_original_nonzero_cost = 0;
#endif
  double bigc = 0;
  for (int i = 0; i < simplex_lp_.numCol_; i++) {
    const double abs_cost = fabs(simplex_info_.workCost_[i]);
    bigc = max(bigc, abs_cost);
#ifdef HiGHSDEV
    if (abs_cost) num_original_nonzero_cost++;
#endif
  }
#ifdef HiGHSDEV
  const int pct0 = (100 * num_original_nonzero_cost) / simplex_lp_.numCol_;
  double average_cost = 0;
  if (num_original_nonzero_cost) {
    average_cost = bigc / num_original_nonzero_cost;
  } else {
    printf("grep_DuPtrb:    STRANGE initial workCost has non nonzeros\n");
  }
  printf(
      "grep_DuPtrb:    Initially have %d nonzero costs (%3d%%) with bigc = %g "
      "and average = %g\n",
      num_original_nonzero_cost, pct0, bigc, average_cost);
#endif
  if (bigc > 100) {
    bigc = sqrt(sqrt(bigc));
#ifdef HiGHSDEV
    printf("grep_DuPtrb:    Large so set bigc = sqrt(bigc) = %g\n", bigc);
#endif
  }

  // If there's few boxed variables, we will just use simple perturbation
  double boxedRate = 0;
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  for (int i = 0; i < numTot; i++)
    boxedRate += (simplex_info_.workRange_[i] < 1e30);
  boxedRate /= numTot;
  if (boxedRate < 0.01) {
    bigc = min(bigc, 1.0);
#ifdef HiGHSDEV
    printf(
        "grep_DuPtrb:    small boxedRate (%g) so set bigc = min(bigc, 1.0) = "
        "%g\n",
        boxedRate, bigc);
#endif
  }
  // Determine the perturbation base
  double base = 5e-7 * bigc;
#ifdef HiGHSDEV
  printf("grep_DuPtrb:    Perturbation base = %g\n", base);
#endif

  // Now do the perturbation
  for (int i = 0; i < simplex_lp_.numCol_; i++) {
    double lower = simplex_lp_.colLower_[i];
    double upper = simplex_lp_.colUpper_[i];
    double xpert = (fabs(simplex_info_.workCost_[i]) + 1) * base *
                   simplex_info_.dual_simplex_cost_perturbation_multiplier *
                   (1 + simplex_info_.numTotRandomValue_[i]);
#ifdef HiGHSDEV
    const double previous_cost = simplex_info_.workCost_[i];
#endif
    if (lower <= -HIGHS_CONST_INF && upper >= HIGHS_CONST_INF) {
      // Free - no perturb
    } else if (upper >= HIGHS_CONST_INF) {  // Lower
      simplex_info_.workCost_[i] += xpert;
    } else if (lower <= -HIGHS_CONST_INF) {  // Upper
      simplex_info_.workCost_[i] += -xpert;
    } else if (lower != upper) {  // Boxed
      simplex_info_.workCost_[i] +=
          (simplex_info_.workCost_[i] >= 0) ? xpert : -xpert;
    } else {
      // Fixed - no perturb
    }
#ifdef HiGHSDEV
    const double perturbation1 =
        fabs(simplex_info_.workCost_[i] - previous_cost);
    if (perturbation1)
      updateValueDistribution(perturbation1,
                              analysis_.cost_perturbation1_distribution);
#endif
  }
  for (int i = simplex_lp_.numCol_; i < numTot; i++) {
    double perturbation2 =
        (0.5 - simplex_info_.numTotRandomValue_[i]) *
        simplex_info_.dual_simplex_cost_perturbation_multiplier * 1e-12;
    simplex_info_.workCost_[i] += perturbation2;
#ifdef HiGHSDEV
    perturbation2 = fabs(perturbation2);
    updateValueDistribution(perturbation2,
                            analysis_.cost_perturbation2_distribution);
#endif
  }
}

void HEkk::initialiseNonbasicWorkValue() {
  // Assign nonbasic values from bounds and (if necessary) nonbasicMove
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  for (int iVar = 0; iVar < numTot; iVar++) {
    if (!simplex_basis_.nonbasicFlag_[iVar]) continue;
    // Nonbasic variable
    const double lower = simplex_info_.workLower_[iVar];
    const double upper = simplex_info_.workUpper_[iVar];
    double value;
    if (lower == upper) {
      value = lower;
    } else if (simplex_basis_.nonbasicMove_[iVar] == NONBASIC_MOVE_UP) {
      value = lower;
    } else if (simplex_basis_.nonbasicMove_[iVar] == NONBASIC_MOVE_DN) {
      value = upper;
    } else {
      assert(simplex_basis_.nonbasicMove_[iVar] == NONBASIC_MOVE_ZE);
      value = 0;
    }
    simplex_info_.workValue_[iVar] = value;
  }
}

void HEkk::choosePriceTechnique(const int price_strategy,
                                const double row_ep_density,
                                bool& use_col_price,
                                bool& use_row_price_w_switch) {
  // By default switch to column PRICE when pi_p has at least this
  // density
  const double density_for_column_price_switch = 0.75;
  use_col_price =
      (price_strategy == SIMPLEX_PRICE_STRATEGY_COL) ||
      (price_strategy == SIMPLEX_PRICE_STRATEGY_ROW_SWITCH_COL_SWITCH &&
       row_ep_density > density_for_column_price_switch);
  use_row_price_w_switch =
      price_strategy == SIMPLEX_PRICE_STRATEGY_ROW_SWITCH ||
      price_strategy == SIMPLEX_PRICE_STRATEGY_ROW_SWITCH_COL_SWITCH;
}

void HEkk::computeTableauRowFromPiP(const HVector& row_ep, HVector& row_ap) {
  const int solver_num_row = simplex_lp_.numRow_;
  const double local_density = 1.0 * row_ep.count / solver_num_row;
  bool use_col_price;
  bool use_row_price_w_switch;
  choosePriceTechnique(simplex_info_.price_strategy, local_density,
                       use_col_price, use_row_price_w_switch);
#ifdef HiGHSDEV
  if (simplex_info_.analyse_iterations) {
    if (use_col_price) {
      analysis_.operationRecordBefore(ANALYSIS_OPERATION_TYPE_PRICE_AP, row_ep,
                                      0.0);
      analysis_.num_col_price++;
    } else if (use_row_price_w_switch) {
      analysis_.operationRecordBefore(ANALYSIS_OPERATION_TYPE_PRICE_AP, row_ep,
                                      analysis_.row_ep_density);
      analysis_.num_row_price_with_switch++;
    } else {
      analysis_.operationRecordBefore(ANALYSIS_OPERATION_TYPE_PRICE_AP, row_ep,
                                      analysis_.row_ep_density);
      analysis_.num_row_price++;
    }
  }
#endif
  analysis_.simplexTimerStart(PriceClock);
  row_ap.clear();
  if (use_col_price) {
    // Perform column-wise PRICE
    matrix_.priceByColumn(row_ap, row_ep);
  } else if (use_row_price_w_switch) {
    // Perform hyper-sparse row-wise PRICE, but switch if the density of row_ap
    // becomes extreme
    const double switch_density = matrix_.hyperPRICE;
    matrix_.priceByRowSparseResultWithSwitch(
        row_ap, row_ep, analysis_.row_ap_density, 0, switch_density);
  } else {
    // Perform hyper-sparse row-wise PRICE
    matrix_.priceByRowSparseResult(row_ap, row_ep);
  }

  const int solver_num_col = simplex_lp_.numCol_;
  if (use_col_price) {
    // Column-wise PRICE computes components of row_ap corresponding
    // to basic variables, so zero these by exploiting the fact that,
    // for basic variables, nonbasicFlag[*]=0
    const int* nonbasicFlag = &simplex_basis_.nonbasicFlag_[0];
    for (int col = 0; col < solver_num_col; col++)
      row_ap.array[col] = nonbasicFlag[col] * row_ap.array[col];
  }
#ifdef HiGHSDEV
  // Possibly analyse the error in the result of PRICE
  const bool analyse_price_error = false;
  if (analyse_price_error) matrix_.debugPriceResult(row_ap, row_ep);
#endif
  // Update the record of average row_ap density
  const double local_row_ap_density = (double)row_ap.count / solver_num_col;
  analysis_.updateOperationResultDensity(local_row_ap_density,
                                         analysis_.row_ap_density);
#ifdef HiGHSDEV
  if (simplex_info_.analyse_iterations)
    analysis_.operationRecordAfter(ANALYSIS_OPERATION_TYPE_PRICE_AP, row_ap);
#endif
  analysis_.simplexTimerStop(PriceClock);
}

void HEkk::computePrimal() {
  analysis_.simplexTimerStart(ComputePrimalClock);
  // Setup a local buffer for the values of basic variables
  HVector primal_col;
  primal_col.setup(simplex_lp_.numRow_);
  primal_col.clear();
  for (int i = 0; i < simplex_lp_.numCol_ + simplex_lp_.numRow_; i++) {
    if (simplex_basis_.nonbasicFlag_[i] && simplex_info_.workValue_[i] != 0) {
      matrix_.collect_aj(primal_col, i, simplex_info_.workValue_[i]);
    }
  }
  // If debugging, take a copy of the RHS
  vector<double> debug_primal_rhs;
  if (options_.highs_debug_level >= HIGHS_DEBUG_LEVEL_COSTLY)
    debug_primal_rhs = primal_col.array;

  // It's possible that the buffer has no nonzeros, so performing
  // FTRAN is unnecessary. Not much of a saving, but the zero density
  // looks odd in the analysis!
  if (primal_col.count) {
    factor_.ftran(primal_col, analysis_.primal_col_density,
                  analysis_.pointer_serial_factor_clocks);
    const double local_primal_col_density =
        (double)primal_col.count / simplex_lp_.numRow_;
    analysis_.updateOperationResultDensity(local_primal_col_density,
                                           analysis_.primal_col_density);
  }
  for (int i = 0; i < simplex_lp_.numRow_; i++) {
    int iCol = simplex_basis_.basicIndex_[i];
    simplex_info_.baseValue_[i] = -primal_col.array[i];
    simplex_info_.baseLower_[i] = simplex_info_.workLower_[iCol];
    simplex_info_.baseUpper_[i] = simplex_info_.workUpper_[iCol];
  }
  //  debugComputePrimal(ekk_instance_, debug_primal_rhs);
  // Now have basic primals
  simplex_lp_status_.has_basic_primal_values = true;
  analysis_.simplexTimerStop(ComputePrimalClock);
}

void HEkk::computeDual() {
  analysis_.simplexTimerStart(ComputeDualClock);
  // Create a local buffer for the pi vector
  HVector dual_col;
  dual_col.setup(simplex_lp_.numRow_);
  dual_col.clear();
  for (int iRow = 0; iRow < simplex_lp_.numRow_; iRow++) {
    const double value =
        simplex_info_.workCost_[simplex_basis_.basicIndex_[iRow]] +
        simplex_info_.workShift_[simplex_basis_.basicIndex_[iRow]];
    if (value) {
      dual_col.count++;
      dual_col.index[iRow] = iRow;
      dual_col.array[iRow] = value;
    }
  }
  // If debugging, take a copy of the basic costs and any previous duals
  vector<double> debug_previous_workDual;
  vector<double> debug_basic_costs;
  if (options_.highs_debug_level >= HIGHS_DEBUG_LEVEL_COSTLY) {
    debug_basic_costs = dual_col.array;
    if (simplex_lp_status_.has_nonbasic_dual_values)
      debug_previous_workDual = simplex_info_.workDual_;
  }
  // Copy the costs in case the basic costs are all zero
  const int numTot = simplex_lp_.numCol_ + simplex_lp_.numRow_;
  for (int i = 0; i < numTot; i++)
    simplex_info_.workDual_[i] = simplex_info_.workCost_[i];
  if (dual_col.count) {
    // RHS of row dual calculation is nonzero
#ifdef HiGHSDEV
    if (simplex_info_.analyse_iterations)
      analysis_.operationRecordBefore(ANALYSIS_OPERATION_TYPE_BTRAN_FULL,
                                      dual_col, analysis_.dual_col_density);
#endif
    factor_.btran(dual_col, analysis_.dual_col_density,
                  analysis_.pointer_serial_factor_clocks);
#ifdef HiGHSDEV
    if (simplex_info_.analyse_iterations)
      analysis_.operationRecordAfter(ANALYSIS_OPERATION_TYPE_BTRAN_FULL,
                                     dual_col);
#endif
    const double local_dual_col_density =
        (double)dual_col.count / simplex_lp_.numRow_;
    analysis_.updateOperationResultDensity(local_dual_col_density,
                                           analysis_.dual_col_density);
    // Create a local buffer for the values of reduced costs
    HVector dual_row;
    dual_row.setup(simplex_lp_.numCol_);
    dual_row.clear();
#ifdef HiGHSDEV
    double price_full_historical_density = 1;
    if (simplex_info_.analyse_iterations)
      analysis_.operationRecordBefore(ANALYSIS_OPERATION_TYPE_PRICE_FULL,
                                      dual_row, price_full_historical_density);
#endif
    matrix_.priceByColumn(dual_row, dual_col);
#ifdef HiGHSDEV
    if (simplex_info_.analyse_iterations)
      analysis_.operationRecordAfter(ANALYSIS_OPERATION_TYPE_PRICE_FULL,
                                     dual_row);
#endif
    for (int i = 0; i < simplex_lp_.numCol_; i++)
      simplex_info_.workDual_[i] -= dual_row.array[i];
    for (int i = simplex_lp_.numCol_; i < numTot; i++)
      simplex_info_.workDual_[i] -= dual_col.array[i - simplex_lp_.numCol_];
    // Possibly analyse the computed dual values
    //    debugComputeDual(ekk_instance_, debug_previous_workDual,
    //                     debug_basic_costs, dual_col.array);
  }
  // Now have nonbasic duals
  simplex_lp_status_.has_nonbasic_dual_values = true;
  analysis_.simplexTimerStop(ComputeDualClock);
}

// The major model updates. Factor calls factor_.update; Matrix
// calls matrix_.update; updatePivots does everything---and is
// called from the likes of HDual::updatePivots
void HEkk::updateFactor(HVector* column, HVector* row_ep, int* iRow,
                        int* hint) {
  analysis_.simplexTimerStart(UpdateFactorClock);
  factor_.update(column, row_ep, iRow, hint);
  // Now have a representation of B^{-1}, but it is not fresh
  simplex_lp_status_.has_invert = true;
  if (simplex_info_.update_count >= simplex_info_.update_limit)
    *hint = INVERT_HINT_UPDATE_LIMIT_REACHED;
  analysis_.simplexTimerStop(UpdateFactorClock);
}

void HEkk::updatePivots(const int columnIn, const int rowOut,
                        const int sourceOut) {
  analysis_.simplexTimerStart(UpdatePivotsClock);
  int columnOut = simplex_basis_.basicIndex_[rowOut];

  // Incoming variable
  simplex_basis_.basicIndex_[rowOut] = columnIn;
  simplex_basis_.nonbasicFlag_[columnIn] = 0;
  simplex_basis_.nonbasicMove_[columnIn] = 0;
  simplex_info_.baseLower_[rowOut] = simplex_info_.workLower_[columnIn];
  simplex_info_.baseUpper_[rowOut] = simplex_info_.workUpper_[columnIn];

  // Outgoing variable
  simplex_basis_.nonbasicFlag_[columnOut] = 1;
  if (simplex_info_.workLower_[columnOut] ==
      simplex_info_.workUpper_[columnOut]) {
    simplex_info_.workValue_[columnOut] = simplex_info_.workLower_[columnOut];
    simplex_basis_.nonbasicMove_[columnOut] = 0;
  } else if (sourceOut == -1) {
    simplex_info_.workValue_[columnOut] = simplex_info_.workLower_[columnOut];
    simplex_basis_.nonbasicMove_[columnOut] = 1;
  } else {
    simplex_info_.workValue_[columnOut] = simplex_info_.workUpper_[columnOut];
    simplex_basis_.nonbasicMove_[columnOut] = -1;
  }
  // Update the dual objective value
  double nwValue = simplex_info_.workValue_[columnOut];
  double vrDual = simplex_info_.workDual_[columnOut];
  double dl_dual_objective_value = nwValue * vrDual;
  simplex_info_.updated_dual_objective_value += dl_dual_objective_value;
  simplex_info_.update_count++;
  // Update the number of basic logicals
  if (columnOut < simplex_lp_.numCol_) simplex_info_.num_basic_logicals -= 1;
  if (columnIn < simplex_lp_.numCol_) simplex_info_.num_basic_logicals += 1;
  // No longer have a representation of B^{-1}, and certainly not
  // fresh!
  simplex_lp_status_.has_invert = false;
  simplex_lp_status_.has_fresh_invert = false;
  // Data are no longer fresh from rebuild
  simplex_lp_status_.has_fresh_rebuild = false;
  analysis_.simplexTimerStop(UpdatePivotsClock);
}

void HEkk::updateMatrix(const int columnIn, const int columnOut) {
  analysis_.simplexTimerStart(UpdateMatrixClock);
  matrix_.update(columnIn, columnOut);
  analysis_.simplexTimerStop(UpdateMatrixClock);
}

void HEkk::computeSimplexInfeasible() {
  computeSimplexPrimalInfeasible();
  computeSimplexDualInfeasible();
}

void HEkk::computeSimplexPrimalInfeasible() {
  // Computes num/max/sum of primal infeasibliities according to the
  // simplex bounds. This is used to determine optimality in dual
  // phase 1 and dual phase 2, albeit using different bounds in
  // workLower/Upper.
  analysis_.simplexTimerStart(ComputePrIfsClock);
  const double scaled_primal_feasibility_tolerance =
      options_.primal_feasibility_tolerance;
  int& num_primal_infeasibilities = simplex_info_.num_primal_infeasibilities;
  double& max_primal_infeasibility = simplex_info_.max_primal_infeasibility;
  double& sum_primal_infeasibilities = simplex_info_.sum_primal_infeasibilities;
  num_primal_infeasibilities = 0;
  max_primal_infeasibility = 0;
  sum_primal_infeasibilities = 0;

  for (int i = 0; i < simplex_lp_.numCol_ + simplex_lp_.numRow_; i++) {
    if (simplex_basis_.nonbasicFlag_[i]) {
      // Nonbasic column
      double value = simplex_info_.workValue_[i];
      double lower = simplex_info_.workLower_[i];
      double upper = simplex_info_.workUpper_[i];
      double primal_infeasibility = max(lower - value, value - upper);
      if (primal_infeasibility > 0) {
        if (primal_infeasibility > scaled_primal_feasibility_tolerance)
          num_primal_infeasibilities++;
        max_primal_infeasibility =
            std::max(primal_infeasibility, max_primal_infeasibility);
        sum_primal_infeasibilities += primal_infeasibility;
      }
    }
  }
  for (int i = 0; i < simplex_lp_.numRow_; i++) {
    // Basic variable
    double value = simplex_info_.baseValue_[i];
    double lower = simplex_info_.baseLower_[i];
    double upper = simplex_info_.baseUpper_[i];
    double primal_infeasibility = max(lower - value, value - upper);
    if (primal_infeasibility > 0) {
      if (primal_infeasibility > scaled_primal_feasibility_tolerance)
        num_primal_infeasibilities++;
      max_primal_infeasibility =
          std::max(primal_infeasibility, max_primal_infeasibility);
      sum_primal_infeasibilities += primal_infeasibility;
    }
  }
  analysis_.simplexTimerStop(ComputePrIfsClock);
}

void HEkk::computeSimplexDualInfeasible() {
  analysis_.simplexTimerStart(ComputeDuIfsClock);
  // Computes num/max/sum of dual infeasibilities in phase 1 and phase
  // 2 according to nonbasicMove. The bounds are only used to identify
  // free variables. Fixed variables are assumed to have
  // nonbasicMove=0 so that no dual infeasibility is counted for them.
  const double scaled_dual_feasibility_tolerance =
      options_.dual_feasibility_tolerance;
  // Possibly verify that nonbasicMove is correct for fixed variables
  //
  //  debugFixedNonbasicMove();

  int& num_dual_infeasibilities = simplex_info_.num_dual_infeasibilities;
  double& max_dual_infeasibility = simplex_info_.max_dual_infeasibility;
  double& sum_dual_infeasibilities = simplex_info_.sum_dual_infeasibilities;
  num_dual_infeasibilities = 0;
  max_dual_infeasibility = 0;
  sum_dual_infeasibilities = 0;

  for (int iVar = 0; iVar < simplex_lp_.numCol_ + simplex_lp_.numRow_; iVar++) {
    if (!simplex_basis_.nonbasicFlag_[iVar]) continue;
    // Nonbasic column
    const double dual = simplex_info_.workDual_[iVar];
    const double lower = simplex_info_.workLower_[iVar];
    const double upper = simplex_info_.workUpper_[iVar];
    double dual_infeasibility = 0;
    if (highs_isInfinity(-lower) && highs_isInfinity(upper)) {
      // Free: any nonzero dual value is infeasible
      dual_infeasibility = fabs(dual);
    } else {
      // Not free: any dual infeasibility is given by the dual value
      // signed by nonbasicMove
      dual_infeasibility = -simplex_basis_.nonbasicMove_[iVar] * dual;
    }
    if (dual_infeasibility > 0) {
      if (dual_infeasibility >= scaled_dual_feasibility_tolerance)
        num_dual_infeasibilities++;
      max_dual_infeasibility =
          std::max(dual_infeasibility, max_dual_infeasibility);
      sum_dual_infeasibilities += dual_infeasibility;
    }
  }
  analysis_.simplexTimerStop(ComputeDuIfsClock);
}

void HEkk::computeSimplexLpDualInfeasible() {
  // Compute num/max/sum of dual infeasibliities according to the
  // bounds of the simplex LP. Assumes that boxed variables have
  // primal variable at the bound corresponding to the sign of the
  // dual so should only be used in dual phase 1 - where it's only
  // used for reporting after rebuilds.
  // Possibly verify that nonbasicMove is correct for fixed variables
  //
  //  debugFixedNonbasicMove();
  const double scaled_dual_feasibility_tolerance =
      options_.dual_feasibility_tolerance;
  int& num_dual_infeasibilities =
      analysis_.num_dual_phase_1_lp_dual_infeasibility;
  double& max_dual_infeasibility =
      analysis_.max_dual_phase_1_lp_dual_infeasibility;
  double& sum_dual_infeasibilities =
      analysis_.sum_dual_phase_1_lp_dual_infeasibility;
  num_dual_infeasibilities = 0;
  max_dual_infeasibility = 0;
  sum_dual_infeasibilities = 0;

  for (int iCol = 0; iCol < simplex_lp_.numCol_; iCol++) {
    int iVar = iCol;
    if (!simplex_basis_.nonbasicFlag_[iVar]) continue;
    // Nonbasic column
    const double dual = simplex_info_.workDual_[iVar];
    const double lower = simplex_lp_.colLower_[iCol];
    const double upper = simplex_lp_.colUpper_[iCol];
    double dual_infeasibility = 0;
    if (highs_isInfinity(upper)) {
      if (highs_isInfinity(-lower)) {
        // Free: any nonzero dual value is infeasible
        dual_infeasibility = fabs(dual);
      } else {
        // Only lower bounded: a negative dual is infeasible
        dual_infeasibility = -dual;
      }
    } else {
      if (highs_isInfinity(-lower)) {
        // Only upper bounded: a positive dual is infeasible
        dual_infeasibility = dual;
      } else {
        // Boxed or fixed: any dual value is feasible
        dual_infeasibility = 0;
      }
    }
    if (dual_infeasibility > 0) {
      if (dual_infeasibility >= scaled_dual_feasibility_tolerance)
        num_dual_infeasibilities++;
      max_dual_infeasibility =
          std::max(dual_infeasibility, max_dual_infeasibility);
      sum_dual_infeasibilities += dual_infeasibility;
    }
  }
  for (int iRow = 0; iRow < simplex_lp_.numRow_; iRow++) {
    int iVar = simplex_lp_.numCol_ + iRow;
    if (!simplex_basis_.nonbasicFlag_[iVar]) continue;
    // Nonbasic row
    const double dual = -simplex_info_.workDual_[iVar];
    const double lower = simplex_lp_.rowLower_[iRow];
    const double upper = simplex_lp_.rowUpper_[iRow];
    double dual_infeasibility = 0;
    if (highs_isInfinity(upper)) {
      if (highs_isInfinity(-lower)) {
        // Free: any nonzero dual value is infeasible
        dual_infeasibility = fabs(dual);
      } else {
        // Only lower bounded: a negative dual is infeasible
        dual_infeasibility = -dual;
      }
    } else {
      if (highs_isInfinity(-lower)) {
        // Only upper bounded: a positive dual is infeasible
        dual_infeasibility = dual;
      } else {
        // Boxed or fixed: any dual value is feasible
        dual_infeasibility = 0;
      }
    }
    if (dual_infeasibility > 0) {
      if (dual_infeasibility >= scaled_dual_feasibility_tolerance)
        num_dual_infeasibilities++;
      max_dual_infeasibility =
          std::max(dual_infeasibility, max_dual_infeasibility);
      sum_dual_infeasibilities += dual_infeasibility;
    }
  }
}

bool HEkk::bailoutReturn() {
  if (solve_bailout_) {
    // If bailout has already been decided: check that it's for one of
    // these reasons
    assert(scaled_model_status_ == HighsModelStatus::REACHED_TIME_LIMIT ||
           scaled_model_status_ == HighsModelStatus::REACHED_ITERATION_LIMIT ||
           scaled_model_status_ ==
               HighsModelStatus::REACHED_DUAL_OBJECTIVE_VALUE_UPPER_BOUND);
  }
  return solve_bailout_;
}

bool HEkk::bailoutOnTimeIterations() {
  if (solve_bailout_) {
    // Bailout has already been decided: check that it's for one of these
    // reasons
    assert(scaled_model_status_ == HighsModelStatus::REACHED_TIME_LIMIT ||
           scaled_model_status_ == HighsModelStatus::REACHED_ITERATION_LIMIT ||
           scaled_model_status_ ==
               HighsModelStatus::REACHED_DUAL_OBJECTIVE_VALUE_UPPER_BOUND);
  } else if (timer_.readRunHighsClock() > options_.time_limit) {
    solve_bailout_ = true;
    scaled_model_status_ = HighsModelStatus::REACHED_TIME_LIMIT;
  } else if (iteration_count_ >= options_.simplex_iteration_limit) {
    solve_bailout_ = true;
    scaled_model_status_ = HighsModelStatus::REACHED_ITERATION_LIMIT;
  }
  return solve_bailout_;
}

HighsStatus HEkk::returnFromSolve(const HighsStatus return_status) {
  simplex_info_.valid_backtracking_basis_ = false;
  HighsLogMessage(options_.logfile, HighsMessageType::INFO,
                  "HEkk: Returning from solver with HighsModelStatus = %s and "
                  "HighsStatus = %s",
                  utilHighsModelStatusToString(scaled_model_status_).c_str(),
                  HighsStatusToString(return_status).c_str());
  return return_status;
}

double HEkk::computeBasisCondition() {
  int solver_num_row = simplex_lp_.numRow_;
  int solver_num_col = simplex_lp_.numCol_;
  vector<double> bs_cond_x;
  vector<double> bs_cond_y;
  vector<double> bs_cond_z;
  vector<double> bs_cond_w;
  HVector row_ep;
  row_ep.setup(solver_num_row);

  const int* Astart = &simplex_lp_.Astart_[0];
  const double* Avalue = &simplex_lp_.Avalue_[0];
  // Compute the Hager condition number estimate for the basis matrix
  const double NoDensity = 1;
  bs_cond_x.resize(solver_num_row);
  bs_cond_y.resize(solver_num_row);
  bs_cond_z.resize(solver_num_row);
  bs_cond_w.resize(solver_num_row);
  // x = ones(n,1)/n;
  // y = A\x;
  double mu = 1.0 / solver_num_row;
  double norm_Binv;
  for (int r_n = 0; r_n < solver_num_row; r_n++) bs_cond_x[r_n] = mu;
  row_ep.clear();
  for (int r_n = 0; r_n < solver_num_row; r_n++) {
    double value = bs_cond_x[r_n];
    if (value) {
      row_ep.index[row_ep.count] = r_n;
      row_ep.array[r_n] = value;
      row_ep.count++;
    }
  }
  for (int ps_n = 1; ps_n <= 5; ps_n++) {
    row_ep.packFlag = false;
    factor_.ftran(row_ep, NoDensity);
    // zeta = sign(y);
    for (int r_n = 0; r_n < solver_num_row; r_n++) {
      bs_cond_y[r_n] = row_ep.array[r_n];
      if (bs_cond_y[r_n] > 0)
        bs_cond_w[r_n] = 1.0;
      else if (bs_cond_y[r_n] < 0)
        bs_cond_w[r_n] = -1.0;
      else
        bs_cond_w[r_n] = 0.0;
    }
    // z=A'\zeta;
    row_ep.clear();
    for (int r_n = 0; r_n < solver_num_row; r_n++) {
      double value = bs_cond_w[r_n];
      if (value) {
        row_ep.index[row_ep.count] = r_n;
        row_ep.array[r_n] = value;
        row_ep.count++;
      }
    }
    row_ep.packFlag = false;
    factor_.btran(row_ep, NoDensity);
    double norm_z = 0.0;
    double ztx = 0.0;
    norm_Binv = 0.0;
    int argmax_z = -1;
    for (int r_n = 0; r_n < solver_num_row; r_n++) {
      bs_cond_z[r_n] = row_ep.array[r_n];
      double abs_z_v = fabs(bs_cond_z[r_n]);
      if (abs_z_v > norm_z) {
        norm_z = abs_z_v;
        argmax_z = r_n;
      }
      ztx += bs_cond_z[r_n] * bs_cond_x[r_n];
      norm_Binv += fabs(bs_cond_y[r_n]);
    }
    if (norm_z <= ztx) break;
    // x = zeros(n,1);
    // x(fd_i) = 1;
    for (int r_n = 0; r_n < solver_num_row; r_n++) bs_cond_x[r_n] = 0.0;
    row_ep.clear();
    row_ep.count = 1;
    row_ep.index[0] = argmax_z;
    row_ep.array[argmax_z] = 1.0;
    bs_cond_x[argmax_z] = 1.0;
  }
  double norm_B = 0.0;
  for (int r_n = 0; r_n < solver_num_row; r_n++) {
    int vr_n = simplex_basis_.basicIndex_[r_n];
    double c_norm = 0.0;
    if (vr_n < solver_num_col)
      for (int el_n = Astart[vr_n]; el_n < Astart[vr_n + 1]; el_n++)
        c_norm += fabs(Avalue[el_n]);
    else
      c_norm += 1.0;
    norm_B = max(c_norm, norm_B);
  }
  double cond_B = norm_Binv * norm_B;
  return cond_B;
}

void HEkk::initialiseAnalysis() {
  analysis_.setup(simplex_lp_, options_, iteration_count_);
}