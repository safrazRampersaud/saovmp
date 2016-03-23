#ifndef VMS_MODEL_HPP
#define VMS_MODEL_HPP

class vms_model
{
	public:
		int    id = 0;
		int serverID = -1;
		bool   active = true;			
		bool   suspend = false;			
		double revenue = 0.00;
		double cpuReq = 0.00;
		double memReq = 0.00;
		double shareable_memReq = 0.00;
		double non_shareable_memReq = 0.00;	
		std::string type = {};
		std::vector<double> sharing;					 
		std::vector<double> scarcity;				 
		std::vector<bool> IF;	
		std::vector<bool> DB;	
		std::vector<bool> CM;	
		std::vector<bool> CR;	
		std::vector<bool> DV;	
		std::vector<bool> OS;
		bool v_PI[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP] = {};	

		void requestOperatingSystem(std::vector<vms_model> (&_V));
		void requestApplications(std::vector<vms_model> (&_V));
};

void vms_model::requestOperatingSystem(std::vector<vms_model> (&_V))
{
	std::ifstream getOSPages(".\\RESOURCES\\getOperatingSystemPages");
	if(getOSPages.fail()){std::cout << "Error opening: getOperatingSystemPages.\n"; exit(0);}
	
	int arbPage(0);
	std::vector<bool> inputBitVec;
	std::vector<bool> OPS_Pages_Push;
	std::vector<std::vector<bool> > OPS_Pages;
	
	//	Upload operating system pages from "getOperatingSystemPages"
	while(getOSPages >> arbPage)
		inputBitVec.push_back(arbPage);
	getOSPages.close();
	
	//	Store operating system pages from inputBitVec into OPS_Pages
	for(int i = 0; i < saovmp_consts::numOfOPVers; i++)
	{
		for(int j = 0; j < saovmp_consts::slotSizeOS; j++)
			OPS_Pages_Push.push_back(inputBitVec.at(i*saovmp_consts::slotSizeOS + j));
		OPS_Pages.push_back(OPS_Pages_Push);
		OPS_Pages_Push.clear();
	}
	
	bool arbBool(0);
	std::vector<bool> OSconstraintVals;
	std::vector<bool> OSconstraintMatrix_Push;
	std::vector<std::vector<bool> > OSconstraintMatrix;
	
	std::string arbString = {};
	std::vector<std::string> vmTypeContainer(0);

	//	Upload VM types used in this experiment from "vmTypes" and store into "vmTypeContainer"
	std::ifstream vmTypeRef(".\\RESOURCES\\vmTypes");
	if(vmTypeRef.fail()){std::cout << "Error opening: vmTypes.\n"; exit(0);}
	
	while(vmTypeRef >> arbString)
		vmTypeContainer.push_back(arbString);
	vmTypeRef.close();
	
	//	Upload the VM-OS constraint matrix from "VM_assign_OS" and store into "OSconstraintVals"
	std::ifstream assignOS(".\\RESOURCES\\VM_assign_OS");
	if(assignOS.fail()){std::cout << "Error opening: VM_assign_OS.\n"; exit(0);}

	while(assignOS >> arbBool)
		OSconstraintVals.push_back(arbBool);
	assignOS.close();	

	//	Using "vmTypeContainer" and "OSconstraintVals", build "OSconstraintMatrix" for VM operating system assignments 
	for(int i = 0; i < vmTypeContainer.size(); i++)
	{	
		for(int j = 0; j < saovmp_consts::numOfOPVers; j++)	
			OSconstraintMatrix_Push.push_back(OSconstraintVals.at(i*saovmp_consts::numOfOPVers + j));
		OSconstraintMatrix.push_back(OSconstraintMatrix_Push);
		OSconstraintMatrix_Push.clear();
	}

	for(int j = 0; j < _V.size(); j++)	//For every VM in experiment
	{
		for(int jo = 0; jo < saovmp_consts::slotSizeOS; jo++)	// Initialize the Operating System per VM
			_V.at(j).OS.push_back(0);
		for(int k = 0; k < vmTypeContainer.size(); k++)	//For every VM Type 
		{		
			if(_V.at(j).type == vmTypeContainer.at(k))	//If the VM in experiment if the type is identified, then 
			{
				for(int i = 0; i < saovmp_consts::numOfOPVers; i++)	// For the number of operating systems	
				{
					if(OSconstraintMatrix.at(k).at(i))	// Identify the legal servers the VM can choose, under the constraint matrix
					{
						for(int l = 0; l < saovmp_consts::slotSizeOS; l++)
							if(OPS_Pages.at(i).at(l))
								_V[j].OS[l] = OPS_Pages[i][l];	//Write the bits to the VM's OS field.
					}
				}
			}
		}
	}
	for(auto &vm : _V)
	{
		for(int ii = 0; ii <  saovmp_consts::slotSizeOS; ii++)
			vm.v_PI[ii] = vm.OS[ii];
	}
}

void vms_model::requestApplications(std::vector<vms_model> (&_V))
{
	int arbPage(0);
	int allocation_idx(0);
	std::vector<bool> inputBitVec;
	std::vector<bool> APS_Pages_Push;
	std::vector<std::vector<bool> > APS_Pages;
	
	//	Upload application pages from "getApplicationPages"
	std::ifstream getAPSPages(".\\RESOURCES\\getApplicationPages");
	if(getAPSPages.fail()){std::cout << "Error opening: Application Pages Support File.\n"; exit(0);}
	while(getAPSPages >> arbPage)
		inputBitVec.push_back(arbPage);
	getAPSPages.close();
		
	std::string arbString = {};
	std::vector<std::string> vmTypeContainer(0);

	//	Upload VM types used in this experiment from "vmTypes" and store into "vmTypeContainer"
	std::ifstream vmTypeRef(".\\RESOURCES\\vmTypes");
	if(vmTypeRef.fail()){std::cout << "Error opening: VM Type Support File.\n"; exit(0);}
	while(vmTypeRef >> arbString)
		vmTypeContainer.push_back(arbString);
	vmTypeRef.close();

	//	Store application pages from inputBitVec into APS_Pages
	for(int i = 0; i < saovmp_consts::numOfAPVers; i++)
	{
		for(int j = 0; j < saovmp_consts::slotSizeAP; j++)
			APS_Pages_Push.push_back(inputBitVec.at(i*saovmp_consts::slotSizeAP+ j));
		APS_Pages.push_back(APS_Pages_Push);
		APS_Pages_Push.clear();
	}

	bool arbBool(0);
	std::vector<bool> APconstraintVals;
	std::vector<bool> APconstraintMatrix_Push;
	std::vector<std::vector<bool> > APconstraintMatrix;
		
	//	Upload the VM-AP constraint matrix from "VM_assign_AP" and store into "APconstraintVals"
	std::ifstream assignAP(".\\RESOURCES\\VM_assign_AP");
	if(assignAP.fail()){std::cout << "Error opening: VM to Application Support File.\n"; exit(0);}
	while(assignAP >> arbBool)
		APconstraintVals.push_back(arbBool);
	assignAP.close();	

	//	Using "vmTypeContainer" and "APconstraintVals", build "APconstraintMatrix" for VM application assignments 
	for(int i = 0; i < vmTypeContainer.size(); i++)
	{	
		for(int j = 0; j < saovmp_consts::numOfAps*saovmp_consts::numOfAPVers; j++)	
			APconstraintMatrix_Push.push_back(APconstraintVals.at(i*(saovmp_consts::numOfAps*saovmp_consts::numOfAPVers) + j));
		APconstraintMatrix.push_back(APconstraintMatrix_Push);
		APconstraintMatrix_Push.clear();
	}

	//	Upload the VM-AP enumeration file which details the amount of application the VM can hold.
	std::vector<int> APconstraintEnum; 
	int arbInt(0);
	std::ifstream enumAP(".\\RESOURCES\\VM_assign_EnumAP");
	if(enumAP.fail()){std::cout << "Error opening: VM Constraint Support File.\n"; exit(0);}
	while(enumAP >> arbInt)
		APconstraintEnum.push_back(arbInt);
	enumAP.close();		
	
	for(int j = 0; j < _V.size(); j++)	//For every VM in experiment
	{
		int countEnumApplications(0);
		std::vector<int> applicationIndices(0);
		for(int k = 0; k < vmTypeContainer.size(); k++)	//For every VM Type 
		{		
			if(_V.at(j).type == vmTypeContainer.at(k))	//If the VM in experiment if the type is identified, then 
			{
				countEnumApplications = APconstraintEnum.at(k);
				for(int i = 0; i < saovmp_consts::numOfAPVers*saovmp_consts::numOfAps; i++)	// For the number of applications	
					if(APconstraintMatrix.at(k).at(i))
						applicationIndices.push_back(i);
	
					random_shuffle(applicationIndices.begin(), applicationIndices.end());
					while(countEnumApplications)
					{	
						if((applicationIndices[0] >= 0) && (applicationIndices[0] <= saovmp_consts::numOfAPVers-1))
						{
							for(int l = 0; l < saovmp_consts::slotSizeAP; l++)
								if(APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l))
									_V.at(j).IF.at(l) =  APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l);
						}
						else if((applicationIndices[0] >= saovmp_consts::numOfAPVers) && (applicationIndices[0] <= 2*saovmp_consts::numOfAPVers-1))
						{
							for(int l = 0; l < saovmp_consts::slotSizeAP; l++)
								if(APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l))
									_V.at(j).DV.at(l) =  APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l);
						}
						else if((applicationIndices[0] >= 2*saovmp_consts::numOfAPVers) && (applicationIndices[0] <= 3*saovmp_consts::numOfAPVers-1))
						{
							for(int l = 0; l < saovmp_consts::slotSizeAP; l++)
								if(APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l))
									_V.at(j).CM.at(l) =  APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l);							
						}
						else if((applicationIndices[0] >= 3*saovmp_consts::numOfAPVers) && (applicationIndices[0] <= 4*saovmp_consts::numOfAPVers-1))
						{
							for(int l = 0; l < saovmp_consts::slotSizeAP; l++)
								if(APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l))
									_V.at(j).CR.at(l) =  APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l);							
						}
						else if((applicationIndices[0] >= 4*saovmp_consts::numOfAPVers) && (applicationIndices[0] <= 5*saovmp_consts::numOfAPVers-1)) 
						{	
							for(int l = 0; l < saovmp_consts::slotSizeAP; l++)
								if(APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l))
									_V.at(j).DB.at(l) =  APS_Pages.at(applicationIndices[0] % saovmp_consts::numOfAPVers).at(l);							
						}
						else
						{
							exit(1);
						}
						applicationIndices.erase(applicationIndices.begin() + 0);	
						countEnumApplications--;
					}
					applicationIndices.clear();
			}
		}		
	}
	for(auto &vm : _V)
	{	
		for(int ii = saovmp_consts::slotSizeOS; ii < saovmp_consts::slotSizeOS + 1*saovmp_consts::slotSizeAP; ii++)
			vm.v_PI[ii] = vm.IF[allocation_idx++];		
		allocation_idx = 0;
		for(int ii = saovmp_consts::slotSizeOS + 1*saovmp_consts::slotSizeAP; ii < saovmp_consts::slotSizeOS + 2*saovmp_consts::slotSizeAP; ii++)
			vm.v_PI[ii] = vm.DV[allocation_idx++];
		allocation_idx = 0;		
		for(int ii = saovmp_consts::slotSizeOS + 2*saovmp_consts::slotSizeAP; ii < saovmp_consts::slotSizeOS + 3*saovmp_consts::slotSizeAP; ii++)
			vm.v_PI[ii] = vm.CM[allocation_idx++];		
		allocation_idx = 0;		
		for(int ii = saovmp_consts::slotSizeOS + 3*saovmp_consts::slotSizeAP; ii < saovmp_consts::slotSizeOS + 4*saovmp_consts::slotSizeAP; ii++)
			vm.v_PI[ii] = vm.CR[allocation_idx++];		
		allocation_idx = 0;		
		for(int ii = saovmp_consts::slotSizeOS + 4*saovmp_consts::slotSizeAP; ii < saovmp_consts::slotSizeOS + 5*saovmp_consts::slotSizeAP; ii++)
			vm.v_PI[ii] = vm.DB[allocation_idx++];
		allocation_idx = 0;		
	}
}
#endif