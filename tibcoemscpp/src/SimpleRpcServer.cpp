#include "stdafx.h"
#include "SimpleRpcServer.h"
#include "boost/lexical_cast.hpp"

using namespace TibcoEMSClient;

SimpleRpcServer::SimpleRpcServer(TibcoSession::ptr_t mySession, const std::string &rpc_name) : m_incoming_tag(rpc_name), session(mySession)
{
	session->BasicConsumer(rpc_name);
}

bool SimpleRpcServer::GetNextIncomingMessage(TibcoMessage::ptr_t &nextMsg)
{
	try {
		nextMsg = session->BasicConsumeMessage();
	}
	catch (std::logic_error &e) {
		return false;
	}
	return true;
}

void SimpleRpcServer::RespondToMessage(TibcoMessage::ptr_t request, TibcoMessage::ptr_t response)
{
	try {
		if (request->CorrelationIdIsSet() && !response->CorrelationIdIsSet())
			response->CorrelationId(request->CorrelationId());
		session->BasicPublish(request->ReplyTo(), response);
	}
	catch (std::logic_error &e) {
		// TODO: Can add logger
	}
}

void SimpleRpcServer::RespondToMessage(TibcoMessage::ptr_t request, const std::string response)
{
	RespondToMessage(request, TibcoMessage::Create(response));
}

SimpleRpcServer::~SimpleRpcServer()
{
}
