#include "ElectromagneticSource.h"
#include "GridCreator.h"

#include <iostream>
#include <cstdio>
#include <cstdlib>


#ifndef M_PI
  #define M_PI 3.14
#endif 
// Set the number of sources:
void ElectromagneticSource::set_number_of_sources(const unsigned int nbrSources){
	if(!this->number_of_sources.get_alreadySet()){
		this->number_of_sources = nbrSources;

		this->lengthsAlreadySet.reserve(this->number_of_sources.get());
		this->centersAlreadySet.reserve(this->number_of_sources.get());
		this->nodesInsideAlreadySet.reserve(this->number_of_sources.get());

		this->airgap.reserve(this->number_of_sources.get());

		this->lengthX.reserve(this->number_of_sources.get());
		this->lengthY.reserve(this->number_of_sources.get());
		this->lengthZ.reserve(this->number_of_sources.get());
		this->centerX.reserve(this->number_of_sources.get());
		this->centerY.reserve(this->number_of_sources.get());
		this->centerZ.reserve(this->number_of_sources.get());

		this->nbrNodeCorner1_Airgap_X.reserve(this->number_of_sources.get());
		this->nbrNodeCorner1_Airgap_Y.reserve(this->number_of_sources.get());
		this->nbrNodeCorner1_Airgap_Z.reserve(this->number_of_sources.get());

		this->nodesInsideAlong_Airgap_X.reserve(this->number_of_sources.get());
		this->nodesInsideAlong_Airgap_Y.reserve(this->number_of_sources.get());
		this->nodesInsideAlong_Airgap_Z.reserve(this->number_of_sources.get());

		this->nbrNodeCorner1_X.reserve(this->number_of_sources.get());
		this->nbrNodeCorner1_Y.reserve(this->number_of_sources.get());
		this->nbrNodeCorner1_Z.reserve(this->number_of_sources.get());

		this->nodesInsideAlong_X.reserve(this->number_of_sources.get());
		this->nodesInsideAlong_Y.reserve(this->number_of_sources.get());
		this->nodesInsideAlong_Z.reserve(this->number_of_sources.get());
	}else{
		printf("ElectromagneticSource::set_number_of_sources::ERROR\n");
		printf("\tThe property of the field 'number_of_sources' was already set.");
		printf("\n\tAborting (at file %s at line %d)\n\n",__FILE__,__LINE__);
	}
}

void ElectromagneticSource::setLengthAlongOneDir(
									const unsigned int direction,
									vector<double> values){
	if(values.size() != this->number_of_sources.get()){
		printf("You should give as many lengths in the specified direction");
		printf(" as there are sources. You gave %ld",values.size());
		printf(" values for %d sources. Aborting.\n",
										this->number_of_sources.get());
		std::abort();
	}

	if(direction == 0){
		for(unsigned int I = 0 ; I < values.size() ; I ++){
			this->lengthX[I] = values[I];
			this->lengthsAlreadySet[I] = true;
		}
	}else if(direction == 1){
		for(unsigned int I = 0 ; I < values.size() ; I ++){
			this->lengthY[I] = values[I];
			this->lengthsAlreadySet[I] = true;
		}
	}else if(direction == 2){
		for(unsigned int I = 0 ; I < values.size() ; I ++){
			this->lengthZ[I] = values[I];
			this->lengthsAlreadySet[I] = true;
		}
	}else{
		printf("Direction should be between 0 and 2. Aborting.\n");
		std::abort();
	}
}

void ElectromagneticSource::setAllFrequencies(vector<double> freqs){
	if(freqs.size() != this->number_of_sources.get()){
		printf("You should give as many frequencies");
		printf(" as there are sources. You gave %ld",freqs.size());
		printf(" values for %d sources. Aborting.\n",
										this->number_of_sources.get());
		std::abort();
	}
	this->frequency = freqs;
}

// Set centers along one direction:
void ElectromagneticSource::setCenterAlongOneDir(
								const unsigned int direction,
								vector<double> values){
	if(values.size() != this->number_of_sources.get()){
		printf("You should give as many centers in the specified direction");
		printf(" as there are sources. You gave %ld",values.size());
		printf(" values for %d sources. Aborting.\n",
										this->number_of_sources.get());
		std::abort();
	}

	if(direction == 0){
		for(unsigned int I = 0 ; I < values.size() ; I ++){
			this->centerX[I] = values[I];
			this->centersAlreadySet[I] = true;
		}
	}else if(direction == 1){
		for(unsigned int I = 0 ; I < values.size() ; I ++){
			this->centerY[I] = values[I];
			this->centersAlreadySet[I] = true;
		}
	}else if(direction == 2){
		for(unsigned int I = 0 ; I < values.size() ; I ++){
			this->centerZ[I] = values[I];
			this->centersAlreadySet[I] = true;
		}
	}else{
		printf("ElectromagneticSource::setCenterAlongOneDir\n");
		printf("Direction should be between 0 and 2. Aborting.\n");
		std::abort();
	}
}

void ElectromagneticSource::setLengths(const double L_X,
										const double L_Y, 
										const double L_Z,
										const unsigned int i){
	// i represents the desired source, from 0 to number_of_source.
	if(this->number_of_sources.get_alreadySet() && 
			this->lengthsAlreadySet[i] != true){
		this->lengthX[i] = L_X;
		this->lengthY[i] = L_Y;
		this->lengthZ[i] = L_Z;
		this->lengthsAlreadySet[i] = true;
	}
}

void ElectromagneticSource::setCenter (const double C_X,
																			 const double C_Y, 
																			 const double C_Z,
																			 const unsigned int i){
	// i represents the desired source, from 0 to number_of_source.
	if(this->number_of_sources.get_alreadySet() &&
			this->centersAlreadySet[i] != true){
		this->centerX[i] = C_X;
		this->centerY[i] = C_Y;
		this->centerZ[i] = C_Z;
		this->centersAlreadySet[i] = true;
	}
}

void ElectromagneticSource::computeNodesInsideSource(const double L_dom_X,
													 const double L_dom_Y,
													 const double L_dom_Z,
													 const double deltaX,
													 const double deltaY,
													 const double deltaZ,
													 const unsigned int i){
	// Number of the node of corner 1
	if(this->number_of_sources.get_alreadySet() == false ||
			this->centersAlreadySet[i] != true ||
			this->lengthsAlreadySet[i] != true){
				if(this->number_of_sources.get_alreadySet() == false){
					printf("this->number_of_sources.get_alreadySet() == false\n");
				}
		printf("ElectromagneticSource::computeNodesInsideSource::ERROR\n");
		printf("The number of sources hasn't been set, aborting.\n");
		std::abort();
	}
	this->nbrNodeCorner1_X[i] = (size_t) (this->centerX[i] - this->lengthX[i] / 2) / deltaX +1;
	this->nbrNodeCorner1_Y[i] = (size_t) (this->centerY[i] - this->lengthY[i] / 2) / deltaY +1;
	this->nbrNodeCorner1_Z[i] = (size_t) (this->centerZ[i] - this->lengthZ[i] / 2) / deltaZ +1;
	
	this->nodesInsideAlong_X[i] = (size_t) this->lengthX[i] / deltaX + 1;
	this->nodesInsideAlong_Y[i] = (size_t) this->lengthY[i] / deltaY + 1;
	this->nodesInsideAlong_Z[i] = (size_t) this->lengthZ[i] / deltaZ + 1;

	this->nodesInsideAlreadySet[i] = true;

	this->nbrNodeCorner1_Airgap_X[i] = (size_t) (this->centerX[i] - this->lengthX[i] / 2) / deltaX +1;
	this->nbrNodeCorner1_Airgap_Y[i] = (size_t) (this->centerY[i] - this->lengthY[i] / 2) / deltaY +1;
	this->nbrNodeCorner1_Airgap_Z[i] = (size_t) (this->centerZ[i] - this->airgap[i].get() / 2) / deltaZ +1;

	this->nodesInsideAlong_Airgap_X[i] = (size_t) this->lengthX[i] / deltaX + 1;
	this->nodesInsideAlong_Airgap_Y[i] = (size_t) this->lengthY[i] / deltaY + 1;
	this->nodesInsideAlong_Airgap_Z[i] = (size_t) this->airgap[i].get() / deltaZ + 1;



	printf("> ElectromagneticSource::computeNodesInsideSource::SOURCE[%d]\n",i);
	printf("\tnbrNodeCorner1_X=%ld\n",this->nbrNodeCorner1_X[i]);
	printf("\tnbrNodeCorner1_Y=%ld\n",this->nbrNodeCorner1_Y[i]);
	printf("\tnbrNodeCorner1_Z=%ld\n",this->nbrNodeCorner1_Z[i]);
	printf("\tthis->nodesInsideAlong_X=%ld\n",this->nodesInsideAlong_X[i]);
	printf("\tthis->nodesInsideAlong_Y=%ld\n",this->nodesInsideAlong_Y[i]);
	printf("\tthis->nodesInsideAlong_Z=%ld\n",this->nodesInsideAlong_Z[i]);
	printf("\tnbrNodeCorner1_Airgap_X=%ld\n",this->nbrNodeCorner1_Airgap_X[i]);
	printf("\tnbrNodeCorner1_Airgap_Y=%ld\n",this->nbrNodeCorner1_Airgap_Y[i]);
	printf("\tnbrNodeCorner1_Airgap_Z=%ld\n",this->nbrNodeCorner1_Airgap_Z[i]);
	printf("\tthis->nodesInsideAlong_Airgap_X=%ld\n",this->nodesInsideAlong_Airgap_X[i]);
	printf("\tthis->nodesInsideAlong_Airgap_Y=%ld\n",this->nodesInsideAlong_Airgap_Y[i]);
	printf("\tthis->nodesInsideAlong_Airgap_Z=%ld\n",this->nodesInsideAlong_Airgap_Z[i]);

}

bool ElectromagneticSource::isInsideSource(const size_t i_global, 
											const size_t j_global, 
											const size_t k_global,
											const unsigned int i){

	// i represents the desired source.
	if(this->nodesInsideAlreadySet[i] != true){
		printf("ElectromagneticSource::isInsideSource::ERROR.\n");
		printf("Please call ElectromagneticSource::computeNodesInsideSource before.\n");
		printf("Aborting.\n\n");
		std::abort();
	}
	if( ( i_global >= this->nbrNodeCorner1_X[i] &&
		  i_global <= (this->nbrNodeCorner1_X[i] + this->nodesInsideAlong_X[i]))
		&&
		( j_global >= this->nbrNodeCorner1_Y[i] &&
		  j_global <= (this->nbrNodeCorner1_Y[i] + this->nodesInsideAlong_Y[i]))
		&&
		( k_global >= this->nbrNodeCorner1_Z[i] &&
		  k_global <= (this->nbrNodeCorner1_Z[i] + this->nodesInsideAlong_Z[i]))){
		//printf("ElectromagneticSource::isInsideSource::TRUE\n");
		return true;
	}
	// By default, return false:
	//printf("ElectromagneticSource::isInsideSource::FALSE\n");
	return false;
}


void ElectromagneticSource::set_airGaps(const std::vector<double> airGaps){
	if(!this->number_of_sources.get_alreadySet()){
		printf("ElectromagneticSource::computeNodesInsideSource::ERROR\n");
		printf("The number of sources hasn't been set, aborting.\n");
		std::abort();
	}

	for(unsigned int I = 0 ; I < this->number_of_sources.get() ; I ++ ){
		this->airgap[I] = airGaps[I];
	}
}

/* --------------------------------------------------------------------------------------------------------------------- */
/* Here, i,j,k are local indices */
void ElectromagneticSource::computeSourceValue(GridCreator &mesh,
				 const size_t i_global, const size_t j_global,
				 const size_t k_global,double tCurrent,char CHAMP)
{
	//double AirGap = 1;
	
	//printf("ElectromagneticSource::computeSourceValue::(%ld,%ld,%ld)\n",i_global,j_global,k_global);

	unsigned int ID_Source = this->DetermineInWhichSourceWeAre(i_global, j_global, k_global);
	//printf("ElectromagneticSource::computeSourceValue::ID_Source=%d.\n",ID_Source);
	if(ID_Source == -1){
		abort();
	}

	/* FIND IN WHICH SOURCE WE ARE */

	/* Size d'une antenne du dipole */
	//double LengthDipoleX = this->lengthX[ID_Source];//mesh.elec_source.LengthX(ID_Source);
	//double LengthDipoleY = this->lengthY[ID_Source];//mesh.elec_source.LengthY(ID_Source);
	//double LengthDipoleZ = this->lengthZ[ID_Source];//(mesh.elec_source.LengthZ(ID_Source) - AirGap)/2;

	/* 	Here, GlobalIndices will contain the global indices corresponding to the local indices i,j,k, which are in the source */
	/*int GlobalIndices[3];
	GlobalIndices[0] = mesh.Transformation(i, j, k, mesh.myrank, mesh.numberofprocess);
	GlobalIndices[1] = mesh.Transformation(i, j, k, mesh.myrank, mesh.numberofprocess);
	GlobalIndices[2] = mesh.Transformation(i, j, k, mesh.myrank, mesh.numberofprocess);
	*/
	/* We know that for a dipole antenna E_x, E_y, H_x, H_y and H_z are all equal to 0, whereas E_z is different if we are in the air gap or not */
	if(CHAMP == 'H'){
		//mesh.nodesMagn(i_global,j_global,k_global).field[0] = 0.0;
		//mesh.nodesMagn(i_global,j_global,k_global).field[1] = 0.0;
		//mesh.nodesMagn(i_global,j_global,k_global).field[2] = 0.0;
	}else if(CHAMP == 'E'){
		mesh.nodesElec(i_global,j_global,k_global).field[0] = 0.0;
		mesh.nodesElec(i_global,j_global,k_global).field[1] = 0.0;

		//if(isInsideAirGap(i_global,j_global,k_global,ID_Source) == true){
			mesh.nodesElec(i_global,j_global,k_global).field[2] = sin(2*M_PI*mesh.input_parser.source.frequency[ID_Source]*tCurrent);
			//cout << "INSIDE AIRGAP" << endl;
			//printf("E_Z(%.10f) = %.7f\n",tCurrent,sin(2*M_PI*mesh.input_parser.source.frequency[ID_Source]*tCurrent)*1000000000000);
		//}
		/* Ask Romin if the function returns the indices or the physical coordinates */
		/*double CenterAntenna[3];
		this->getCenter(ID_Source,CenterAntenna);

		/* If we are in the antenna */
		/*if(CenterAntenna[0]-(LengthDipoleX/2)/mesh.deltaX <= GlobalIndices[0]  
					&& GlobalIndices[0] <= CenterAntenna[0]-(LengthDipoleX/2)/mesh.deltaX )
		{
			if(CenterAntenna[1]-(LengthDipoleY/2)/mesh.deltaY <= GlobalIndices[1] && GlobalIndices[1] <= CenterAntenna[1]-(LengthDipoleY/2)/mesh.deltaY)
			{
				if(CenterAntenna[2]-(LengthDipoleZ/2)/mesh.deltaZ <= GlobalIndices[2] && GlobalIndices[2] <= CenterAntenna[2]-(LengthDipoleY/2)/mesh.deltaZ)
					mesh.nodesElec(i_global,j_global,k_global).field[2] = sin(2*M_PI*mesh.elec_source.frequency[ID_Source]*tCurrent);
			}
		}
		else
		{
			mesh.nodesElec(i_global,j_global,k_global).field[2] = 0.0;	
		}*/
	}else{
		printf("ElectromagneticSource::computeSourceValue:: ERROR\n");
		printf("Should be 'E' or 'H' but has '%c'.\n",CHAMP);
		printf("Abort.\n\n");
		abort();
	}

	
}

bool ElectromagneticSource::isInsideSource(const size_t i_global,
											const size_t j_global,
											const size_t k_global){
	for(unsigned int I = 0 ; I < this->number_of_sources.get() ; I ++){
		if(this->isInsideSource(i_global,j_global,k_global,I)){
			return true;
		}
	}
	// By default we return -1, meaning we are not in a source.
	return false;
}

bool ElectromagneticSource::isInsideAirGap(const size_t i_global,
											const size_t j_global,
											const size_t k_global){
	for(unsigned int I = 0 ; I < this->number_of_sources.get() ; I ++){
		if(this->isInsideAirGap(i_global,j_global,k_global,I)){
			return true;
		}
	}
	// By default we return -1, meaning we are not in a source.
	return false;
}

bool ElectromagneticSource::isInsideAirGap(const size_t i_global, 
											const size_t j_global, 
											const size_t k_global,
											const unsigned int i){

	// i represents the desired source.
	if(this->nodesInsideAlreadySet[i] != true){
		printf("ElectromagneticSource::isInsideSource::ERROR.\n");
		printf("Please call ElectromagneticSource::computeNodesInsideSource before.\n");
		printf("Aborting.\n\n");
		std::abort();
	}
	if( ( i_global >= this->nbrNodeCorner1_Airgap_X[i] &&
		  i_global <= (this->nbrNodeCorner1_Airgap_X[i] + this->nodesInsideAlong_Airgap_X[i]))
		&&
		( j_global >= this->nbrNodeCorner1_Airgap_Y[i] &&
		  j_global <= (this->nbrNodeCorner1_Airgap_Y[i] + this->nodesInsideAlong_Airgap_Y[i]))
		&&
		( k_global >= this->nbrNodeCorner1_Airgap_Z[i] &&
		  k_global <= (this->nbrNodeCorner1_Airgap_Z[i] + this->nodesInsideAlong_Airgap_Z[i]))){
		//printf("ElectromagneticSource::isInsideAirGap::TRUE\n");
		return true;
	}
	// By default, return false:
	//printf("ElectromagneticSource::isInsideAirGap::FALSE\n");
	return false;
}


int ElectromagneticSource::DetermineInWhichSourceWeAre(const size_t i_global,
														const size_t j_global,
														const size_t k_global){
	for(unsigned int I = 0 ; I < this->number_of_sources.get() ; I ++){
		if(this->isInsideSource(i_global,j_global,k_global,I)){
			return I;
		}
	}
	// By default we return -1, meaning we are not in a source.
	return -1;
}