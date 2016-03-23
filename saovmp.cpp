/*
	saovmp.cpp: Simulator for "Sharing-Aware Online VM Packing in Heterogeneous Environments".
*/

#include<algorithm>
#include<iostream>
#include<fstream>
#include<random>

#include "HPP/saovmp_consts.hpp"
#include "HPP/saovmp_model.hpp"
#include "HPP/vms_model.hpp"
#include "HPP/srv_model.hpp"

int main(int argc, char *argv[])
{	
	std::vector<vms_model> V;	
	std::vector<srv_model> S;

	saovmp_model	*execute_model = new saovmp_model;
	vms_model	*execute_vms   = new vms_model;

	execute_model->initPrototype(V, S, argv[1], argv[2]);		
	execute_vms->requestOperatingSystem(V);	
	execute_vms->requestApplications(V);	
	execute_model->prepareInputs(V,S);
	
	for(int algo_id = saovmp_model::nfs; algo_id != saovmp_model::end_strats; algo_id++)
	{
		switch(algo_id)
		{
			case saovmp_model::nfs	:  	execute_model->nextFitSharing();
								break;
			case saovmp_model::ffs	:  	execute_model->firstFitSharing();
								break;
			case saovmp_model::bfs	:  	execute_model->bestFitSharing();
								break;						
			case saovmp_model::wfs	:  	execute_model->worstFitSharing();
								break;						
			case saovmp_model::rfs	:  	execute_model->randomFitSharing();
								break;						
			case saovmp_model::nf	:  	execute_model->nextFit();
								break;
			case saovmp_model::ff	:  	execute_model->firstFit();
								break;
			case saovmp_model::bf	:  	execute_model->bestFit();
								break;						
			case saovmp_model::wf	:  	execute_model->worstFit();
								break;						
			case saovmp_model::rf	:  	execute_model->randomFit();
								break;						
			default					:	
								break;
		}
	}	
	execute_model->generateResults(argv[1], argv[2]);
	delete execute_model;
	delete execute_vms;	
	return 0;
}				