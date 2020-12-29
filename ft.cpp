//
//  ft.cpp
//  Update
//
//  Created by pique on 2020/12/29.
//  Copyright © 2020 pique. All rights reserved.
//

#include <stdio.h>

void HFactor::ftran(HVector& vector) const {

  ftranL(vector);
  ftranU(vector);
}

void HFactor::btran(HVector& vector) const {
  btranU(vector);
  btranL(vector);
}

void HFactor::ftranL(HVector& rhs) const {
 
    // Alias to RHS
    int RHScount = 0;
    int* RHSindex = &rhs.index[0];
    double* RHSarray = &rhs.array[0];

    // Alias to factor L
    const int* Lstart = &this->Lstart[0];
    const int* Lindex = this->Lindex.size() > 0 ? &this->Lindex[0] : NULL;
    const double* Lvalue = this->Lvalue.size() > 0 ? &this->Lvalue[0] : NULL;

    // Transform
    for (int i = 0; i < numRow; i++) {
      int pivotRow = LpivotIndex[i];
      const double pivotX = RHSarray[pivotRow];
      if (fabs(pivotX) > HIGHS_CONST_TINY) {
        RHSindex[RHScount++] = pivotRow;
        const int start = Lstart[i];
        const int end = Lstart[i + 1];
        for (int k = start; k < end; k++)
          RHSarray[Lindex[k]] -= pivotX * Lvalue[k];
      } else
        RHSarray[pivotRow] = 0;
    }
    // Save the count
    rhs.count = RHScount;
}

void HFactor::btranL(HVector& rhs) const {
  
  if (current_density > hyperCANCEL || historical_density > hyperBTRANL) {
    // Alias to RHS
    int RHScount = 0;
    int* RHSindex = &rhs.index[0];
    double* RHSarray = &rhs.array[0];

    // Alias to factor L
    const int* LRstart = &this->LRstart[0];
    const int* LRindex = this->LRindex.size() > 0 ? &this->LRindex[0] : NULL;
    const double* LRvalue = this->LRvalue.size() > 0 ? &this->LRvalue[0] : NULL;

    // Transform
    for (int i = numRow - 1; i >= 0; i--) {
      int pivotRow = LpivotIndex[i];
      const double pivotX = RHSarray[pivotRow];
      if (fabs(pivotX) > HIGHS_CONST_TINY) {
        RHSindex[RHScount++] = pivotRow;
        RHSarray[pivotRow] = pivotX;
        const int start = LRstart[i];
        const int end = LRstart[i + 1];
        for (int k = start; k < end; k++)
          RHSarray[LRindex[k]] -= pivotX * LRvalue[k];
      } else
        RHSarray[pivotRow] = 0;
    }
    // Save the count
    rhs.count = RHScount;
  }
}

void HFactor::ftranU(HVector& rhs) const {
  // The update part
  if (updateMethod == UPDATE_METHOD_FT) {

    //    const double current_density = 1.0 * rhs.count / numRow;
    ftranFT(rhs);
    rhs.tight();
    rhs.pack();
  }

  // The regular part
  const double current_density = 1.0 * rhs.count / numRow;
  if (current_density > hyperCANCEL || historical_density > hyperFTRANU) {
    const bool report_ftran_upper_sparse =
        false;  // current_density < hyperCANCEL;

    // Alias to non constant
    double RHS_syntheticTick = 0;
    int RHScount = 0;
    int* RHSindex = &rhs.index[0];
    double* RHSarray = &rhs.array[0];

    // Alias to the factor
    const int* Ustart = &this->Ustart[0];
    const int* Uend = &this->Ulastp[0];
    const int* Uindex = this->Uindex.size() > 0 ? &this->Uindex[0] : NULL;
    const double* Uvalue = this->Uvalue.size() > 0 ? &this->Uvalue[0] : NULL;

    // Transform
    int UpivotCount = UpivotIndex.size();
    for (int iLogic = UpivotCount - 1; iLogic >= 0; iLogic--) {
      // Skip void
      if (UpivotIndex[iLogic] == -1) continue;

      // Normal part
      const int pivotRow = UpivotIndex[iLogic];
      double pivotX = RHSarray[pivotRow];
      if (fabs(pivotX) > HIGHS_CONST_TINY) {
        pivotX /= UpivotValue[iLogic];
        RHSindex[RHScount++] = pivotRow;
        RHSarray[pivotRow] = pivotX;
        const int start = Ustart[iLogic];
        const int end = Uend[iLogic];
        if (iLogic >= numRow) {
          RHS_syntheticTick += (end - start);
        }
        for (int k = start; k < end; k++)
          RHSarray[Uindex[k]] -= pivotX * Uvalue[k];
      } else
        RHSarray[pivotRow] = 0;
    }

    // Save the count
    rhs.count = RHScount;
    rhs.syntheticTick += RHS_syntheticTick * 15 + (UpivotCount - numRow) * 10;
  }

}

void HFactor::btranU(HVector& rhs) const {

  // The regular part
  
  if (current_density > hyperCANCEL || historical_density > hyperBTRANU) {
    // Alias to non constant
    double RHS_syntheticTick = 0;
    int RHScount = 0;
    int* RHSindex = &rhs.index[0];
    double* RHSarray = &rhs.array[0];

    // Alias to the factor
    const int* URstart = &this->URstart[0];
    const int* URend = &this->URlastp[0];
    const int* URindex = &this->URindex[0];
    const double* URvalue = &this->URvalue[0];

    // Transform
    int UpivotCount = UpivotIndex.size();
    for (int iLogic = 0; iLogic < UpivotCount; iLogic++) {
      // Skip void
      if (UpivotIndex[iLogic] == -1) continue;

      // Normal part
      const int pivotRow = UpivotIndex[iLogic];
      double pivotX = RHSarray[pivotRow];
      if (fabs(pivotX) > HIGHS_CONST_TINY) {
        pivotX /= UpivotValue[iLogic];
        RHSindex[RHScount++] = pivotRow;
        RHSarray[pivotRow] = pivotX;
        const int start = URstart[iLogic];
        const int end = URend[iLogic];
        if (iLogic >= numRow) {
          RHS_syntheticTick += (end - start);
        }
        for (int k = start; k < end; k++)
          RHSarray[URindex[k]] -= pivotX * URvalue[k];
      } else
        RHSarray[pivotRow] = 0;
    }

    // Save the count
    rhs.count = RHScount;
    rhs.syntheticTick += RHS_syntheticTick * 15 + (UpivotCount - numRow) * 10;
  }

  // The update part
  if (updateMethod == UPDATE_METHOD_FT) {
    factor_timer.start(FactorBtranUpperFT, factor_timer_clock_pointer);
    rhs.tight();
    rhs.pack();
    //    const double current_density = 1.0 * rhs.count / numRow;
    btranFT(rhs);
    rhs.tight();
    factor_timer.stop(FactorBtranUpperFT, factor_timer_clock_pointer);
  }

}

void HFactor::ftranFT(HVector& vector) const {
  // Alias to PF buffer
  const int PFpivotCount = PFpivotIndex.size();
  int* PFpivotIndex = NULL;
  if (this->PFpivotIndex.size() > 0)
    PFpivotIndex = (int*)&this->PFpivotIndex[0];

  const int* PFstart = this->PFstart.size() > 0 ? &this->PFstart[0] : NULL;
  const int* PFindex = this->PFindex.size() > 0 ? &this->PFindex[0] : NULL;
  const double* PFvalue = this->PFvalue.size() > 0 ? &this->PFvalue[0] : NULL;

  // Alias to non constant
  int RHScount = vector.count;
  int* RHSindex = &vector.index[0];
  double* RHSarray = &vector.array[0];

  // Forwardly apply row ETA
  for (int i = 0; i < PFpivotCount; i++) {
    int iRow = PFpivotIndex[i];
    double value0 = RHSarray[iRow];
    double value1 = value0;
    const int start = PFstart[i];
    const int end = PFstart[i + 1];
    for (int k = start; k < end; k++)
      value1 -= RHSarray[PFindex[k]] * PFvalue[k];
    // This would skip the situation where they are both zeros
    if (value0 || value1) {
      if (value0 == 0) RHSindex[RHScount++] = iRow;
      RHSarray[iRow] =
          (fabs(value1) < HIGHS_CONST_TINY) ? HIGHS_CONST_ZERO : value1;
    }
  }

  // Save count back
  vector.count = RHScount;
  vector.syntheticTick += PFpivotCount * 20 + PFstart[PFpivotCount] * 5;
  if (PFstart[PFpivotCount] / (PFpivotCount + 1) < 5) {
    vector.syntheticTick += PFstart[PFpivotCount] * 5;
  }
}

void HFactor::btranFT(HVector& vector) const {
  // Alias to PF buffer
  const int PFpivotCount = PFpivotIndex.size();
  const int* PFpivotIndex =
      this->PFpivotIndex.size() > 0 ? &this->PFpivotIndex[0] : NULL;
  const int* PFstart = this->PFstart.size() > 0 ? &this->PFstart[0] : NULL;
  const int* PFindex = this->PFindex.size() > 0 ? &this->PFindex[0] : NULL;
  const double* PFvalue = this->PFvalue.size() > 0 ? &this->PFvalue[0] : NULL;

  // Alias to non constant
  double RHS_syntheticTick = 0;
  int RHScount = vector.count;
  int* RHSindex = &vector.index[0];
  double* RHSarray = &vector.array[0];

  // Backwardly apply row ETA
  for (int i = PFpivotCount - 1; i >= 0; i--) {
    int pivotRow = PFpivotIndex[i];
    double pivotX = RHSarray[pivotRow];
    if (pivotX) {
      const int start = PFstart[i];
      const int end = PFstart[i + 1];
      RHS_syntheticTick += (end - start);
      for (int k = start; k < end; k++) {
        int iRow = PFindex[k];
        double value0 = RHSarray[iRow];
        double value1 = value0 - pivotX * PFvalue[k];
        if (value0 == 0) RHSindex[RHScount++] = iRow;
        RHSarray[iRow] =
            (fabs(value1) < HIGHS_CONST_TINY) ? HIGHS_CONST_ZERO : value1;
      }
    }
  }

  vector.syntheticTick += RHS_syntheticTick * 15 + PFpivotCount * 10;

  // Save count back
  vector.count = RHScount;
}

void HFactor::updateFT(HVector* aq, HVector* ep, int iRow
                       //, int* hint
) {
  // Store pivot
  int pLogic = UpivotLookup[iRow];
  double pivot = UpivotValue[pLogic];
  double alpha = aq->array[iRow];
  UpivotIndex[pLogic] = -1;

  // Delete pivotal row from U
  for (int k = URstart[pLogic]; k < URlastp[pLogic]; k++) {
    // Find the pivotal position
    int iLogic = UpivotLookup[URindex[k]];
    int iFind = Ustart[iLogic];
    int iLast = --Ulastp[iLogic];
    for (; iFind <= iLast; iFind++)
      if (Uindex[iFind] == iRow) break;
    // Put last to find, and delete last
    Uindex[iFind] = Uindex[iLast];
    Uvalue[iFind] = Uvalue[iLast];
  }

  // Delete pivotal column from UR
  for (int k = Ustart[pLogic]; k < Ulastp[pLogic]; k++) {
    // Find the pivotal position
    int iLogic = UpivotLookup[Uindex[k]];
    int iFind = URstart[iLogic];
    int iLast = --URlastp[iLogic];
    for (; iFind <= iLast; iFind++)
      if (URindex[iFind] == iRow) break;
    // Put last to find, and delete last
    URspace[iLogic]++;
    URindex[iFind] = URindex[iLast];
    URvalue[iFind] = URvalue[iLast];
  }

  // Store column to U
  Ustart.push_back(Uindex.size());
  for (int i = 0; i < aq->packCount; i++)
    if (aq->packIndex[i] != iRow) {
      Uindex.push_back(aq->packIndex[i]);
      Uvalue.push_back(aq->packValue[i]);
    }
  Ulastp.push_back(Uindex.size());
  int UstartX = Ustart.back();
  int UendX = Ulastp.back();
  UtotalX += UendX - UstartX + 1;

  // Store column as UR elements
  for (int k = UstartX; k < UendX; k++) {
    // Which ETA file
    int iLogic = UpivotLookup[Uindex[k]];

    // Move row to the end if necessary
    if (URspace[iLogic] == 0) {
      // Make pointers
      int row_start = URstart[iLogic];
      int row_count = URlastp[iLogic] - row_start;
      int new_start = URindex.size();
      int new_space = row_count * 1.1 + 5;

      // Check matrix UR
      URindex.resize(new_start + new_space);
      URvalue.resize(new_start + new_space);

      // Move elements
      int iFrom = row_start;
      int iEnd = row_start + row_count;
      int iTo = new_start;
      copy(&URindex[iFrom], &URindex[iEnd], &URindex[iTo]);
      copy(&URvalue[iFrom], &URvalue[iEnd], &URvalue[iTo]);

      // Save new pointers
      URstart[iLogic] = new_start;
      URlastp[iLogic] = new_start + row_count;
      URspace[iLogic] = new_space - row_count;
    }

    // Put into the next available space
    URspace[iLogic]--;
    int iPut = URlastp[iLogic]++;
    URindex[iPut] = iRow;
    URvalue[iPut] = Uvalue[k];
  }

  // Store UR pointers
  URstart.push_back(URstart[pLogic]);
  URlastp.push_back(URstart[pLogic]);
  URspace.push_back(URspace[pLogic] + URlastp[pLogic] - URstart[pLogic]);

  // Update pivot count
  UpivotLookup[iRow] = UpivotIndex.size();
  UpivotIndex.push_back(iRow);
  UpivotValue.push_back(pivot * alpha);

  // Store row_ep as R matrix
  for (int i = 0; i < ep->packCount; i++) {
    if (ep->packIndex[i] != iRow) {
      PFindex.push_back(ep->packIndex[i]);
      PFvalue.push_back(-ep->packValue[i] * pivot);
    }
  }
  UtotalX += PFindex.size() - PFstart.back();

  // Store R matrix pivot
  PFpivotIndex.push_back(iRow);
  PFstart.push_back(PFindex.size());

  // Update total countX
  UtotalX -= Ulastp[pLogic] - Ustart[pLogic];
  UtotalX -= URlastp[pLogic] - URstart[pLogic];

  //    // See if we want refactor
  //    if (UtotalX > UmeritX && PFpivotIndex.size() > 100)
  //        *hint = 1;
}
