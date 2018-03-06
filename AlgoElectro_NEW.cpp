#include "AlgoElectro_NEW.hpp"

#include <ctime>
#include <new>
#include "omp.h"

double AlgoElectro_NEW::Compute_dt(GridCreator_NEW &mesh){
    // Retrieve the spatial step in each direction:
    double dx = mesh.delta_Electromagn[0];
    double dy = mesh.delta_Electromagn[1];
    double dz = mesh.delta_Electromagn[2];
    // Time step:
    double dt = 0.0;
    // Temporary variable:
    double tmp= 0.0;
    // Iterator:
    unsigned char i=0;                                      

    // Iterate over the number of materials. For each material, compute the required time step.
    // At the end, the smallest time step is chosen.                                          
    for (i = 0 ; i < mesh.materials.numberOfMaterials ; i++ ){

            // Get material:
            string material = mesh.materials.materialName_FromMaterialID[i];
            // Get permeability:
            double mu_material = mesh.materials.getProperty(
                    mesh.input_parser.GetInitTemp_FromMaterialName[material],
                    i,4);    

            // Get permittivity:
            double epsilon_material = mesh.materials.getProperty(
                   mesh.input_parser.GetInitTemp_FromMaterialName[material],
                    i,5);     
            // Compute speed of light:
            double c = 1/(sqrt(mu_material*epsilon_material));
            // Take the smallest time step:
            if( i == 0 ){
                dt = 1 / ( c * sqrt( 1 / ( dx * dx ) + 1 / ( dy * dy ) + 1 / (dz *dz) ) );
            }
            else{
                tmp = 1/(c*sqrt(1/(dx*dx) + 1/(dy*dy) + 1/(dz*dz)));
                if( tmp < dt ){
                    dt = tmp;
                }
            }
    }
    return dt;
}

void AlgoElectro_NEW::update(GridCreator_NEW &grid){

    // Start clock for monitoring CPU time:
    double start_algo_update;
    double end___algo_update;
    grid.profiler.addTimingInputToDictionnary("AlgoElectro_NEW_UPDATE_omp_get_wtime");
    start_algo_update = omp_get_wtime();

    // Retrieve the time step:
    double dt = this->Compute_dt(grid);
    double current_time = 0.0;
    std::cout << "AlgoElectro_NEW :: dt is " << dt << std::endl;

    /*
     *  TEMPERATURE WILL NEVER CHANGE IN THIS ALGORITHM.
     *  INITIALIZE ALL THE COEFFICIENTS NEEDED FOR THE UPDATE EQUATIONS.
     */
    /* ELECTRIC AND MAGNETIC FIELDS - SIZES
    *   Ex of size (M − 1) × N × P
    *   Ey of size M × (N − 1) × P
    *   Ez of size M × N × (P − 1)
    *   Hx of size M × (N − 1) × (P − 1)
    *   Hy of size (M − 1) × N × (P − 1)
    *   Hz of size (M − 1) × (N − 1) × P
    */

    // In the object grid, set the properties mu, eps, magnetic cond. and electric cond. f each node:
    grid.Initialize_Electromagnetic_Properties("AIR_AT_INIT_TEMP");

    /* Set the coefficients for the magnetic update */

    size_t size;
    size_t memory;
    
    size_t M = grid.sizes_EH[0];
    size_t N = grid.sizes_EH[1];
    size_t P = grid.sizes_EH[2];

    // Magnetic field Hx of size M × (N − 1) × (P − 1):
    size = grid.size_Hx[0] * grid.size_Hx[1] * grid.size_Hx[2];
    double *C_hxh   = new double[size];
    double *C_hxe_1 = new double[size];
    double *C_hxe_2 = new double[size];

    memory = (8+8+8) * size;
    grid.profiler.addMemoryUsage("BYTES",memory);

    // Magnetic field Hy of size (M − 1) × N × (P − 1):
    size = grid.size_Hy[0] * grid.size_Hy[1] * grid.size_Hy[2];
    double *C_hyh   = new double[size];
    double *C_hye_1 = new double[size];
    double *C_hye_2 = new double[size];

    memory = (8+8+8) * size;
    grid.profiler.addMemoryUsage("BYTES",memory);

    // Magnetic field Hz of size (M − 1) × (N − 1) × P:
    size = grid.size_Hz[0]*grid.size_Hz[1]*grid.size_Hz[2];
    double *C_hzh   = new double[size];
    double *C_hze_1 = new double[size];
    double *C_hze_2 = new double[size];

    memory = (8+8+8) * size;
    grid.profiler.addMemoryUsage("BYTES",memory);

    // Electric field Ex of size (M − 1) × N × P:
    size = grid.size_Ex[0]*grid.size_Ex[1]*grid.size_Ex[2];
    double *C_exe   = new double[size];
    double *C_exh_1 = new double[size];
    double *C_exh_2 = new double[size];

    memory = (8+8+8) * size;
    grid.profiler.addMemoryUsage("BYTES",memory);

    // Electric field Ey of size M × (N − 1) × P:
    size = grid.size_Ey[0]*grid.size_Ey[1]*grid.size_Ey[2]; 
    double *C_eye   = new double[size];
    double *C_eyh_1 = new double[size];
    double *C_eyh_2 = new double[size];

    memory = (8+8+8) * size;
    grid.profiler.addMemoryUsage("BYTES",memory);

    // Electric field Ez of size M × N × (P − 1):
    size = grid.size_Ez[0]*grid.size_Ez[1]*grid.size_Ez[2];
    double *C_eze   = new double[size];
    double *C_ezh_1 = new double[size];
    double *C_ezh_2 = new double[size];

    memory = (8+8+8) * size;
    grid.profiler.addMemoryUsage("BYTES",memory);

    /* COMPUTING COEFFICIENTS */
    omp_set_num_threads(6);
    #pragma omp parallel
    {
        if(omp_get_num_threads() < 6){
            fprintf(stderr,"AlgoElectro_NEW::UPDATE::ERROR\n");
            fprintf(stderr,"Not enough OMP threads. Needs 6 but has %d.\n",omp_get_num_threads());
            fprintf(stderr,"Aborting in file %s:%d.\n",__FILE__,__LINE__);
            abort();
        }
    }
    #pragma omp barrier

    size_t index;

    #pragma omp parallel num_threads(omp_get_num_threads())
    {
        /* Coefficients for Ex */
        // Ex of size (M − 1) × N × P
        #pragma omp for collapse(3) nowait\
            private(index)
            for(size_t K = 0 ; K < grid.size_Ex[2] ; K ++){
                for(size_t J = 0 ; J < grid.size_Ex[1] ; J ++){
                    for(size_t I = 0 ; I < grid.size_Ex[0] ; I ++){

                        index = I + grid.size_Ex[0] * ( J + grid.size_Ex[1] * K);

                        double COEF_E = grid.E_x_electrical_cond[index] * dt
                            / (2.0 * grid.E_x_eps[index]);

                        // Coefficient C_exe:
                        C_exe[index] = (1-COEF_E) / (1+COEF_E);

                        // Coefficient C_exh_1:
                        C_exh_1[index] = 1 / ( 1 + COEF_E) * dt 
                            / (grid.E_x_eps[index] * grid.delta_Electromagn[1]);

                        // Coefficient C_exh_2:
                        C_exh_2[index] = 1 / ( 1 + COEF_E) * dt 
                            / (grid.E_x_eps[index] * grid.delta_Electromagn[2]);

                    }
                }
            }

        /* Coefficients of Ey */
        // Ey is of size M × (N − 1) × P.
        #pragma omp for collapse(3) nowait\
            private(index)
            for(size_t K = 0 ; K < grid.size_Ey[2] ; K ++){
                for(size_t J = 0 ; J < grid.size_Ey[1] ; J ++){
                    for(size_t I = 0 ; I < grid.size_Ey[0] ; I ++){

                        index = I + grid.size_Ey[0] * ( J + grid.size_Ey[1] * K);

                        double COEF_E = grid.E_y_electrical_cond[index] * dt
                            / (2.0 * grid.E_y_eps[index]);

                        // Coefficient C_eye:
                        C_eye[index] = (1-COEF_E) / (1+COEF_E);

                        // Coefficient C_eyh_1:
                        C_eyh_1[index] = 1 / ( 1 + COEF_E) * dt 
                            / (grid.E_y_eps[index] * grid.delta_Electromagn[2]);

                        // Coefficient C_eyh_2:
                        C_eyh_2[index] = 1 / ( 1 + COEF_E) * dt 
                            / (grid.E_y_eps[index] * grid.delta_Electromagn[0]);
                    }
                }
            }

        /* Coefficients of Ez */
        // Ez is of size  M × N × (P − 1)
        #pragma omp for collapse(3) nowait\
            private(index)
            for(size_t K = 0 ; K < grid.size_Ez[2] ; K ++){
                for(size_t J = 0 ; J < grid.size_Ez[1] ; J ++){
                    for(size_t I = 0 ; I < grid.size_Ez[0] ; I ++){

                        index = I + grid.size_Ez[0] * ( J + grid.size_Ez[1] * K);

                        double COEF_E = grid.E_z_electrical_cond[index] * dt
                            / (2.0 * grid.E_z_eps[index]);

                        // Coefficient C_eze:
                        C_eze[index] = (1-COEF_E) / (1+COEF_E);

                        // Coefficient C_ezh_1:
                        C_ezh_1[index] = 1 / ( 1 + COEF_E) * dt 
                            / (grid.E_z_eps[index] * grid.delta_Electromagn[0]);

                        // Coefficient C_ezh_2:
                        C_ezh_2[index] = 1 / ( 1 + COEF_E) * dt 
                            / (grid.E_z_eps[index] * grid.delta_Electromagn[1]);
                    }
                }
            }

        /* Coefficients of Hx */
        // Hx is of size M × (N − 1) × (P − 1):
        #pragma omp for collapse(3) nowait\
            private(index)
            for(size_t K = 0 ; K < grid.size_Hx[2] ; K ++){
                for(size_t J = 0 ; J < grid.size_Hx[1] ; J ++){
                    for(size_t I = 0 ; I < grid.size_Hx[0] ; I ++){

                        index = I + grid.size_Hx[0] * ( J + grid.size_Hx[1] * K);

                        double COEF_H = grid.H_x_magnetic_cond[index] * dt
                            / (2.0 * grid.H_x_mu[index]);

                        // Coefficient C_hxh:
                        C_hxh[index] = (1-COEF_H) / (1+COEF_H);

                        // Coefficient C_hxe_1:
                        C_hxe_1[index] = 1 / ( 1 + COEF_H) * dt 
                            / (grid.H_x_mu[index] * grid.delta_Electromagn[2]);

                        // Coefficient C_hxe_2:
                        C_hxe_2[index] = 1 / ( 1 + COEF_H) * dt 
                            / (grid.H_x_mu[index] * grid.delta_Electromagn[1]);
                    }
                }
            }

        /* Coefficients of Hy */
        // Hy is of size (M − 1) × N × (P − 1)
        #pragma omp for collapse(3) nowait\
            private(index)
            for(size_t K = 0 ; K < grid.size_Hy[2] ; K ++){
                for(size_t J = 0 ; J < grid.size_Hy[1] ; J ++){
                    for(size_t I = 0 ; I < grid.size_Hy[0] ; I ++){

                        index = I + grid.size_Hy[0] * ( J + grid.size_Hy[1] * K);

                        double COEF_H = grid.H_y_magnetic_cond[index] * dt
                            / (2.0 * grid.H_y_mu[index]);

                        // Coefficient C_hxh:
                        C_hyh[index] = (1-COEF_H) / (1+COEF_H);

                        // Coefficient C_hxe_1:
                        C_hye_1[index] = 1 / ( 1 + COEF_H) * dt 
                            / (grid.H_y_mu[index] * grid.delta_Electromagn[0]);

                        // Coefficient C_hxe_2:
                        C_hye_2[index] = 1 / ( 1 + COEF_H) * dt 
                            / (grid.H_y_mu[index] * grid.delta_Electromagn[2]);
                    }
                }
            }

        /* Coefficients of Hz */
        // Hz is of size (M − 1) × (N − 1) × P
        #pragma omp for collapse(3) nowait\
            private(index)
            for(size_t K = 0 ; K < grid.size_Hz[2] ; K ++){
                for(size_t J = 0 ; J < grid.size_Hz[1] ; J ++){
                    for(size_t I = 0 ; I < grid.size_Hz[0] ; I ++){

                        index = I + grid.size_Hz[0] * ( J + grid.size_Hz[1] * K);

                        double COEF_H = grid.H_z_magnetic_cond[index] * dt
                            / (2.0 * grid.H_z_mu[index]);

                        // Coefficient C_hxh:
                        C_hzh[index] = (1-COEF_H) / (1+COEF_H);

                        // Coefficient C_hxe_1:
                        C_hze_1[index] = 1 / ( 1 + COEF_H) * dt 
                            / (grid.H_z_mu[index] * grid.delta_Electromagn[1]);

                        // Coefficient C_hxe_2:
                        C_hze_2[index] = 1 / ( 1 + COEF_H) * dt 
                            / (grid.H_z_mu[index] * grid.delta_Electromagn[0]);
                    }
                }
            }

    }
    // END OF #pragma omp parallel num_threads(6)

    ///////////////////////////////////////////////
    // UPDATE WHILE LOOP - PARALLELIZED WITH     //
    // OPENMP THREADS    - MINIMUM 6 OPENMP      //
    // THREADS ARE REQUIRED FOR MPI COMMUICATION //
    // TO WORK.                                  //
    ///////////////////////////////////////////////

    size_t index_1Plus;
    size_t index_1Moins;
    size_t index_2Plus;
    size_t index_2Moins;
    size_t size_x;
    size_t size_y;
    size_t size_x_1;
    size_t size_y_1;
    size_t size_x_2;
    size_t size_y_2;

    // Temporary pointers, to avoid doing grid.sthg !
    double *H_x_tmp = grid.H_x;
    double *H_y_tmp = grid.H_y;
    double *H_z_tmp = grid.H_z;
    double *E_x_tmp = grid.E_x;
    double *E_y_tmp = grid.E_y;
    double *E_z_tmp = grid.E_z;

    #pragma omp parallel num_threads(omp_get_num_threads()) default(none)\
        shared(grid,dt,current_time)\
        shared(H_x_tmp,H_y_tmp,H_z_tmp)\
        shared(E_x_tmp,E_y_tmp,E_z_tmp)\
        shared(C_hxh,C_hxe_1,C_hxe_2)\
        shared(C_hyh,C_hye_1,C_hye_2)\
        shared(size_x,size_y)\
        shared(size_x_1,size_y_1)\
        shared(size_x_2,size_y_2)
    {

        while(current_time < grid.input_parser.get_stopTime()){
            // Updating the magnetic field Hx.
            // Don't update neighboors ! Start at 1. Go to size-1.

            size_x = grid.size_Hx[0];
            size_y = grid.size_Hx[1];

            size_x_1 = grid.size_Ey[0];
            size_y_1 = grid.size_Ey[1];

            size_x_2 = grid.size_Ez[0];
            size_y_2 = grid.size_Ez[1];

            #pragma omp for collapse(3) nowait\
                private(index)\
                private(index_1Plus,index_1Moins)\
                private(index_2Plus,index_2Moins)
            for (size_t K = 1; K < grid.size_Hx[2]-1 ; K++){
                for(size_t J = 1 ; J < grid.size_Hx[1]-1 ; J ++){
                    for(size_t I = 1 ; I < grid.size_Hx[0]-1 ; I++){

                        // Hx(mm, nn, pp):
                        index        = I + size_x   * ( J     + size_y   * K);
                        // Ey(mm, nn, pp + 1):
                        index_1Plus  = I + size_x_1 * ( J     + size_y_1 * (K+1));
                        // Ey(mm, nn, pp):
                        index_1Moins = I + size_x_1 * ( J     + size_y_1 * K);
                        // Ez(mm, nn + 1, pp):
                        index_2Plus  = I + size_x_2 * ( (J+1) + size_y_2 * K);
                        // Ez(mm, nn, pp):
                        index_2Moins = I + size_x_2 * ( J     + size_y_2 * K);

                        H_x_tmp[index] = C_hxh[index] * H_x_tmp[index]
                                + C_hxe_1[index] * (E_y_tmp[index_1Plus] - E_y_tmp[index_1Moins])
                                - C_hxe_2[index] * (E_z_tmp[index_2Plus] - E_z_tmp[index_2Moins]);

                    }
                }
            }

            // Updating the magnetic field Hy.
            // Don't update neighboors ! Start at 1. Go to size-1.

            size_x = grid.size_Hy[0];
            size_y = grid.size_Hy[1];

            size_x_1 = grid.size_Ez[0];
            size_y_1 = grid.size_Ez[1];

            size_x_2 = grid.size_Ex[0];
            size_y_2 = grid.size_Ex[1];

            #pragma omp for collapse(3) nowait\
                private(index)\
                private(index_1Plus,index_1Moins)\
                private(index_2Plus,index_2Moins)
            for(size_t K = 1 ; K < grid.size_Hy[2]-1 ; K ++){
                for(size_t J = 1 ; J < grid.size_Hy[1]-1 ; J ++){
                    for(size_t I = 1 ; I < grid.size_Hy[0]-1 ; I ++){

                        index = 0;
                        // Ez(mm + 1, nn, pp):
                        index_1Plus = 0;
                        // Ez(mm, nn, pp)
                        index_1Moins = 0;
                        // Ex(mm, nn, pp + 1):
                        index_2Plus = 0;
                        // Ex(mm, nn, pp):
                        index_2Moins = 0;

                        H_y_tmp[index] = C_hyh[index] * H_y_tmp[index]
                                + C_hye_1[index] * (E_z_tmp[index_1Plus] - E_z_tmp[index_1Moins])
                                - C_hye_2[index] * (E_x_tmp[index_2Plus] - E_x_tmp[index_2Moins]);

                    }
                }
            }

            #pragma omp master
            {
                current_time += 1000000*dt;
            }
        } /* END OF WHILE */
    }/* END OF PARALLEL REGION */


    /* FREE MEMORY */
    
    // Free H_x coefficients:
    size = grid.size_Hx[0]*grid.size_Hx[1]*grid.size_Hx[2];
    delete[] C_hxh;
    delete[] C_hxe_1;
    delete[] C_hxe_2;

    memory = (8+8+8) * size;
    grid.profiler.removeMemoryUsage("BYTES",memory,"Algo_Free_H_x_coeffs");


    // Free H_y coefficents:
    size = grid.size_Hy[0]*grid.size_Hy[1]*grid.size_Hy[2];
    delete[] C_hyh;
    delete[] C_hye_1;
    delete[] C_hye_2;

    memory = (8+8+8) * size;
    grid.profiler.removeMemoryUsage("BYTES",memory,"Algo_Free_H_y_coeffs");

    // Free H_z coefficients:
    size = grid.size_Hz[0]*grid.size_Hz[1]*grid.size_Hz[2];
    delete[] C_hzh;
    delete[] C_hze_1;
    delete[] C_hze_2;

    memory = (8+8+8) * size;
    grid.profiler.removeMemoryUsage("BYTES",memory,"Algo_Free_H_z_coeffs");

    // Free E_x coefficients:
    size = grid.size_Ex[0]*grid.size_Ex[1]*grid.size_Ex[2];
    delete[] C_exe;
    delete[] C_exh_1;
    delete[] C_exh_2;

    memory = (8+8+8) * size;
    grid.profiler.removeMemoryUsage("BYTES",memory,"Algo_Free_E_x_coeffs");

    // Free E_y coefficients:
    size = grid.size_Ey[0]*grid.size_Ey[1]*grid.size_Ey[2];
    delete[] C_eye;
    delete[] C_eyh_1;
    delete[] C_eyh_2;

    memory = (8+8+8) * size;
    grid.profiler.removeMemoryUsage("BYTES",memory,"Algo_Free_E_y_coeffs");

    // Free E_z coefficients:
    size = grid.size_Ez[0]*grid.size_Ez[1]*grid.size_Ez[2];
    delete[] C_eze;
    delete[] C_ezh_1;
    delete[] C_ezh_2;

    memory = (8+8+8) * size;
    grid.profiler.removeMemoryUsage("BYTES",memory,"Algo_Free_E_z_coeffs");

    // End clock for monitoring CPU time
    end___algo_update = omp_get_wtime();
    double elapsedTimeSec = end___algo_update - start_algo_update;
    grid.profiler.incrementTimingInput("AlgoElectro_NEW_UPDATE_omp_get_wtime",elapsedTimeSec);
    std::cout << "AlgoElectro_NEW_UPDATE => Time: " << elapsedTimeSec << " s" << std::endl;
}