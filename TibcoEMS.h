#pragma once
#include "boost/smart_ptr.hpp"
#include <boost/date_time/posix_time/posix_time.hpp>
#include "PriorityRequestScheduler_TUI.h"
#include "NetworkLog_TUI.h"
#include "ParallelProcess.h"
#include "SimpleRpcServer.h"

using namespace boost::posix_time;
using namespace TibcoEMSClient;

namespace avail {
	class AvailRequest;
}

class TibcoEMSDaemon {
private:
	PriorityRequestScheduler_TUI &scheduler;

public:
	TibcoEMSDaemon(PriorityRequestScheduler_TUI &sched) : scheduler(sched) {}
	bool Start(int numThreads);
};

class TibcoEMSThread {
private:
	PriorityRequestScheduler_TUI &scheduler;	
	SimpleRpcServer::ptr_t server;
	bool connected;

private:
	void ProcessMessage(TibcoMessage::ptr_t &msg) const;

public:
	TibcoEMSThread(PriorityRequestScheduler_TUI &scheduler);		
	void operator()() const;
};


class TibcoEMSProcess: public SimpleProcess {
private:
	SimpleRpcServer::ptr_t server;
	TibcoMessage::ptr_t message;

private:
	string GetToken(unsigned int index, const vector<string> &tokens);
	bool ParseRequest(const string &req, avail::AvailRequest &request);

protected:
	virtual void ExecuteSimple();

public:
	TibcoEMSProcess(const SimpleRpcServer::ptr_t &myServer, const TibcoMessage::ptr_t &myRequest) : server(myServer), message(myRequest) { }
	string AvailabilityResponse(const string &line);
	virtual ~TibcoEMSProcess() {}
	virtual BaseProcess *Clone() const;
};

