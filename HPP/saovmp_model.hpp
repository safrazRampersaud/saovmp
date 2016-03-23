#ifndef SAOVMP_MODEL_HPP
#define	SAOVMP_MODEL_HPP

#include "srv_model.hpp"
#include "vms_model.hpp"

std::random_device rd;
std::mt19937 generator(rd());					

class saovmp_model
{
	public:				
		saovmp_model();
		~saovmp_model();
		
		void initPrototype(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S), std::string vmInputFile, std::string serverInputFile);
		void prepareInputs(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S));		
		void generateResults(std::string inputVMFile, std::string inputServerFile);
		
		//	Sharing-Aware Algorithms
		void nextFitSharing();		//	nfs
		void firstFitSharing();		//	ffs
		void bestFitSharing();		//	bfs
		void worstFitSharing();		//	wfs
		void randomFitSharing();	//	rfs
		
		//	Sharing-Oblivious Algorithms
		void nextFit();				//	nf
		void firstFit();			//	ff
		void bestFit();				//	bf
		void worstFit();			//	wf
		void randomFit();			//	rf

		enum algorithms {nfs, ffs, bfs, wfs, rfs, nf, ff, bf, wf, rf, end_strats};
				
	private:
		std::vector<vms_model> saovmp_V;
		std::vector<srv_model> saovmp_S;	
		
		int 	idx;
		int 	comparator[saovmp_consts::NUM_STRATS];		
		int	activeServersByType[saovmp_consts::SERVER_TYPES][saovmp_consts::NUM_STRATS];
		bool 	allocated;
		double	scarcity_metric;
		double	initServerMem[saovmp_consts::SERVER_TYPES];	
		double 	utilizedMemPerServer[saovmp_consts::SERVER_TYPES][saovmp_consts::NUM_STRATS];
		
		void sharing_per_server(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S));		
		void non_sharing_per_server(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S));			
		void process_allocation(std::vector<vms_model> &(process_V), std::vector<srv_model> &(process_S), algorithms strat);
		void server_resource_scarcity(std::vector<vms_model> &(calc_scarcity_V), std::vector<srv_model> &(calc_scarcity_S), algorithms strat);
};

saovmp_model::saovmp_model() : scarcity_metric(0.00), idx(-1), allocated(false)
{
	comparator[saovmp_consts::NUM_STRATS] = {};		
	activeServersByType[saovmp_consts::SERVER_TYPES][saovmp_consts::NUM_STRATS] = {};
	initServerMem[saovmp_consts::SERVER_TYPES] = {};	
	utilizedMemPerServer[saovmp_consts::SERVER_TYPES][saovmp_consts::NUM_STRATS] = {};
}

saovmp_model::~saovmp_model() {}

/*
Random-Fit-Sharing: 
-------------------
Assign each VM to the first randomly selected server that fits its 
request requirements in consideration of page-sharing.
*/
void saovmp_model::randomFitSharing()
{	
	std::vector<vms_model> rfs_vms;
	std::vector<srv_model> rfs_srv;
		
	for(auto vm : saovmp_V)
		rfs_vms.push_back(vm);
	for(auto pm : saovmp_S)
		rfs_srv.push_back(pm);

	for(int j = 0; j < rfs_vms.size(); j++)
	{	
		allocated = false;
		sharing_per_server(rfs_vms, rfs_srv);			
		while(allocated == false)
		{	
			std::uniform_int_distribution<> distrbution(0, rfs_srv.size());
			int select = distrbution(generator);
			if(	((rfs_srv[select].cpuCap - rfs_vms[j].cpuReq) >= 0.00) && 
				((rfs_srv[select].memCap - rfs_vms[j].memReq + rfs_vms[j].sharing[select] >= 0.00)))
			{
				for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
					saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
					if(rfs_vms[j].v_PI[pii] == true)
						rfs_srv[select].PI[pii] = true;
					
				rfs_srv[select].cpuCap = rfs_srv[select].cpuCap - rfs_vms[j].cpuReq;
				rfs_srv[select].memCap = rfs_srv[select].memCap - rfs_vms[j].memReq + rfs_vms[j].sharing[select];
				rfs_srv[select].active = 1;
				rfs_srv[select].reduct = true;
				rfs_vms[j].suspend = true;
				allocated = true;
			}
		}
	}
	
	process_allocation(rfs_vms, rfs_srv, rfs);
}

/*
Worst-Fit-Sharing: 
------------------
Assign each VM to the server which results in the most remaining utilizedMemPerServer scarcity
per resource in consideration of page-sharing.
*/
void saovmp_model::worstFitSharing()
{		
	std::vector<vms_model> wfs_vms;
	std::vector<srv_model> wfs_srv;
		
	for(auto vm : saovmp_V)
		wfs_vms.push_back(vm);
	for(auto pm : saovmp_S)
		wfs_srv.push_back(pm);
	
	idx = -1;	
	scarcity_metric = std::numeric_limits<double>::max();
	for(int j = 0; j < wfs_vms.size(); j++)
	{	
		sharing_per_server(wfs_vms, wfs_srv);		
		server_resource_scarcity(wfs_vms, wfs_srv, wfs);
		for(int k = 0; k < wfs_srv.size(); k++)
		{	
			if(scarcity_metric > wfs_vms[j].scarcity[k] && (wfs_vms[j].scarcity[k] > 0))
			{
				scarcity_metric = wfs_vms[j].scarcity[k];
				idx = k;
			}	
		}
		
		for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
			saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
			if(wfs_vms[j].v_PI[pii] == true)
				wfs_srv[idx].PI[pii] = true;
					
		wfs_srv[idx].cpuCap = wfs_srv[idx].cpuCap - wfs_vms[j].cpuReq;
		wfs_srv[idx].memCap = wfs_srv[idx].memCap - wfs_vms[j].memReq + wfs_vms[j].sharing[idx];
		wfs_srv[idx].active = 1;
		wfs_srv[idx].reduct = true;
		wfs_vms[j].suspend = true;
		idx = -1;
		scarcity_metric = std::numeric_limits<double>::max();
	}
	
	process_allocation(wfs_vms, wfs_srv, wfs);
}

/*
Best-Fit-Sharing: 
------------------
Assign each VM to the server which results in the least remaining utilizedMemPerServer scarcity
per resource in consideration of page-sharing.
*/
void saovmp_model::bestFitSharing()
{	
	std::vector<vms_model> bfs_vms;
	std::vector<srv_model> bfs_srv;
		
	for(auto vm : saovmp_V)
		bfs_vms.push_back(vm);
	for(auto pm : saovmp_S)
		bfs_srv.push_back(pm);
		
	idx 		= -1;	
	scarcity_metric = std::numeric_limits<double>::min();
	for(int j = 0; j < bfs_vms.size(); j++)
	{	
		sharing_per_server(bfs_vms, bfs_srv);		
		server_resource_scarcity(bfs_vms, bfs_srv, bfs);
		for(int k = 0; k < bfs_srv.size(); k++)
		{	
			if(scarcity_metric < bfs_vms[j].scarcity[k] && (bfs_vms[j].scarcity[k] > 0))
			{
				scarcity_metric = bfs_vms[j].scarcity[k];
				idx = k;
			}	
		}
		
		for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
			saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
			if(bfs_vms[j].v_PI[pii] == true)
				bfs_srv[idx].PI[pii] = true;
					
		bfs_srv[idx].cpuCap = bfs_srv[idx].cpuCap - bfs_vms[j].cpuReq;
		bfs_srv[idx].memCap = bfs_srv[idx].memCap - bfs_vms[j].memReq + bfs_vms[j].sharing[idx];
		bfs_srv[idx].active = 1;
		bfs_srv[idx].reduct = true;
		bfs_vms[j].suspend = true;
		idx = -1;
		scarcity_metric = std::numeric_limits<double>::min();
	}
	
	process_allocation(bfs_vms,bfs_srv,bfs);
}

/*
First-Fit-Sharing: 
------------------
Assign each VM to the first server that fits its requirements in consideration of page-sharing.
*/
void saovmp_model::firstFitSharing()
{
	std::vector<vms_model> ffs_vms;
	std::vector<srv_model> ffs_srv;
		
	for(auto vm : saovmp_V)
		ffs_vms.push_back(vm);
	for(auto pm : saovmp_S)
		ffs_srv.push_back(pm);
		
	for(int j = 0; j < ffs_vms.size(); j++)
	{	
		sharing_per_server(ffs_vms, ffs_srv);		
		for(int k = 0; k < ffs_srv.size(); k++)
		{	
			if(	((ffs_srv[k].cpuCap - ffs_vms[j].cpuReq) >= 0.00) && 
				((ffs_srv[k].memCap - ffs_vms[j].memReq + ffs_vms[j].sharing[k] >= 0.00)))
			{
				for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
					saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
					if(ffs_vms[j].v_PI[pii] == true)
						ffs_srv[k].PI[pii] = true;
					
				ffs_srv[k].cpuCap = ffs_srv[k].cpuCap - ffs_vms[j].cpuReq;
				ffs_srv[k].memCap = ffs_srv[k].memCap - ffs_vms[j].memReq +ffs_vms[j].sharing[k];
				ffs_srv[k].active = 1;
				ffs_vms[j].suspend = true;
				ffs_srv[k].reduct = true;				
				break;
			}
		}
	}
	
	process_allocation(ffs_vms,ffs_srv,ffs);
}	


/*
Next-Fit-Sharing: 
------------------
Assign each VM to the only active server that fits its requirements in consideration of page-sharing.
*/
void saovmp_model::nextFitSharing()	
{
	int activeServer(0);
	std::vector<vms_model> nfs_vms;
	std::vector<srv_model> nfs_srv;
		
	for(auto vm : saovmp_V)
		nfs_vms.push_back(vm);
	for(auto pm : saovmp_S)
		nfs_srv.push_back(pm);

	nfs_srv[0].active = 1;
	
	for(int j = 0; j < nfs_vms.size(); j++)
	{	
		allocated = false;
		sharing_per_server(nfs_vms, nfs_srv);		
		while(allocated == false)
		{				
			if(nfs_srv[activeServer].active == -1)		//	inactive signal
				nfs_srv[activeServer].active = 1;		//	active signal
				
			if(	(	(nfs_srv[activeServer].cpuCap - nfs_vms[j].cpuReq) >= 0.00) && 
				(	(nfs_srv[activeServer].memCap - nfs_vms[j].memReq + nfs_vms[j].sharing[activeServer] >= 0.00)) &&
					(nfs_srv[activeServer].active == 1))
			{
				for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
					saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
					if(nfs_vms[j].v_PI[pii] == true)
						nfs_srv[activeServer].PI[pii] = true;
					
				nfs_srv[activeServer].cpuCap = nfs_srv[activeServer].cpuCap - nfs_vms[j].cpuReq;
				nfs_srv[activeServer].memCap = nfs_srv[activeServer].memCap - nfs_vms[j].memReq + nfs_vms[j].sharing[activeServer];
				nfs_vms[j].suspend = true;
				nfs_srv[activeServer].reduct = true;
				allocated = true;
			}
			else
			{
				nfs_srv[activeServer].active = 0;
				activeServer++;
			}	
		}
		activeServer = 0;		//	closed signal
	}

	process_allocation(nfs_vms, nfs_srv, nfs);
}	

void saovmp_model::process_allocation(std::vector<vms_model> &(process_V), std::vector<srv_model> &(process_S), algorithms strat)
{	
	int getBinSize(0);
	bool guarantee = true;
	double countServerTypes[saovmp_consts::SERVER_TYPES] = {};	
	double remainingMemResource[saovmp_consts::SERVER_TYPES] = {};
	
	//	Identify the number of active servers in environment.
	for(int k = 0; k < process_S.size(); k++)
		if(process_S[k].reduct == true)
			getBinSize++;

	//	Guarantee all VMs are instantiated
	for(int j = 0; j < process_V.size(); j++)
		if(process_V[j].suspend != true)
			guarantee = false;			
			
	//	Store the number of active bins
	if(guarantee)
		comparator[strat] = getBinSize;
	else
		comparator[strat] = -1;
	
	for(int k = 0; k < process_S.size(); k++)
	{
		if(process_S[k].reduct)
		{
			countServerTypes[process_S[k].type]++;
			remainingMemResource[process_S[k].type] += process_S[k].memCap;
		}
	}
	
	for(int i = 0; i < saovmp_consts::SERVER_TYPES; i++)
		activeServersByType[i][strat] = countServerTypes[i];
	
	for(int i = 0; i < saovmp_consts::SERVER_TYPES; i++)
	{
		if(countServerTypes[i] == 0)
			utilizedMemPerServer[i][strat] = -1;
		else
			utilizedMemPerServer[i][strat] = countServerTypes[i]*initServerMem[i] - remainingMemResource[i];
	}
}

void saovmp_model::initPrototype(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S), std::string vmInputFile, std::string serverInputFile)
{
	// Upload VMs for Experiment // 
	std::ifstream open_vm(vmInputFile);
	if(open_vm.fail()){std::cout << "Error opening: VM Input File.\n"; exit(0);}
	
	// Prep storage variables for uploading VMs; following vms_model
	vms_model arbVMS = {};
	int arb_id = 0;
	std::string arb_type = {};
	double arbCpuReq = 0.00;
	double arbMemReq = 0.00;
	double arbShareable_memReq = 0.00;
	double arbNonShareable_memReq = 0.00;	

	while(open_vm >> arb_type >> arbCpuReq >> arbMemReq >> arbShareable_memReq >> arbNonShareable_memReq)
	{
		arbVMS.id 			= ++arb_id;
		arbVMS.type 			= arb_type;
		arbVMS.cpuReq 			= arbCpuReq;
		arbVMS.memReq 			= arbMemReq;
		arbVMS.shareable_memReq     	= arbShareable_memReq;
		arbVMS.non_shareable_memReq 	= arbNonShareable_memReq;
		input_V.push_back(arbVMS);
	}
	
	for(auto &vm : input_V)
	{
		for(int jj = 0; jj < saovmp_consts::slotSizeAP; jj++)	//Initialize all Applications
		{
			vm.IF.push_back(0);	
			vm.DV.push_back(0);	
			vm.CM.push_back(0);	
			vm.CR.push_back(0);	
			vm.DB.push_back(0);	
		}
	}
	open_vm.close();

	// Upload Server Configuration for Experiment // 
	arb_id = 0;
	std::ifstream open_srv(serverInputFile);
	if(open_srv.fail()){std::cout << "Error opening: Server Input File.\n"; exit(0);}
	
	// Prep storage variables for uploading Server Configuration; following srv_model
	srv_model arbSRV = {};
	double arbMemCap = 0.00;
	double arbCpuCap = 0.00;
	int	arbCountServersPerType = 0;
	int arbServerType = 0;
	while(open_srv >> arbCpuCap >> arbMemCap >> arbCountServersPerType)
	{
		initServerMem[arbServerType] = arbMemCap; 
		for(int i = 0; i < arbCountServersPerType; i++)
		{	
			arbSRV.id 		= ++arb_id;
			arbSRV.type		= arbServerType;
			arbSRV.memCap 		= arbMemCap;
			arbSRV.cpuCap 		= arbCpuCap;		
			input_S.push_back(arbSRV);
		}
		++arbServerType;
	}	
	open_srv.close();
	for(auto &vm : input_V)			//Initialize Sharing and Scarcity Fields
	{
		for(auto &s : input_S)			
		{
			vm.sharing.push_back(0);	
			vm.scarcity.push_back(0);	
		}
	}	
}

void saovmp_model::prepareInputs(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S))
{
	for(auto vm : input_V)
		saovmp_V.push_back(vm);
	for(auto pm : input_S)
		saovmp_S.push_back(pm);
}

void saovmp_model::sharing_per_server(std::vector<vms_model> (&input_V), std::vector<srv_model> (&input_S))
{
	for(int k = 0; k < input_S.size(); k++)
		for(int j = 0; j < input_V.size(); j++)
			input_V[j].sharing.at(k) = 0.00;
	
	for(int k = 0; k < input_S.size(); k++)
		for(int j = 0; j < input_V.size(); j++)
			for(int i = 0; i < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
				saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; i++)
				if((input_V[j].v_PI[i] == true) &&  (input_S[k].PI[i] == true))
					input_V[j].sharing.at(k) = input_V[j].sharing.at(k) + 1;
}

void saovmp_model::server_resource_scarcity(std::vector<vms_model> &(calc_scarcity_V), std::vector<srv_model> &(calc_scarcity_S), algorithms strat)
{
	double tempMEM(0.00);
	double tempCPU(0.00);
	
	for(int k = 0; k < calc_scarcity_S.size(); k++)
		for(int j = 0; j < calc_scarcity_V.size(); j++)
			calc_scarcity_V[j].scarcity[k] = 0.00;
			
	for(int k = 0; k < calc_scarcity_S.size(); k++)
	{
		for(int j = 0; j < calc_scarcity_V.size(); j++)
		{
			if(	(calc_scarcity_S[k].cpuCap - calc_scarcity_V[j].cpuReq < 0.00) || 
				(calc_scarcity_S[k].memCap - calc_scarcity_V[j].memReq + calc_scarcity_V[j].sharing[k]  < 0.00))			
			{
				calc_scarcity_V[j].scarcity[k] = -1.00;
			}
			else
			{					
				tempCPU = (calc_scarcity_V[j].cpuReq/calc_scarcity_S[k].cpuCap);
				tempMEM = ((calc_scarcity_V[j].memReq - sqrt(calc_scarcity_V[j].sharing[k]))/calc_scarcity_S[k].memCap);
				
				switch(strat)
				{
					case bfs:	
					case bf:	if(tempCPU > tempMEM)
									calc_scarcity_V[j].scarcity[k] = tempCPU;
								else
									calc_scarcity_V[j].scarcity[k] = tempMEM;
								break;
					case wfs:	
					case wf:	if(tempCPU < tempMEM)
									calc_scarcity_V[j].scarcity[k] = tempCPU;
								else
									calc_scarcity_V[j].scarcity[k] = tempMEM;
								break;	
				}	
			}
		}
	}
}

void saovmp_model::nextFit()	
{
	int activeServer(0);
	std::vector<vms_model> nf_vms;
	std::vector<srv_model> nf_srv;
		
	for(auto vm : saovmp_V)
		nf_vms.push_back(vm);
	for(auto pm : saovmp_S)
		nf_srv.push_back(pm);

	nf_srv[0].active = 1;
	
	for(int j = 0; j < nf_vms.size(); j++)
	{	
		allocated = false;
		while(allocated == false)
		{				
			if(nf_srv[activeServer].active == -1)		//	inactive
				nf_srv[activeServer].active = 1;		//	active
				
			if(	(	(nf_srv[activeServer].cpuCap - nf_vms[j].cpuReq) >= 0.00) && 
				(	(nf_srv[activeServer].memCap - nf_vms[j].memReq  >= 0.00)) &&
					(nf_srv[activeServer].active == 1))
			{
				for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
					saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
					if(nf_vms[j].v_PI[pii] == true)
						nf_srv[activeServer].PI[pii] = true;
					
				nf_srv[activeServer].cpuCap = nf_srv[activeServer].cpuCap - nf_vms[j].cpuReq;
				nf_srv[activeServer].memCap = nf_srv[activeServer].memCap - nf_vms[j].memReq;
				nf_vms[j].suspend = true;
				nf_srv[activeServer].reduct = true;
				allocated = true;
			}
			else
			{
				nf_srv[activeServer].active = 0;
				activeServer++;
			}	
		}
		activeServer = 0;		//	closed
	}
	
	process_allocation(nf_vms, nf_srv, nf);
}	

void saovmp_model::firstFit()
{
	std::vector<vms_model> ff_vms;
	std::vector<srv_model> ff_srv;
		
	for(auto vm : saovmp_V)
		ff_vms.push_back(vm);
	for(auto pm : saovmp_S)
		ff_srv.push_back(pm);
		
	for(int j = 0; j < ff_vms.size(); j++)
	{	
		sharing_per_server(ff_vms, ff_srv);		
		for(int k = 0; k < ff_srv.size(); k++)
		{	
			if(	((ff_srv[k].cpuCap - ff_vms[j].cpuReq) >= 0.00) && 
				((ff_srv[k].memCap - ff_vms[j].memReq  >= 0.00)))
			{
				for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
					saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
					if(ff_vms[j].v_PI[pii] == true)
						ff_srv[k].PI[pii] = true;
					
				ff_srv[k].cpuCap = ff_srv[k].cpuCap - ff_vms[j].cpuReq;
				ff_srv[k].memCap = ff_srv[k].memCap - ff_vms[j].memReq;
				ff_srv[k].active = 1;
				ff_vms[j].suspend = true;
				ff_srv[k].reduct = true;				
				break;
			}
		}
	}
	
	process_allocation(ff_vms,ff_srv,ff);
}

void saovmp_model::bestFit()
{	
	std::vector<vms_model> bf_vms;
	std::vector<srv_model> bf_srv;
		
	for(auto vm : saovmp_V)
		bf_vms.push_back(vm);
	for(auto pm : saovmp_S)
		bf_srv.push_back(pm);
		
	idx = -1;	
	scarcity_metric = std::numeric_limits<double>::min();
	for(int j = 0; j < bf_vms.size(); j++)
	{	
		server_resource_scarcity(bf_vms, bf_srv, bf);
		for(int k = 0; k < bf_srv.size(); k++)
		{	
			if(scarcity_metric < bf_vms[j].scarcity[k] && (bf_vms[j].scarcity[k] > 0))
			{
				idx = k;
				scarcity_metric = bf_vms[j].scarcity[k];
			}	
		}
		
		for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
			saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
			if(bf_vms[j].v_PI[pii] == true)
				bf_srv[idx].PI[pii] = true;
					
		bf_srv[idx].cpuCap = bf_srv[idx].cpuCap - bf_vms[j].cpuReq;
		bf_srv[idx].memCap = bf_srv[idx].memCap - bf_vms[j].memReq;
		bf_srv[idx].active = 1;
		bf_srv[idx].reduct = true;
		bf_vms[j].suspend = true;
		idx = -1;
		scarcity_metric 	= std::numeric_limits<double>::min();
	}
	
	process_allocation(bf_vms,bf_srv,bf);
}

void saovmp_model::worstFit()
{		
	std::vector<vms_model> wf_vms;
	std::vector<srv_model> wf_srv;
		
	for(auto vm : saovmp_V)
		wf_vms.push_back(vm);
	for(auto pm : saovmp_S)
		wf_srv.push_back(pm);
	
	idx = -1;	
	scarcity_metric = std::numeric_limits<double>::max();
	for(int j = 0; j < wf_vms.size(); j++)
	{	
		server_resource_scarcity(wf_vms, wf_srv, wf);
		for(int k = 0; k < wf_srv.size(); k++)
		{	
			if(scarcity_metric > wf_vms[j].scarcity[k] && (wf_vms[j].scarcity[k] > 0))
			{
				scarcity_metric = wf_vms[j].scarcity[k];
				idx = k;
			}	
		}
		
		for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
			saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
			if(wf_vms[j].v_PI[pii] == true)
				wf_srv[idx].PI[pii] = true;
					
		wf_srv[idx].cpuCap = wf_srv[idx].cpuCap - wf_vms[j].cpuReq;
		wf_srv[idx].memCap = wf_srv[idx].memCap - wf_vms[j].memReq;
		wf_srv[idx].active = 1;
		wf_srv[idx].reduct = true;
		wf_vms[j].suspend = true;
		idx = -1;
		scarcity_metric = std::numeric_limits<double>::max();
	}
	
	process_allocation(wf_vms, wf_srv, wf);
}


void saovmp_model::randomFit()
{	
	std::vector<vms_model> rf_vms;
	std::vector<srv_model> rf_srv;
		
	for(auto vm : saovmp_V)
		rf_vms.push_back(vm);
	for(auto pm : saovmp_S)
		rf_srv.push_back(pm);

	for(int j = 0; j < rf_vms.size(); j++)
	{	
 		allocated = false;
		while(allocated == false)
		{	
			std::uniform_int_distribution<> distrbution(0, rf_srv.size());
			int select = distrbution(generator);
			if(	((rf_srv[select].cpuCap - rf_vms[j].cpuReq) >= 0.00) && 
				((rf_srv[select].memCap - rf_vms[j].memReq  >= 0.00)))
			{
				for(int pii = 0; pii < saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + 
					saovmp_consts::numOfAps*saovmp_consts::slotSizeAP; pii++)
					if(rf_vms[j].v_PI[pii] == true)
						rf_srv[select].PI[pii] = true;
					
				rf_srv[select].cpuCap = rf_srv[select].cpuCap - rf_vms[j].cpuReq;
				rf_srv[select].memCap = rf_srv[select].memCap - rf_vms[j].memReq;
				rf_srv[select].active = 1;
				rf_srv[select].reduct = true;
				rf_vms[j].suspend = true;
				allocated = true;
			}
		}
	}
	
	process_allocation(rf_vms, rf_srv, rf);
}

void saovmp_model::generateResults(std::string inputVMFile, std::string inputServerFile)
{
	std::cout << "VM Input File:\t\t" << inputVMFile  << "\n";
	std::cout << "Server Input File:\t" << inputServerFile  << "\n\n";
	
	std::cout << "********** Total Active Servers Per Allocation Strategy **********\n";
	for(int algo_id = saovmp_model::nfs; algo_id != saovmp_model::end_strats; algo_id++)
	{
		switch(algo_id)
		{
			case nfs	:  	std::cout << "NFS:\t" << comparator[algo_id] << "\t";
							break;
			case ffs		:  	std::cout << "FFS:\t" << comparator[algo_id] << "\t";
							break;
			case bfs	:  	std::cout << "BFS:\t" << comparator[algo_id] << "\t";
							break;						
			case wfs	:  	std::cout << "WFS:\t" << comparator[algo_id] << "\t";
							break;						
			case rfs	:  	std::cout << "RFS:\t" << comparator[algo_id] << "\n";
							break;						
			case nf		:	std::cout << "NF :\t" << comparator[algo_id] << "\t";
							break;
			case ff		:  	std::cout << "FF :\t" << comparator[algo_id] << "\t";
							break;
			case bf		:  	std::cout << "BF :\t" << comparator[algo_id] << "\t";
							break;						
			case wf		:  	std::cout << "WF :\t" << comparator[algo_id] << "\t";
							break;						
			case rf		:  	std::cout << "RF :\t" << comparator[algo_id] << "\t";
							break;						
			default		:	
							break;
		}
	}	

	std::ifstream getServerTypeInfo(inputServerFile);
	if(getServerTypeInfo.fail()){std::cout << "Error opening: Server Input File.\n"; exit(0);}
		
	int inputCPU(0);
	int inputMem(0);
	int inputNumServ(0);
	
	std::cout << "\n\n******************** Total Active Servers Per Server Type ( CPU in Cores, MEM in GBs ) ********************\n\t\t";	
	while(getServerTypeInfo >> inputCPU >> inputMem >> inputNumServ)
		std::cout << "(" << inputCPU << " , " << inputMem << ")\t";
	getServerTypeInfo.close();
	
	for(int algo_id = saovmp_model::nfs; algo_id != saovmp_model::end_strats; algo_id++)
	{
		switch(algo_id)
		{
			case nfs	:  	std::cout << "\nNFS:\t\t";
							break;
			case ffs		:  	std::cout << "\nFFS:\t\t";
							break;
			case bfs	:  	std::cout << "\nBFS:\t\t";
							break;						
			case wfs	:  	std::cout << "\nWFS:\t\t";
							break;						
			case rfs	:  	std::cout << "\nRFS:\t\t";
							break;						
			case nf		:	std::cout << "\nNF :\t\t";
							break;
			case ff		:  	std::cout << "\nFF :\t\t";
							break;
			case bf		:  	std::cout << "\nBF :\t\t";
							break;						
			case wf		:  	std::cout << "\nWF :\t\t";
							break;						
			case rf		:  	std::cout << "\nRF :\t\t";
							break;						
			default		:	
							break;
		}

		for(int i = 0; i < saovmp_consts::SERVER_TYPES; i++)
			std::cout << activeServersByType[i][algo_id] << "\t\t\t";
	}
}

#endif