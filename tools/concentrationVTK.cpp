/*
  This code transforms the particles position to concentration field
  defined in a square lattice. The concentration fields are saved as VTK
  files that can be postprocessed with the software VisIt,
  see https://wci.llnl.gov/simulation/computer-codes/visit/
  and https://en.wikipedia.org/wiki/VisIt

  The concentration in a cell of volume (dx*dy)  is defined as 
  c = number_of_particles_inside_cell / (dx*dy);

  Therefore the average concentration in the system is
  c_avg = total_number_of_particles / (lx*ly)

  where (lx*ly) is the (2D-) volume of the system.


  HOW TO COMPILE:
  This programs uses some files in fluam/src/, to compile it
  copy the file to fluam/bin/ and do
  g++ -o concentrationVTK.exe concentrationVTK.cpp


  HOW TO USE:
  concentrationVTK.exe run.particles outputname lx ly mx my nsteps sample [Npout]

  with
  run.particles = file with the particles positions generated by fluam
  outputname = prefix for the output files. The name of the files with 
               the concentration field will be 
	       outputname.index.concentration.vtk
  lx = system's length along the x-axis
  ly = system's length along the y-axis
  mx = number of cells along the x-axis          
  my = number of cells along the y-axis
  nsteps = How many configurations to read from the file
  sample = save concentration field only 1 of every "sample" steps.
  Npout = (default all particles) how many particles to include in the concentration
 */

#include <stdlib.h> 
#include <sstream>
#include <iostream>
#include <fstream>
#include "../src/visit_writer.h"
#include "../src/visit_writer.c"
using namespace std;

int main(int argc, char* argv[]){
  ifstream fileinput(argv[1]);
  string outputname = argv[2];
  double lx = atof(argv[3]);
  double ly = atof(argv[4]);
  int mx = atoi(argv[5]);
  int my = atoi(argv[6]);
  int nsteps = atoi(argv[7]);
  int sample = atoi(argv[8]);
  int Npout = 0;
  if(argc>9) Npout = atoi(argv[9]);


  // Read first line
  string word;
  getline(fileinput,word);
  cout << word << endl;
  word = word.substr(18,10);
  int np;
  np = atoi(word.c_str());
  int npout = np;
  if((Npout>0) && (Npout<=np)) npout=Npout;
  std::cout << "Outputing first " << npout << " particles only" << std::endl;

  // Create variables
  int step = 0;
  int skip = -1;
  double dx = lx / mx;
  double dy = ly / my;
  double t;
  double inverse_volume_cell = 1.0 / (dx * dy);
  double x[np], y[np], z[np];  
  double *c; 
  c = new double [mx*my]; 
 
  // Options some variables for VTK
  double *grid_x, *grid_y, *grid_z;
  int dims[] = {mx+1, my+1, 1}; //Use this for projection in 2D
  int nvars;
  int *vardims;
  int *centering;
  char **varnames;
  double **vars;
  char nameConcentration[] = {"concentration"};
  nvars = 1; // concentration
  vardims = new int[1];
  vardims[0] = 1;
  centering = new int [1];
  centering[0] = 0;
  varnames = new char* [1];
  varnames[0] = &nameConcentration[0];
  vars = new double* [1];
  vars[0] = c;
  grid_x = new double[mx+1];
  grid_y = new double[my+1];
  grid_z = new double[2];
  // Create 1-D grids for VisIt
  for(int i=0;i<=mx;i++){
    grid_x[i] = i*dx - 0.5 * lx;
  }
  cout << endl << endl << endl;
  for(int i=0;i<=my;i++){
    grid_y[i] = i*dy - 0.5 * ly;
  }
  grid_z[0] = 0; grid_z[1] = 0;

  // Loop over steps
  while((fileinput >> t) and (step<nsteps)){
    std::cout << "Time t=" << t << " step=" << step << std::endl;
    for(int i=0;i<np;i++){
      fileinput >> x[i] >> y[i] >> z[i];
    }
    if(step>skip and ((step % sample) == 0)){
      // Set concentration to zero
      for(int i=0; i < mx*my; i++){
	c[i] = 0;
      }

      // Loop over particles and save as concentration
      for(int i=0;i<npout;i++){
	x[i] = x[i] - (int(x[i]/lx + 0.5*((x[i]>0)-(x[i]<0)))) * lx;
	y[i] = y[i] - (int(y[i]/ly + 0.5*((y[i]>0)-(y[i]<0)))) * ly;
	  
	int jx   = int(x[i] / dx + 0.5*mx) % mx;
	int jy   = int(y[i] / dy + 0.5*my) % my;
	int icel = jx + jy * mx;
	c[icel] += inverse_volume_cell;
      }

      // Save snapshot by index
      string savefile;
      int index = step;
      stringstream s;
      s << index;
      string st = s.str();
      savefile = outputname + "." + st + ".concentration.vtk";

      cout << savefile << endl;
     
      /*Use visit_writer to write a regular mesh with data. */
      write_rectilinear_mesh(savefile.c_str(),// Output file
       			     0,               // 0=ASCII,  1=Binary
       			     dims,            // {mx, my, mz}
       			     grid_x,           // array 
       			     grid_y,
       			     grid_z,
       			     nvars,           // number of variables
      			     vardims,         // Size of each variable, 1=scalar, velocity=3*scalars
       			     centering,       // 0 nodal, 1 zonal
       			     varnames,   
       			     vars);
    }
    step++;
  }
  

  // Free memory
  delete[] c;
  delete[] vardims;
  delete[] centering;
  delete[] varnames;
  delete[] vars;
  delete[] grid_x;
  delete[] grid_y;
  delete[] grid_z;
}