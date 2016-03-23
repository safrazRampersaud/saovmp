#ifndef SRV_MODEL_HPP
#define SRV_MODEL_HPP

class srv_model
{
	public:
		int id = 0;
		int type = - 1;
		std::vector<int> assign;
		double memCap = 0.00;
		double cpuCap = 0.00;
		double SRVpro = 0.00;
		int active = -1; 
		bool reduct = 0;
		bool PI[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP] = {};	
		int A[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP] = {};
};
#endif