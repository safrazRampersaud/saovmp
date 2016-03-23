#ifndef SRV_MODEL_HPP
#define SRV_MODEL_HPP

class srv_model
{
	public:
		srv_model();
		~srv_model();
		int id;
		int type;
		std::vector<int> assign;
		double memCap;
		double cpuCap;
		double SRVpro;
		int active; 
		bool reduct;
		bool PI[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP];	
		int A[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP];
};

srv_model::srv_model() : id(0), type(-1), memCap(0.00), cpuCap(0.00), SRVpro(0.00), active(-1), reduct(false)
{
	PI[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP] = {};	
	A[saovmp_consts::numOfOps*saovmp_consts::slotSizeOS + saovmp_consts::numOfAps*saovmp_consts::slotSizeAP] = {};

}

srv_model::~srv_model() {}

#endif