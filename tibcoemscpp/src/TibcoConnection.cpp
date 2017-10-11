#include "stdafx.h"
#include "TibcoConnection.h"
#include "boost/lexical_cast.hpp"

using namespace TibcoEMSClient;

//***************************************************************************************************************
// TibcoConnection
// TODO: Falta pasar el usuari/contrasenya al constructor de moment establim la connexió amb null
//***************************************************************************************************************

TibcoConnection::TibcoConnection(const std::string &url)
{
	gErrorContext = NULL;

	tibems_status status = TIBEMS_OK;
	status = tibemsErrorContext_Create(&gErrorContext);
	if (status != TIBEMS_OK)
		throw std::logic_error(boost::lexical_cast<string>(format("ErrorContext create failed: %s") % tibemsStatus_GetText(status)));

	tibemsConnectionFactory factory = NULL;
	factory = tibemsConnectionFactory_Create();
	if (!factory)
		throw std::logic_error("Error creating tibemsConnectionFactory");

	status = tibemsConnectionFactory_SetServerURL(factory, url.c_str());
	if (status != TIBEMS_OK)
	{
		const char*strErr = NULL;
		tibemsErrorContext_GetLastErrorString(gErrorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error setting server url: %s") % strErr));
	}

	status = tibemsConnectionFactory_CreateConnection(factory, &connection, NULL, NULL);
	if (status != TIBEMS_OK)
	{
		const char*strErr = NULL;
		tibemsErrorContext_GetLastErrorString(gErrorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error creating tibemsConnection: %s") % strErr));
	}
}

bool TibcoConnection::Start()
{
	tibems_status status = tibemsConnection_Start(connection);
	return (status == TIBEMS_OK);
}

void TibcoConnection::CreateSession(tibemsSession &session)
{
	tibems_status status = TIBEMS_OK;
	status = tibemsConnection_CreateSession(connection, &session, TIBEMS_FALSE, TIBEMS_AUTO_ACKNOWLEDGE);
	if (status != TIBEMS_OK)
	{
		const char *strErr = NULL;
		tibemsErrorContext_GetLastErrorString(gErrorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error creating tibemsSession: %s") % strErr));
	}
}

TibcoConnection::~TibcoConnection()
{
	tibemsConnection_Close(connection);
	tibemsErrorContext_Close(gErrorContext);
}

//***************************************************************************************************************
// TibcoSession
//***************************************************************************************************************

TibcoSession::TibcoSession(TibcoConnection::ptr_t myConn) : conn(myConn)
{
	// Crear una sessio en la connexió que ens passen
	conn->CreateSession(session);

	tibems_status status = TIBEMS_OK;
	status = tibemsErrorContext_Create(&errorContext);
	if (status != TIBEMS_OK)
		throw std::logic_error(boost::lexical_cast<string>(format("ErrorContext in session create failed: %s") % tibemsStatus_GetText(status)));
}

tibemsDestination TibcoSession::CreateTemporaryQueue() const
{
	tibemsDestination destReplies;
	tibems_status status = tibemsSession_CreateTemporaryQueue(session, &destReplies);
	if (status != TIBEMS_OK)
		throw std::logic_error("Error creating temporary queue");
	return destReplies;
}

void TibcoSession::BasicConsumer(const std::string &queue_name)
{
	tibems_status status = TIBEMS_OK;
	tibemsDestination dest;

	status = tibemsQueue_Create(&dest, queue_name.c_str());
	if (status != TIBEMS_OK)
	{
		const char *strErr = NULL;
		tibemsErrorContext_GetLastErrorString(errorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error binding to queue: %s") % strErr));
	}

	status = tibemsSession_CreateConsumer(session, &consumer, dest, NULL, TIBEMS_FALSE);
	if (status != TIBEMS_OK)
	{
		const char *strErr = NULL;
		tibemsErrorContext_GetLastErrorString(errorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error creating consumer: %s") % strErr));
	}
}

void TibcoSession::BasicProducer(const std::string &queue_name)
{
	ProducersMap::const_iterator found = producers.find(queue_name);
	if (found != producers.end())
		return;

	tibems_status status = TIBEMS_OK;
	tibemsDestination dest;

	status = tibemsQueue_Create(&dest, queue_name.c_str());
	if (status != TIBEMS_OK)
	{
		const char *strErr = NULL;
		tibemsErrorContext_GetLastErrorString(errorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error binding to queue: %s") % strErr));
	}

	tibemsMsgProducer producer;

	status = tibemsSession_CreateProducer(session, &producer, dest);
	if (status != TIBEMS_OK)
	{
		const char *strErr = NULL;
		tibemsErrorContext_GetLastErrorString(errorContext, &strErr);
		throw std::logic_error(boost::lexical_cast<string>(format("Error creating producer: %s") % strErr));
	}

	producers[queue_name] = producer;
}

tibemsMsgRequestor TibcoSession::BasicRequestor(const tibemsDestination &dest)
{
	tibemsMsgRequestor requestor;
	tibems_status status = tibemsMsgRequestor_Create(session, &requestor, dest);
	if (status != TIBEMS_OK)
		throw std::logic_error("Error creating requestor");
	return requestor;
}

tibemsMsgProducer TibcoSession::GetProducer(const std::string &destName, const tibemsDestination &dest)
{
	size_t numProducers = producers.size();
	ProducersMap::const_iterator found = producers.find(destName);
	if (found != producers.end())
		return found->second;

	tibemsMsgProducer producer;
	tibems_status status = tibemsSession_CreateProducer(session, &producer, dest);
	if (status != TIBEMS_OK)
		throw std::logic_error(boost::lexical_cast<string>(format("Error creating producer for %s destination: %s") % destName % tibemsStatus_GetText(status)));

	producers[destName] = producer;
	return producer;
}

TibcoMessage::ptr_t TibcoSession::BasicConsumeMessage()
{
	tibemsMsg msgRequest;
	tibems_status status = TIBEMS_OK;
	status = tibemsMsgConsumer_Receive(consumer, &msgRequest);
	if (status != TIBEMS_OK)
		throw std::logic_error(boost::lexical_cast<string>(format("Error receiving message: %s") % tibemsStatus_GetText(status)));

	return TibcoMessage::Create(msgRequest);
}

void TibcoSession::BasicPublish(const tibemsDestination &dest, TibcoMessage::ptr_t msgResponse)
{
	tibems_status status = TIBEMS_OK;
	char destName[512];

	status = tibemsDestination_GetName(dest, destName, sizeof(destName));
	if (status != TIBEMS_OK)
		throw std::logic_error(boost::lexical_cast<string>(format("Error getting destination name: %s") % tibemsStatus_GetText(status)));

	status = tibemsMsgProducer_Send(GetProducer(std::string(destName), dest), msgResponse->Message());
	if (status != TIBEMS_OK)
	{
		// Si el desti ja no existeix, l'eliminam del unordered_map
		if (status == TIBEMS_INVALID_DESTINATION)
		{
			std::string queue_name(destName);
			ProducersMap::const_iterator found = producers.find(queue_name);
			if (found != producers.end())
			{
				tibemsMsgProducer_Close(found->second);
				producers.erase(found);
			}
		}
		throw std::logic_error("Error publish message");
	}
}

TibcoSession::~TibcoSession()
{
	foreach(const ProducersMap::value_type &producer, producers)
	{
		tibemsMsgProducer_Close(producer.second);
	}

	if (consumer != NULL)
		tibemsMsgConsumer_Close(consumer);

	tibemsSession_Close(session);
}