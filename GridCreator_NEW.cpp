#include "GridCreator_NEW.h"

#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include "omp.h"

#include <algorithm>

#include <cassert>

/*
 * Defines for the column number of the properties:
 */
#define COLUMN_PERMEABILITY 4
#define COLUMN_PERMITTIVITY 5
#define COLUMN_ELEC_CONDUC  6
#define COLUMN_MAGN_CONDUC  7

/* CONSTRUCTOR */
GridCreator_NEW::GridCreator_NEW(InputParser &input_parser,
					    Materials &materials,
					    MPI_Initializer &MPI_communicator,
                        ProfilingClass &profiler):
                        input_parser(input_parser),
                        materials(materials),
                        MPI_communicator(MPI_communicator),
                        profiler(profiler){
    // Retrieve the spatial steps for the electromagnetic and thermal grids:
    this->delta_Electromagn[0] = this->input_parser.deltaX_Electro;
    this->delta_Electromagn[1] = this->input_parser.deltaY_Electro;
    this->delta_Electromagn[2] = this->input_parser.deltaZ_Electro;
    this->delta_Thermal        = this->input_parser.delta_Thermal;

    // Call the MPI division function, from the MPI_communicator field.
    // It retrieves the number of nodes for the electromagnetic grid along each direction:
	this->MPI_communicator.MPI_DIVISION(*this);

}

/* DESTRUCTOR */
GridCreator_NEW::~GridCreator_NEW(void){
    #ifndef NDEBUG
        std::cout << "GridCreator_NEW::~GridCreator_NEW::IN" << std::endl;
    #endif
    /* FREE ALLOCATED SPACE */

    // E_x:
    if(this->E_x != NULL){
        delete[] this->E_x;
    }
    // E_x_material:
    if(this->E_x_material !=NULL){
        delete[] this->E_x_material;
    }
    // E_x_eps:
    if(this->E_x_eps != NULL){
        delete[] this->E_x_eps;
    }
    // E_x_electrical_cond
    if(this->E_x_electrical_cond != NULL){
        delete[] this->E_x_electrical_cond;
    }
    
    // E_y:
    if(this->E_y != NULL){
        delete[] this->E_y;
    }
    // E_y_material:
    if(this->E_y_material != NULL){
        delete[] this->E_y_material;
    }
    // E_y_eps:
    if(this->E_y_eps != NULL){
        delete[] this->E_y_eps;
    }
    // E_y_electrical_cond
    if(this->E_y_electrical_cond != NULL){
        delete[] this->E_y_electrical_cond;
    }

    // E_z:
    if(this->E_z != NULL){
        delete[] this->E_z;
    }
    // E_z_material:
    if(this->E_z_material != NULL){
        delete[] this->E_z_material;
    }
    // E_z_eps:
    if(this->E_z_eps != NULL){
        delete[] this->E_z_eps;
    }
    // E_z_electrical_cond
    if(this->E_z_electrical_cond != NULL){
        delete[] this->E_z_electrical_cond;
    }

    // H_x:
    if(this->H_x != NULL){
        delete[] this->H_x;
    }
    // H_x_material:
    if(this->H_x_material!= NULL){
        delete[] this->H_x_material;
    }
    // H_x_mu:
    if(this->H_x_mu != NULL){
        delete[] this->H_x_mu;
    }
    // H_x_magnetic_cond:
    if(this->H_x_magnetic_cond != NULL){
        delete[] this->H_x_magnetic_cond;
    }

    // H_y:
    if(this->H_y != NULL){
        delete[] this->H_y;
    }
    // H_y_material:
    if(this->H_y_material != NULL){
        delete[] this->H_y_material;   
    }
    // H_y_mu:
    if(this->H_y_mu != NULL){
        delete[] this->H_y_mu;
    }
    // H_y_magnetic_cond:
    if(this->H_y_magnetic_cond != NULL){
        delete[] this->H_y_magnetic_cond;
    }

    // H_z:
    if(this->H_z != NULL){
        delete[] this->H_z;
    }
    // H_z_material:
    if(this->H_z_material != NULL){
        delete[] this->H_z_material;
    }
    // H_z_mu:
    if(this->H_z_mu != NULL){
        delete[] this->H_z_mu;
    }
    // H_z_magnetic_cond:
    if(this->H_z_magnetic_cond != NULL){
        delete[] this->H_z_magnetic_cond;
    }

    // Temperature:
    if(this->temperature != NULL){
        delete[] this->temperature;
    }
    // Temperature_material:
    if(this->temperature_material != NULL){
        delete[] this->temperature_material;
    }
    // Thermal conductivity:
    if(this->thermal_conductivity != NULL){
        delete[] this->thermal_conductivity;
    }
    // Thermal diffusivity:
    if(this->thermal_diffusivity != NULL){
        delete[] this->thermal_diffusivity;
    }
    #ifndef NDEBUG
        std::cout << "GridCreator_NEW::~GridCreator_NEW::OUT" << std::endl;
    #endif
}

/* GRID INITIALIZATION */
void GridCreator_NEW::meshInitialization(void){
    
    // Timing the grid initialization (in CPU time):
    double start_grid_init;
    double end___grid_init;
    this->profiler.addTimingInputToDictionnary("Grid_meshInit_omp_get_wtime");
    start_grid_init = omp_get_wtime();  

    size_t M = this->sizes_EH[0];
    size_t N = this->sizes_EH[1];
    size_t P = this->sizes_EH[2];

    size_t size;

    if(M == 0 || N == 0 || P == 0){
        fprintf(stderr,"GridCreator_NEW::meshInitialization::ERROR\n");
        fprintf(stderr,"\t>>> One of the quantities (M,N,P)=(%zu,%zu,%zu) is invalid.\nAborting.\n",M,N,P);
        fprintf(stderr,"\t>>> At line %d, in file %s.\n",__LINE__,__FILE__);
        abort();
    }

    /**
     * For each field (magnetic or electric), add 2 nodes in each direction so that 
     * we have information on what happens inside the MPI processes around the current MPI
     * process.
     */


    /* ALLOCATE SPACE FOR THE ELECTRIC FIELDS */

    /// Remove one if necessary (see paper and size of the global grid):
    size_t REMOVE_ONE = 1;

    // Size of E_x is  (M − 1) × N × P. Add 2 nodes in each direction for the neighboors.

    if(this->MPI_communicator.must_add_one_to_E_X_along_XYZ[0] == true){
        this->size_Ex[0] = M + 2 - REMOVE_ONE;
    }else{
        this->size_Ex[0] = M + 2;
    }
    if(this->MPI_communicator.must_add_one_to_E_X_along_XYZ[1] == true){
        this->size_Ex[1] = N + 2 - REMOVE_ONE;
    }else{
        this->size_Ex[1] = N + 2;        
    }
    if(this->MPI_communicator.must_add_one_to_E_X_along_XYZ[2] == true){
        this->size_Ex[2] = P + 2 - REMOVE_ONE;
    }else{
        this->size_Ex[2] = P + 2;
    }
    
    size = this->size_Ex[0] * this->size_Ex[1] * this->size_Ex[2];

    this->E_x                 = new double[size]();
    this->E_x_material        = new unsigned char[size]();
    this->E_x_eps             = new double[size]();
    this->E_x_electrical_cond = new double [size]();

    // Size of E_y is  M × (N − 1) × P. Add 2 nodes in each direction for the neighboors.
    if(this->MPI_communicator.must_add_one_to_E_Y_along_XYZ[0] == true){
        this->size_Ey[0] = M + 2 - REMOVE_ONE;
    }else{
        this->size_Ey[0] = M + 2;
    }

    if(this->MPI_communicator.must_add_one_to_E_Y_along_XYZ[1] == true){
        this->size_Ey[1] = N + 2 - REMOVE_ONE;
    }else{
        this->size_Ey[1] = N + 2;
    }

    if(this->MPI_communicator.must_add_one_to_E_Y_along_XYZ[2] == true){
        this->size_Ey[2] = P + 2 - REMOVE_ONE;
    }else{
        this->size_Ey[2] = P + 2;
    }

    size = this->size_Ey[0] * this->size_Ey[1] * this->size_Ey[2];

    this->E_y                 = new double[size]();
    this->E_y_material        = new unsigned char[size]();
    this->E_y_eps             = new double[size]();
    this->E_y_electrical_cond = new double[size]();

    // Size of E_z is  M × N × (P − 1). Add 2 nodes in each direction for the neighboors.

    if(this->MPI_communicator.must_add_one_to_E_Z_along_XYZ[0] == true){
        this->size_Ez[0] = M + 2 - REMOVE_ONE;
    }else{
        this->size_Ez[0] = M + 2;
    }

    if(this->MPI_communicator.must_add_one_to_E_Z_along_XYZ[1] == true){
        this->size_Ez[1] = N + 2 - REMOVE_ONE;
    }else{
        this->size_Ez[1] = N + 2;
    }

    if(this->MPI_communicator.must_add_one_to_E_Z_along_XYZ[2] == true){
        this->size_Ez[2] = P + 2 - REMOVE_ONE;
    }else{
        this->size_Ez[2] = P + 2;
    }

    size = this->size_Ez[0] * this->size_Ez[1] * this->size_Ez[2];

    this->E_z                 = new double[size]();
    this->E_z_material        = new unsigned char[size]();
    this->E_z_eps             = new double[size]();
    this->E_z_electrical_cond = new double[size]();

    /* ALLOCATE SPACE FOR THE MAGNETIC FIELDS */

    // Size of H_x is  M × (N − 1) × (P − 1). Add 2 nodes in each direction for the neighboors.

    if(this->MPI_communicator.must_add_one_to_H_X_along_XYZ[0] == true){
        this->size_Hx[0] = M + 2 - REMOVE_ONE;
    }else{
        this->size_Hx[0] = M + 2;
    }

    if(this->MPI_communicator.must_add_one_to_H_X_along_XYZ[1] == true){
        this->size_Hx[1] = N + 2 - REMOVE_ONE;
    }else{
        this->size_Hx[1] = N + 2;
    }

    if(this->MPI_communicator.must_add_one_to_H_X_along_XYZ[2] == true){
        this->size_Hx[2] = P + 2 - REMOVE_ONE;
    }else{
        this->size_Hx[2] = P + 2;
    }

    size = this->size_Hx[0] * 
            this->size_Hx[1] * 
            this->size_Hx[2];

    this->H_x               = new double[size]();
    this->H_x_material      = new unsigned char[size]();
    this->H_x_magnetic_cond = new double[size]();
    this->H_x_mu            = new double[size]();

    // Size of H_y is  (M − 1) × N × (P − 1). Add 2 nodes in each direction for the neighboors.

    if(this->MPI_communicator.must_add_one_to_H_Y_along_XYZ[0] == true){
        this->size_Hy[0] = M + 2 - REMOVE_ONE;
    }else{
        this->size_Hy[0] = M + 2;
    }

    if(this->MPI_communicator.must_add_one_to_H_Y_along_XYZ[1] == true){
        this->size_Hy[1] = N + 2 - REMOVE_ONE;
    }else{
        this->size_Hy[1] = N + 2;
    }

    if(this->MPI_communicator.must_add_one_to_H_Y_along_XYZ[2] == true){
        this->size_Hy[2] = P + 2 - REMOVE_ONE;
    }else{
        this->size_Hy[2] = P + 2;
    }

    size = this->size_Hy[0]
             * this->size_Hy[1]
             * this->size_Hy[2];

    this->H_y               = new double[size]();
    this->H_y_material      = new unsigned char[size]();
    this->H_y_mu            = new double[size]();
    this->H_y_magnetic_cond = new double[size]();

    // Size of H_z is  (M − 1) × (N − 1) × P. Add 2 nodes in each direction for the nieghboors.
    if(this->MPI_communicator.must_add_one_to_H_Z_along_XYZ[0] == true){
        this->size_Hz[0] = M + 2 - REMOVE_ONE;
    }else{
        this->size_Hz[0] = M + 2;
    }

    if(this->MPI_communicator.must_add_one_to_H_Z_along_XYZ[1] == true){
        this->size_Hz[1] = N + 2 - REMOVE_ONE;
    }else{
        this->size_Hz[1] = N + 2;
    }

    if(this->MPI_communicator.must_add_one_to_H_Z_along_XYZ[2] == true){
        this->size_Hz[2] = P + 2 - REMOVE_ONE;
    }else{
        this->size_Hz[2] = P + 2;
    }

    size = this->size_Hz[0]
             * this->size_Hz[1]
             * this->size_Hz[2];

    this->H_z               = new double[size]();
    this->H_z_material      = new unsigned char[size]();
    this->H_z_mu            = new double[size]();
    this->H_z_magnetic_cond = new double[size]();

    /* ALLOCATE SPACE FOR THE TEMPERATURE FIELD */

    // The temperature grid is homogeneous. (this->size_Thermal^3).
    size_t T = this->size_Thermal[0] * this->size_Thermal[1] * this->size_Thermal[2];

    if(T == 0){
        fprintf(stderr,"GridCreator_NEW::meshInitialization::ERROR\n");
        fprintf(stderr,"Your temperature grid is empty (size T is 0).\n");
        fprintf(stderr,"Aborting.\nFile %s:%d\n",__FILE__,__LINE__);
        abort();
    }

    this->temperature          = new double[T]();
    this->temperature_material = new unsigned char[T]();
    this->thermal_conductivity = new double[T]();
    this->thermal_diffusivity  = new double[T]();

    /* INITIALIZATION OF THE NODES */
    #ifndef NDEBUG
        printf("[MPI %d] - Assigning material...\n",this->MPI_communicator.getRank());
    #endif
    this->Assign_A_Material_To_Each_Node();

    /* INITIALIZATION OF TEMPERATURE NODES (give a initial temperature) */
    #ifndef NDEBUG
        printf("[MPI %d] - Assigning initial temperature...\n",this->MPI_communicator.getRank());
    #endif
    this->Assign_Init_Temperature_to_Temperature_nodes();

    // Get elapsed CPU time:
    end___grid_init = omp_get_wtime(); 
    double elapsedTimeSec = end___grid_init - start_grid_init;
    this->profiler.incrementTimingInput("Grid_meshInit_omp_get_wtime",elapsedTimeSec);

    #ifndef NDEBUG
        printf("[MPI %d] - Grid initialization in %.5lf seconds.\n",
            this->MPI_communicator.getRank(),
            elapsedTimeSec);
    #endif

}

/* Assign a material to each node (both electromagnetic and thermal grid)*/
void GridCreator_NEW::Assign_A_Material_To_Each_Node(){
    /*
     * This function fills in the vectors of material.
     */
    /// Verify the simulation type:
    if( this->input_parser.get_SimulationType() != "USE_AIR_EVERYWHERE"
        && this->input_parser.get_SimulationType() != "TEST_PARAVIEW"
        && this->input_parser.get_SimulationType() != "TEST_PARAVIEW_MPI")
    {
        fprintf(stderr,"GridCreator_NEW::Assign_A_Material_To_Each_Node()::ERROR\n");
        fprintf(stderr,"\t>>> Simulation type (%s) doesn't correspond to any known type. Aborting.\n",
            this->input_parser.get_SimulationType().c_str());
        fprintf(stderr,"\tFile %s:%d\n",__FILE__,__LINE__);
        abort();
    }

    // Assign material as a function of the simulation type.

    GridCreator_NEW *ref_obj = this;

    /////////////////////////////////////////
    /// FILL IN WITH PARALLEL OMP THREADS ///
    /////////////////////////////////////////
    #pragma omp parallel default(none) firstprivate(ref_obj)
        {
            size_t index;
            double *E_x = this->E_x;
            double *E_y = this->E_y;
            double *E_z = this->E_z;
            unsigned char *E_x_material = this->E_x_material;
            unsigned char *E_y_material = this->E_y_material;
            unsigned char *E_z_material = this->E_z_material;

            double *H_x = this->H_x;
            double *H_y = this->H_y;
            double *H_z = this->H_z;
            unsigned char *H_x_material = this->H_x_material;
            unsigned char *H_y_material = this->H_y_material;
            unsigned char *H_z_material = this->H_z_material;

            std::vector<size_t> size_Ex = this->size_Ex;
            std::vector<size_t> size_Ey = this->size_Ey;
            std::vector<size_t> size_Ez = this->size_Ez;

            std::vector<size_t> size_Hx = this->size_Hx;
            std::vector<size_t> size_Hy = this->size_Hy;
            std::vector<size_t> size_Hz = this->size_Hz;

            std::vector<size_t> size_Thermal = this->size_Thermal;
            unsigned char *temperature_material = this->temperature_material;
            double *temperature = ref_obj->temperature;
            
            bool is_USE_AIR_EVERYWHERE       = false;
            bool is_TEST_PARAVIEW            = false;
            bool is_TEST_PARAVIEW_MPI        = false;
            bool is_GLOBAL_TEST_PARAVIEW_MPI_ELECTRIC = false;
            bool is_GLOBAL_TEST_PARAVIEW_MPI_MAGNETIC = false;
            bool is_RANK_TEST_PARAVIEW_MPI_TEMP       = false;
            bool is_RANK_TEST_PARAVIEW_MPI_ELECTRIC   = false;
            bool is_RANK_TEST_PARAVIEW_MPI_MAGNETIC   = false;
            int  RANK_MPI                             = ref_obj->MPI_communicator.getRank() + 1;
            bool is_LOCAL_TEST_PARAVIEW_MPI_ELECTRIC  = false;

            std::map<std::basic_string<char>, unsigned char> materialID_FromMaterialName = ref_obj->materials.materialID_FromMaterialName;

            if(ref_obj->input_parser.get_SimulationType() == "USE_AIR_EVERYWHERE"){
                is_USE_AIR_EVERYWHERE = true;

            }else if(ref_obj->input_parser.get_SimulationType() == "TEST_PARAVIEW"){
                is_TEST_PARAVIEW      = true;

            }else if(ref_obj->input_parser.get_SimulationType() == "TEST_PARAVIEW_MPI"){
                is_TEST_PARAVIEW_MPI  = true;

                if(ref_obj->input_parser.TEST_PARAVIEW_MPI_ARGS["E"] == "GLOBAL"){
                    is_GLOBAL_TEST_PARAVIEW_MPI_ELECTRIC = true;
                }else if(ref_obj->input_parser.TEST_PARAVIEW_MPI_ARGS["E"] == "RANK"){
                    is_RANK_TEST_PARAVIEW_MPI_ELECTRIC   = true;
                }else if(ref_obj->input_parser.TEST_PARAVIEW_MPI_ARGS["E"] == "LOCAL"){
                    is_LOCAL_TEST_PARAVIEW_MPI_ELECTRIC = true;
                }else{
                    abort();
                }

                if(ref_obj->input_parser.TEST_PARAVIEW_MPI_ARGS["H"] == "GLOBAL"){
                    is_GLOBAL_TEST_PARAVIEW_MPI_MAGNETIC = true;
                }else if(ref_obj->input_parser.TEST_PARAVIEW_MPI_ARGS["H"] == "RANK"){
                    is_RANK_TEST_PARAVIEW_MPI_MAGNETIC   = true;
                }

                if(ref_obj->input_parser.TEST_PARAVIEW_MPI_ARGS["TEMP"] == "RANK"){
                    is_RANK_TEST_PARAVIEW_MPI_TEMP = true;
                }

            }else{
                printf("FATAL ERROR. SORRY.\n");
                abort();
            }
            // EX field:
            #pragma omp for nowait
            for(size_t K = 0 ; K < size_Ex[2] ; K ++){
                for(size_t J = 0 ; J < size_Ex[1] ; J ++ ){
                    for(size_t I = 0 ; I < size_Ex[0] ; I ++){

                        /// Compute the index:
                        index = I + size_Ex[0] * ( J + size_Ex[1] * K );

                        /// Determine the initial condition to impose.

                        if(is_USE_AIR_EVERYWHERE){
                                unsigned char mat = materialID_FromMaterialName["AIR"];
                                E_x_material[index] = mat;
                        }else if(is_TEST_PARAVIEW){
                            E_x[index] = I;
                        }else if(is_TEST_PARAVIEW_MPI){
                            if(is_GLOBAL_TEST_PARAVIEW_MPI_ELECTRIC){
                                /// Put the global index:
                                size_t local[3];
                                local[0] = I;
                                local[1] = J;
                                local[2] = K;
                                size_t global[3];
                                ref_obj->get_Global_from_Local_Electro(local,global);
                                E_x[index] = global[0];
                            }else if(is_RANK_TEST_PARAVIEW_MPI_ELECTRIC){
                                E_x[index] = RANK_MPI;
                            }else if(is_LOCAL_TEST_PARAVIEW_MPI_ELECTRIC){
                                E_x[index] = I;
                            }else{
                                /// Put the local index:
                                E_x[index] = I;
                            }
                        }else{
                            abort();
                        }
                        
                    }
                }
            }

            // EY field:
            #pragma omp for nowait
            for(size_t K = 0 ; K < size_Ey[2] ; K ++){
                for(size_t J = 0 ; J < size_Ey[1] ; J ++ ){
                    for(size_t I = 0 ; I < size_Ey[0] ; I ++){

                        /// Determine the index:

                        index = I + size_Ey[0] * ( J + size_Ey[1] * K );

                        /// Determine the initial condition to impose.

                        if(is_USE_AIR_EVERYWHERE){

                            unsigned char mat = materialID_FromMaterialName["AIR"];
                            E_y_material[index] = mat;

                        }else if(is_TEST_PARAVIEW){
                            E_y[index] = J;
                        }else if(is_TEST_PARAVIEW_MPI){
                            if(is_GLOBAL_TEST_PARAVIEW_MPI_ELECTRIC){
                                /// Put the global index:
                                size_t local[3];
                                local[0] = I;
                                local[1] = J;
                                local[2] = K;
                                size_t global[3];
                                ref_obj->get_Global_from_Local_Electro(local,global);
                                E_y[index] = global[1];
                            }else if(is_RANK_TEST_PARAVIEW_MPI_ELECTRIC){
                                E_y[index] = RANK_MPI;
                            }else if(is_LOCAL_TEST_PARAVIEW_MPI_ELECTRIC){
                                E_y[index] = J;
                            }else{
                                /// Put the local index:
                                E_y[index] = J;
                            }
                        }else{
                            abort();
                        }
                        
                    }
                }
            }

            // EZ field:
            #pragma omp for nowait
            for(size_t K = 0 ; K < size_Ez[2] ; K ++){
                for(size_t J = 0 ; J < size_Ez[1] ; J ++ ){
                    for(size_t I = 0 ; I < size_Ez[0] ; I ++){

                        /// Determine the index:
                        index = I + size_Ez[0] * ( J + size_Ez[1] * K );

                        /// Determine the initial condition to impose.

                        if(is_USE_AIR_EVERYWHERE){

                            unsigned char mat = materialID_FromMaterialName["AIR"];
                            E_z_material[index] = mat;

                        }else if(is_TEST_PARAVIEW){
                            E_y[index] = K;
                        }else if(is_TEST_PARAVIEW_MPI){
                            if(is_GLOBAL_TEST_PARAVIEW_MPI_ELECTRIC){
                                /// Put the global index:
                                size_t local[3];
                                local[0] = I;
                                local[1] = J;
                                local[2] = K;
                                size_t global[3];
                                ref_obj->get_Global_from_Local_Electro(local,global);
                                E_z[index] = global[2];
                            }else if(is_RANK_TEST_PARAVIEW_MPI_ELECTRIC){
                                E_z[index] = RANK_MPI;
                            }else if(is_LOCAL_TEST_PARAVIEW_MPI_ELECTRIC){
                                E_z[index] = K;
                            }else{
                                /// Put the local index:
                                E_z[index] = K;
                            }
                        }else{
                            abort();
                        }
                        
                    }
                }
            }

            // HX field:
            #pragma omp for nowait
            for(size_t K = 0 ; K < size_Hx[2] ; K ++){
                for(size_t J = 0 ; J < size_Hx[1] ; J ++ ){
                    for(size_t I = 0 ; I < size_Hx[0] ; I ++){

                        /// Determine the index:
                        index = I + size_Hx[0] * ( J + size_Hx[1] * K );

                        /// Determine the initial condition to impose.

                        if(is_USE_AIR_EVERYWHERE){

                            unsigned char mat = materialID_FromMaterialName["AIR"];
                            H_x_material[index] = mat;

                        }else if(is_TEST_PARAVIEW){
                            H_x[index] = I;
                        }else if(is_TEST_PARAVIEW_MPI){
                            if(is_GLOBAL_TEST_PARAVIEW_MPI_MAGNETIC){
                                /// Put the global index:
                                size_t local[3];
                                local[0] = I;
                                local[1] = J;
                                local[2] = K;
                                size_t global[3];
                                ref_obj->get_Global_from_Local_Electro(local,global);
                                H_x[index] = global[0];
                            }else if(is_RANK_TEST_PARAVIEW_MPI_MAGNETIC){
                                H_x[index] = RANK_MPI;
                            }else{
                                /// Put the local index:
                                H_x[index] = I;
                            }
                        }else{
                            abort();
                        }
                        
                    }
                }
            }

            // HY field:
            #pragma omp for nowait
            for(size_t K = 0 ; K < size_Hy[2] ; K ++){
                for(size_t J = 0 ; J < size_Hy[1] ; J ++ ){
                    for(size_t I = 0 ; I < size_Hy[0] ; I ++){

                        /// Determine the index:
                        index = I + size_Hy[0] * ( J + size_Hy[1] * K );

                        /// Determine the initial condition to impose.

                        if(is_USE_AIR_EVERYWHERE){

                            unsigned char mat = materialID_FromMaterialName["AIR"];
                            H_y_material[index] = mat;

                        }else if(is_TEST_PARAVIEW){
                            H_y[index] = J;
                        }else if(is_TEST_PARAVIEW_MPI){
                            if(is_GLOBAL_TEST_PARAVIEW_MPI_MAGNETIC){
                                /// Put the global index:
                                size_t local[3];
                                local[0] = I;
                                local[1] = J;
                                local[2] = K;
                                size_t global[3];
                                ref_obj->get_Global_from_Local_Electro(local,global);
                                H_y[index] = global[1];
                            }else if(is_RANK_TEST_PARAVIEW_MPI_MAGNETIC){
                                H_y[index] = RANK_MPI;
                            }else{
                                /// Put the local index:
                                H_y[index] = J;
                            }
                        }else{
                            abort();
                        }
                        
                    }
                }
            }

            // HZ field:
            #pragma omp for nowait
            for(size_t K = 0 ; K < size_Hz[2] ; K ++){
                for(size_t J = 0 ; J < size_Hz[1] ; J ++ ){
                    for(size_t I = 0 ; I < size_Hz[0] ; I ++){

                        /// Determine the index:
                        index = I + size_Hz[0] * ( J + size_Hz[1] * K );

                        /// Determine the initial condition to impose.

                        if(is_USE_AIR_EVERYWHERE){

                            unsigned char mat = materialID_FromMaterialName["AIR"];
                            H_z_material[index] = mat;

                        }else if(is_TEST_PARAVIEW){
                            H_z[index] = K;

                        }else if(is_TEST_PARAVIEW_MPI){

                            if(is_GLOBAL_TEST_PARAVIEW_MPI_MAGNETIC){
                                /// Put the global index:
                                size_t local[3];
                                local[0] = I;
                                local[1] = J;
                                local[2] = K;
                                size_t global[3];
                                ref_obj->get_Global_from_Local_Electro(local,global);
                                H_z[index] = global[2];
                            }else if(is_RANK_TEST_PARAVIEW_MPI_MAGNETIC){
                                H_z[index] = RANK_MPI;
                            }else{
                                /// Put the local index:
                                H_z[index] = K;
                            }
                        }else{
                            abort();
                        }
                        
                    }
                }
            }
                  
            // Temperature nodes:
            #pragma omp for nowait
                    for(size_t K = 0 ; K < size_Thermal[2] ; K++){
                        for(size_t J = 0 ; J < size_Thermal[1] ; J ++){
                            for(size_t I = 0 ; I < size_Thermal[0] ; I ++){
                                
                                /// Determine the index:
                                index = I + size_Thermal[0] * (J + size_Thermal[1] *K);

                                /// Determine the initial condition to impose.

                                if(is_USE_AIR_EVERYWHERE){

                                    unsigned char mat = materialID_FromMaterialName["AIR"];
                                    temperature_material[index] = mat;
                                }else if(is_TEST_PARAVIEW){
                                    ref_obj->temperature[index] = -1;
                                }else if(is_TEST_PARAVIEW_MPI){
                                    if(is_RANK_TEST_PARAVIEW_MPI_TEMP){
                                        /// Put the MPI rank:
                                        temperature[index] = RANK_MPI;
                                    }else{
                                        /// Put -1:
                                        temperature[index] = -1;
                                    }
                                }else{
                                    abort();
                                }

                            }
                        }
                    }
        } /* END OF PARALLEL REGION */
}

// Assign to each node of the temperature field an initial temperature:
void GridCreator_NEW::Assign_Init_Temperature_to_Temperature_nodes(void){

    unsigned int DEFAULT = 4;
    unsigned int nbr_omp_threads = 0;
    if((unsigned)omp_get_num_threads() > DEFAULT){
            nbr_omp_threads = omp_get_num_threads();
    }else{
        nbr_omp_threads = DEFAULT;
    }

    size_t index;

    if(this->input_parser.get_SimulationType() == "USE_AIR_EVERYWHERE"){

        double init_air_temp = this->input_parser.GetInitTemp_FromMaterialName["AIR"];

        #pragma omp parallel num_threads(nbr_omp_threads)\
            private(index)
        {
            #pragma omp for collapse(3)
                    for(size_t I = 0 ; I < this->size_Thermal[0] ; I++){
                        for(size_t J = 0 ; J < this->size_Thermal[1] ; J ++){
                            for(size_t K = 0 ; K < this->size_Thermal[2] ; K ++){

                                index = I + this->size_Thermal[0] * (J + this->size_Thermal[1] * K);
                                this->temperature[index] = init_air_temp;
                            }
                        }
                    }
        }
    }else if(this->input_parser.get_SimulationType() == "TEST_PARAVIEW"){
        /**
         * @brief In the case of "TEST_PARAVIEW", fill in with I.
         */
        #pragma omp parallel num_threads(nbr_omp_threads)\
            private(index)
        {
            #pragma omp for collapse(3)
                    for(size_t K = 0 ; K < this->size_Thermal[2] ; K++){
                        for(size_t J = 0 ; J < this->size_Thermal[1] ; J ++){
                            for(size_t I = 0 ; I < this->size_Thermal[0] ; I ++){

                                index = I + this->size_Thermal[0] *
                                         (J + this->size_Thermal[1] * K);;

                                this->temperature[index] = I;

                            }
                        }
                    }
        }
    }else if(this->input_parser.get_SimulationType() == "TEST_PARAVIEW_MPI"){
        /**
         * We put the rank of the current MPI process inside the temperature field.
         */
        int rank_mpi = this->MPI_communicator.getRank();
        #pragma omp parallel num_threads(nbr_omp_threads)\
            private(index)
        {
            #pragma omp for collapse(3)
                    for(size_t K = 0 ; K < this->size_Thermal[2] ; K++){
                        for(size_t J = 0 ; J < this->size_Thermal[1] ; J ++){
                            for(size_t I = 0 ; I < this->size_Thermal[0] ; I ++){

                                index = I + this->size_Thermal[0] *
                                         (J + this->size_Thermal[1] * K);;

                                this->temperature[index] = (double)rank_mpi;

                            }
                        }
                    }
        }
    }else{
        fprintf(stderr,"GridCreator_NEW::Assign_Temperature_to_Temperature_nodes::ERROR\n");
        fprintf(stderr,"\t>>> Simulation type doesn't correspond to any known type. Aborting.\n");
        fprintf(stderr,"\tFile %s:%d\n",__FILE__,__LINE__);
        abort();
    }
}

// Assign to each electromagnetic node its properties as a function of the temperature:
void GridCreator_NEW::Initialize_Electromagnetic_Properties(std::string whatToDo /*= string()*/){

    #ifndef NDEBUG
        std::cout << "GridCreator_NEW::Initialize_Electromagnetic_Properties::IN" << std::endl;
    #endif

    unsigned int DEFAULT = 4;
    unsigned int nbr_omp_threads = 0;
    if((unsigned)omp_get_num_threads() > DEFAULT){
            nbr_omp_threads = omp_get_num_threads();
    }else{
        nbr_omp_threads = DEFAULT;
    }

    // Decide what to do based on the argument string 'whatToDo':

    if(whatToDo == "AIR_AT_INIT_TEMP"){
        /*
         * The nodes properties (mu, eps, magn. and elec. cond.) are assigned by assuming initial
         * air temperature.
         */

        size_t index;

        // Retrieve the air temperature:
        double air_init_temp = this->input_parser.GetInitTemp_FromMaterialName["AIR"];
        unsigned char mat    = this->materials.materialID_FromMaterialName["AIR"];
        double eps           = this->materials.getProperty(air_init_temp,
                                                            mat,
                                                            COLUMN_PERMITTIVITY);
        double electric_cond = this->materials.getProperty(air_init_temp,
                                                            mat,
                                                            COLUMN_ELEC_CONDUC);
        double mu            = this->materials.getProperty(air_init_temp,
                                                            mat,
                                                            COLUMN_PERMEABILITY);
        double magnetic_cond = this->materials.getProperty(air_init_temp,
                                                            mat,
                                                            COLUMN_MAGN_CONDUC);

        #pragma omp parallel num_threads(nbr_omp_threads)
        {
            // EX field:
            #pragma omp for collapse(3) nowait\
                private(index)
            for(size_t K = 0 ; K < this->size_Ex[2] ; K ++){
                for(size_t J = 0 ; J < this->size_Ex[1] ; J ++ ){
                    for(size_t I = 0 ; I < this->size_Ex[0] ; I ++){

                        index = I + this->size_Ex[0] * ( J + this->size_Ex[1] * K );
                        this->E_x_eps[index] = eps;
                        this->E_x_electrical_cond[index] = electric_cond;
                        
                    }
                }
            }

            // EY field:
            #pragma omp for collapse(3) nowait\
                private(index)
            for(size_t K = 0 ; K < this->size_Ey[2] ; K ++){
                for(size_t J = 0 ; J < this->size_Ey[1] ; J ++ ){
                    for(size_t I = 0 ; I < this->size_Ey[0] ; I ++){

                        index = I + this->size_Ey[0] * ( J + this->size_Ey[1] * K );
                        this->E_y_eps[index] = eps;
                        this->E_y_electrical_cond[index] = electric_cond;
                        
                    }
                }
            }

            // EZ field:
            #pragma omp for collapse(3) nowait\
                private(index)
            for(size_t K = 0 ; K < this->size_Ez[2] ; K ++){
                for(size_t J = 0 ; J < this->size_Ez[1] ; J ++ ){
                    for(size_t I = 0 ; I < this->size_Ez[0] ; I ++){

                        index = I + this->size_Ez[0] * ( J + this->size_Ez[1] * K );
                        this->E_z_eps[index] = eps;
                        this->E_z_electrical_cond[index] = electric_cond;
                        
                    }
                }
            }

            // HX field:
            #pragma omp for collapse(3) nowait\
                private(index)
            for(size_t K = 0 ; K < this->size_Hx[2] ; K ++){
                for(size_t J = 0 ; J < this->size_Hx[1] ; J ++ ){
                    for(size_t I = 0 ; I < this->size_Hx[0] ; I ++){

                        index = I + this->size_Hx[0] * ( J + this->size_Hx[1] * K );
                        this->H_x_mu[index] = mu;
                        this->H_x_magnetic_cond[index] = magnetic_cond;
                        
                    }
                }
            }

            // HY field:
            #pragma omp for collapse(3) nowait\
                private(index)
            for(size_t K = 0 ; K < this->size_Hy[2] ; K ++){
                for(size_t J = 0 ; J < this->size_Hy[1] ; J ++ ){
                    for(size_t I = 0 ; I < this->size_Hy[0] ; I ++){

                        index = I + this->size_Hy[0] * ( J + this->size_Hy[1] * K );
                        this->H_y_mu[index] = mu;
                        this->H_y_magnetic_cond[index] = magnetic_cond;
                        
                    }
                }
            }

            // HZ field:
            #pragma omp for collapse(3) nowait\
                private(index)
            for(size_t K = 0 ; K < this->size_Hz[2] ; K ++){
                for(size_t J = 0 ; J < this->size_Hz[1] ; J ++ ){
                    for(size_t I = 0 ; I < this->size_Hz[0] ; I ++){

                        index = I + this->size_Hz[0] * ( J + this->size_Hz[1] * K );
                        this->H_z_mu[index] = mu;
                        this->H_z_magnetic_cond[index] = magnetic_cond;
                        
                    }
                }
            }

        }

    }else{
        fprintf(stderr,"GridCreator_NEW::Initialize_Electromagnetic_Properties::ERROR\n");
        fprintf(stderr,"No 'whatToDo' corresponding to %s. Aborting.\n",whatToDo.c_str());
        fprintf(stderr,"In file %s:%d\n",__FILE__,__LINE__);
        abort();
    }

}

/**
 * @brief Get the global node number from the local node number, EM grid.
 */
void GridCreator_NEW::get_Global_from_Local_Electro(size_t *local,size_t* global){

    global[0] = local[0] + this->originIndices_Electro[0];
    global[1] = local[1] + this->originIndices_Electro[1];
    global[2] = local[2] + this->originIndices_Electro[2];

}

/**
 *  @brief Get the global node number from the local node number, Thermal grid.
 */
void GridCreator_NEW::get_Global_from_Local_Thermal(size_t *local,size_t* global){

    global[0] = local[0] + this->originIndices_Thermal[0];
    global[1] = local[1] + this->originIndices_Thermal[1];
    global[2] = local[2] + this->originIndices_Thermal[2];

}

/**
 * @brief Compute nodes that are inside the sources
 * 
 * Arguments are: 1) local_nodes_inside_source_NUMBER:
 *                      Gives the local number of the nodes which are inside the sources.
 *                      The number is the result of I + size_FIELD[0] * ( J + size_FIELD[1] * K).
 *                      Has the size [nbr_nodes_inside_source].
 *                2) ID_Source:
 *                      Contains the ID of the source in which the node is.
 *                      Has the size [nbr_nodes_inside_source].
 *                3) local_nodes_inside_source_FREQ:
 *                      Contains the frequency of each source.
 *                      Has the same size as the number of sources.
 *                4) nbr_nodes_inside_source:
 *                      Number of nodes inside the source.
 *                      Pointer to a single 'size_t'.
 * 
 */

class ReducerForOMP{
    public:
        template<typename T>
        void reduce_custom(std::vector<T> *v, int begin, int end){

            if(end - begin == 1){
                return;
            }

            int pivot = (begin+end)/2;

            #pragma omp task
                reduce_custom(v,begin,pivot);
            #pragma omp task
                reduce_custom(v,pivot,end);
            #pragma omp taskwait

            v[begin].insert(v[begin].end(),v[pivot].begin(),v[pivot].end());

        }
};

void GridCreator_NEW::Compute_nodes_inside_sources(
        std::vector<size_t>        &local_nodes_inside_source_NUMBER,
        std::vector<unsigned char> &ID_Source,
        std::vector<double>        &local_nodes_inside_source_FREQ,
        const std::string          &TYPE_OF_FIELD
    )
{
    #ifndef NDEBUG
        fprintf(stdout,"Hi ! You enter in %s, to compute which nodes are inside the sources (electro).\n",__FUNCTION__);
    #endif
    /// Check inputs:
    if(local_nodes_inside_source_NUMBER.size() != 0){
        // Means the array is possibly already allocated. Abort.
        fprintf(stderr,"In %s, 'local_nodes_inside_source_NUMBER' is not empty. Aborting.\n",
            __FUNCTION__);
        fprintf(stderr,"File %s:%d\n",__FILE__,__LINE__);
        abort();
    }
    if(local_nodes_inside_source_FREQ.size() != 0){
        // Means the array is possibly already allocated. Abort.
        fprintf(stderr,"In %s, 'local_nodes_inside_source_FREQ' is not empty. Aborting.\n",__FUNCTION__);
        fprintf(stderr,"File %s:%d\n",__FILE__,__LINE__);
        abort();
    }
    if(ID_Source.size() != 0){
        // Means the array is possibly already allocated. Abort.
        fprintf(stderr,"In %s :: 'ID_Source' is not empty. Aborting.\n",__FUNCTION__);
        fprintf(stderr,"File %s:%d\n",__FILE__,__LINE__);
        abort();
    }
    if(TYPE_OF_FIELD == std::string() 
        && TYPE_OF_FIELD != "Ex"
        && TYPE_OF_FIELD != "Ey"
        && TYPE_OF_FIELD != "Ez"
        && TYPE_OF_FIELD != "Hx"
        && TYPE_OF_FIELD != "Hy"
        && TYPE_OF_FIELD != "Hz"){
        fprintf(stderr,"In %s :: empty or wrong 'TYPE_OF_FIELD'. Aborting.\n",__FUNCTION__);
        abort();
    }

    std::vector<size_t>        *numbers_for_nodes;
    std::vector<unsigned char> *ID_for_nodes;
    std::vector<double>        *freq;

    std::vector<size_t> SIZES;

    if(TYPE_OF_FIELD == "Ex"){
        SIZES = this->size_Ex;

    }else if(TYPE_OF_FIELD == "Ey"){
        SIZES = this->size_Ey;

    }else if(TYPE_OF_FIELD == "Ez"){
        SIZES = this->size_Ez;

    }else if(TYPE_OF_FIELD == "Hx"){
        SIZES = this->size_Hx;

    }else if(TYPE_OF_FIELD == "Hy"){
        SIZES = this->size_Hy;
        
    }else if(TYPE_OF_FIELD == "Hz"){
        SIZES = this->size_Hz;

    }else{
        fprintf(stderr,"In %s :: wrong TYPE_OF_FIELD (has %s) ! Aborting.\n",__FUNCTION__,TYPE_OF_FIELD.c_str());
        fprintf(stderr,"File %s:%d\n",__FILE__,__LINE__);
        abort();
    }

    std::string type = TYPE_OF_FIELD;

    std::vector<size_t> I_max(omp_get_max_threads());
    for(size_t it = 0 ; it < I_max.size() ; it++ )
        I_max[it] = 0;

    std::vector<size_t> J_max(omp_get_max_threads());
    for(size_t it = 0 ; it < J_max.size() ; it++ )
        J_max[it] = 0;

    std::vector<size_t> I_min(omp_get_max_threads());
    for(size_t it = 0 ; it < I_min.size() ; it++ )
        I_min[it] = 5E5;

    std::vector<size_t> J_min(omp_get_max_threads());
    for(size_t it = 0 ; it < J_min.size() ; it++ )
        J_min[it] = 5E5;

    #pragma omp parallel default(none)\
        shared(numbers_for_nodes)\
        shared(freq)\
        shared(ID_for_nodes)\
        shared(local_nodes_inside_source_NUMBER)\
        shared(ID_Source)\
        shared(SIZES,type)\
        shared(I_min,I_max,J_min,J_max)
    {
        std::vector<size_t> SIZES_PRIVATE = SIZES;

        // Allocate:
        #pragma omp single
        {
            numbers_for_nodes = new std::vector<size_t>       [omp_get_num_threads()];
            ID_for_nodes      = new std::vector<unsigned char>[omp_get_num_threads()];
            freq              = new std::vector<double>       [omp_get_num_threads()];
        }

        size_t I,J,K;

        size_t local[3];
        size_t global[3];

        // Check for the nodes Ez:
        #pragma omp for schedule(static)
        for(K = 1 ; K < SIZES_PRIVATE[2]-1 ; K ++){
            for(J = 1 ; J < SIZES_PRIVATE[1]-1 ; J ++){
                for(I = 1 ; I < SIZES_PRIVATE[0]-1 ; I ++){

                    // Convert to global node numbering:
                    // Shift of 1 because we start the loop at 1:
                    local[0] = I-1;
                    local[1] = J-1;
                    local[2] = K-1;
                    this->get_Global_from_Local_Electro(local,global);

                    // Loop over all the sources:
                    for(unsigned char id = 0 ; id < this->input_parser.source.get_number_of_sources() ; id ++){
                        std::string res = this->input_parser.source.is_inside_source_Romin(
                            global[0],
                            global[1],
                            global[2],
                            this->delta_Electromagn,
                            type,
							this->input_parser.conditionsInsideSources[id],
                            id,
                            this->input_parser.origin_Electro_grid
                        );
                        if(res == "true")
                        {
                            /// Record min max
                            if(J > J_max[omp_get_thread_num()]){
                                J_max[omp_get_thread_num()] = J;
                            }
                            if(I > I_max[omp_get_thread_num()]){ 
                                I_max[omp_get_thread_num()] = I;
                            }
                            if(J < J_min[omp_get_thread_num()]){
                                J_min[omp_get_thread_num()] = J;
                            }
                            if(I_min[omp_get_thread_num()] > I){
                                I_min[omp_get_thread_num()] = I;
                            }
                            // The node is inside the source !!!
                            // 0 corresponds to Ex.

                            numbers_for_nodes[omp_get_thread_num()].push_back(
                                I + SIZES_PRIVATE[0] * ( J + SIZES_PRIVATE[1] * K)
                            );

                            ID_for_nodes[omp_get_thread_num()].push_back((unsigned char)id);
                        
                        }

                        if( res == "0"){
                            /// We must impose the field to 0.
                            /// Record min max
                            if(J > J_max[omp_get_thread_num()]){
                                J_max[omp_get_thread_num()] = J;
                            }
                            if(I > I_max[omp_get_thread_num()]){ 
                                I_max[omp_get_thread_num()] = I;
                            }
                            if(J < J_min[omp_get_thread_num()]){
                                J_min[omp_get_thread_num()] = J;
                            }
                            if(I_min[omp_get_thread_num()] > I){
                                I_min[omp_get_thread_num()] = I;
                            }
                            numbers_for_nodes[omp_get_thread_num()].push_back(
                                I + SIZES_PRIVATE[0] * ( J + SIZES_PRIVATE[1] * K)
                            );
                            ID_for_nodes[omp_get_thread_num()].push_back(UCHAR_MAX);
                        }

                    }
                }
            }
        } /* END OF EX */
        #pragma omp barrier
        #pragma omp single nowait
        {
            #ifndef NDEBUG
                printf("\n\n>>> FOR %s :: Index goes from (%zu,%zu) to (%zu,%zu)\n\n",
                    type.c_str(),
                    I_min[std::distance(I_min.begin(),std::min_element(I_min.begin(), I_min.end()))],
                    J_min[std::distance(J_min.begin(),std::min_element(J_min.begin(), J_min.end()))],
                    I_max[std::distance(I_max.begin(),std::max_element(I_max.begin(), I_max.end()))],
                    J_max[std::distance(J_max.begin(),std::max_element(J_max.begin(), J_max.end()))]);
            #endif
        }
        
        // Assemble all the results in one vector:
        #pragma omp single nowait
        {
            ReducerForOMP reducer;
            reducer.reduce_custom(numbers_for_nodes,0,omp_get_num_threads());
        }
        #pragma omp single
        {
            ReducerForOMP reducer;
            reducer.reduce_custom(ID_for_nodes,0,omp_get_num_threads());
        }

    }

    local_nodes_inside_source_NUMBER = numbers_for_nodes[0];
    ID_Source = ID_for_nodes[0];

    delete[] numbers_for_nodes;
    delete[] ID_for_nodes;
    delete[] freq;

}

bool GridCreator_NEW::is_global_inside_me(
    size_t nbr_X_gl,
    size_t nbr_Y_gl,
    size_t nbr_Z_gl
)
{
    if(    nbr_X_gl >= this->originIndices_Electro[0] 
        && nbr_X_gl < this->originIndices_Electro[0]+this->sizes_EH[0])
    {
        if(    nbr_Y_gl >= this->originIndices_Electro[1] 
            && nbr_Y_gl < this->originIndices_Electro[1]+this->sizes_EH[1]){
            if(    nbr_Z_gl >= this->originIndices_Electro[2] 
                && nbr_Z_gl < this->originIndices_Electro[2]+this->sizes_EH[2]){
                return true;
            }
        }
    }
    return false;
}

/**
 * From the global node numbering, it returns the local node numbering,
 * only if the node is inside this grid !
 */
void GridCreator_NEW::get_local_from_global_electro(
    const size_t nbr_X_gl ,const size_t nbr_Y_gl ,const size_t nbr_Z_gl,
    size_t *nbr_X_loc     ,size_t *nbr_Y_loc     ,size_t *nbr_Z_loc,
    bool *is_ok
)
{
    /// Check that the global node is inside this grid:
    if( !is_global_inside_me(
                    nbr_X_gl,
                    nbr_Y_gl,
                    nbr_Z_gl))
        {
            DISPLAY_WARNING(
                "You have requested a global node which is not in this grid."
            );
            *is_ok = false;
            return;
        }

    /**
     * We must be carefull about the nodes for send/recv operations in MPI comm;
     * we add a '+1' because the first column/row/slice is for send/recv.
     */

    *nbr_X_loc = nbr_X_gl - this->originIndices_Electro[0] + 1;
    *nbr_Y_loc = nbr_Y_gl - this->originIndices_Electro[1] + 1;
    *nbr_Z_loc = nbr_Z_gl - this->originIndices_Electro[2] + 1;

    *is_ok = true;
}