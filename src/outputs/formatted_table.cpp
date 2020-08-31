//========================================================================================
// AthenaXXX astrophysical plasma code
// Copyright(C) 2020 James M. Stone <jmstone@ias.edu> and the Athena code team
// Licensed under the 3-clause BSD License (the "LICENSE")
//========================================================================================
//! \file formatted_table.cpp
//  \brief writes output data as a formatted (ASCI) table.  Since outputing data in this
//  format is very slow and creates large files, it cannot be used for anything other than
//  1D slices.  Code will issue error if this format is selected for 2D or 3D outputs.
//  TODO: Uses MPI-IO to write all MeshBlocks to a single file

#include <cstdio>      // fwrite(), fclose(), fopen(), fnprintf(), snprintf()
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "athena.hpp"
#include "mesh/mesh.hpp"
#include "outputs.hpp"

//----------------------------------------------------------------------------------------
// ctor: calls OutputType base class constructor

FormattedTableOutput::FormattedTableOutput(OutputParameters op, Mesh *pm)
  : OutputType(op, pm)
{
  // check that 1D slice specified, otherwise issue warning and quit
  if (pm->nx2gt1) {
    if (!(output_params.slice1) && !(output_params.slice2)) {
      std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__
                << std::endl << "Formatted table outputs can only contain 1D slices"
                << std::endl << "Please add additional slice planes" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  if (pm->nx3gt1) {
    if ((!(output_params.slice2) && !(output_params.slice3)) ||
        (!(output_params.slice1) && !(output_params.slice3))) {
      std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__
                << std::endl << "Formatted table outputs can only contain 1D slices"
                << std::endl << "Please add additional slice planes" << std::endl;
      exit(EXIT_FAILURE);
    }
  }
  
}

//----------------------------------------------------------------------------------------
//! \fn void FormattedTableOutput:::WriteOutputFile(Mesh *pm)
//  \brief writes output_data_ to file in tabular format using C style std::fprintf

void FormattedTableOutput::WriteOutputFile(Mesh *pm, ParameterInput *pin)
{
  // create filename: "file_basename" + "." + "file_id" + "." + XXXXX + ".tab"
  // where XXXXX = 5-digit file_number
  std::string fname;
  char number[6];
  std::snprintf(number, sizeof(number), "%05d", output_params.file_number);

  fname.assign(output_params.file_basename);
  fname.append(".");
  fname.append(output_params.file_id);
  fname.append(".");
  fname.append(number);
  fname.append(".tab");

  // open file for output
  FILE *pfile;
  if ((pfile = std::fopen(fname.c_str(),"w")) == nullptr) {
    std::cout << "### FATAL ERROR in " << __FILE__ << " at line " << __LINE__ << std::endl
        << std::endl << "Output file '" << fname << "' could not be opened" << std::endl;
    exit(EXIT_FAILURE);
  }

  // print file header
  std::fprintf(pfile, "# Athena++ data at time=%e", pm->time);
  std::fprintf(pfile, "  cycle=%d \n", pm->ncycle);

  // write one of x1, x2, x3 column headers
  std::fprintf(pfile, "#");
  if (nout1 > 1) std::fprintf(pfile, " i       x1v     ");
  if (nout2 > 1) std::fprintf(pfile, " j       x2v     ");
  if (nout3 > 1) std::fprintf(pfile, " k       x3v     ");
  // write data col headers from "name" stored in output_data_ AthenaArray
  for (auto it : output_data_) {
    std::fprintf(pfile, "    %s      ", it.GetLabel().c_str());
  }
  std::fprintf(pfile, "\n"); // terminate line

    // loop over all cells in data arrays
  for (int n=0; n<pm->nmbthisrank; ++n) {
    for (int k=0; k<nout3; ++k) {
      for (int j=0; j<nout2; ++j) {
        for (int i=0; i<nout1; ++i) {
          // write x1, x2, x3 indices and coordinates on start of new line
          if (nout1 > 1) {
            std::fprintf(pfile, "%04d", i+ois);
            std::fprintf(pfile, output_params.data_format.c_str(), output_x1posn_(n,i));
          }
          if (nout2 > 1) {
            std::fprintf(pfile, " %04d", j+ojs);  // note extra space for formatting
            std::fprintf(pfile, output_params.data_format.c_str(), output_x2posn_(n,j));
          }
          if (nout3 > 1) {
            std::fprintf(pfile, " %04d", k+oks);  // note extra space for formatting
            std::fprintf(pfile, output_params.data_format.c_str(), output_x3posn_(n,k));
          }

          // step through std::vector of output_data_ and write each on same line
          for (auto it : output_data_) {
            std::fprintf(pfile, output_params.data_format.c_str(), it(n,k,j,i));
          }
          std::fprintf(pfile,"\n"); // terminate line
        }
      }
    }
  }  // end loop over MeshBlocks

  std::fclose(pfile);   // don't forget to close the output file

  // increment counters
  output_params.file_number++;
  output_params.last_time += output_params.dt;
  // store filenumber and time into ParameterInput for restarts
  pin->SetInteger(output_params.block_name, "file_number", output_params.file_number);
  pin->SetReal(output_params.block_name, "last_time", output_params.last_time);

  return;
}
