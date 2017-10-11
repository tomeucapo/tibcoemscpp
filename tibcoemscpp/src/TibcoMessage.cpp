#include "stdafx.h"
#include "TibcoMessage.h"

using namespace TibcoEMSClient;

TibcoMessage::TibcoMessage()
{
	if (tibemsMsg_Create(&message) != TIBEMS_OK)
		std::logic_error("Error creating generic message");
}

TibcoMessage::TibcoMessage(const tibemsMsg &msg)
{
	this->message = msg;
}

TibcoMessage::TibcoMessage(const std::string &text)
{
	if (tibemsTextMsg_Create(&message) != TIBEMS_OK)
		std::logic_error("Error creating text message");

	if (tibemsTextMsg_SetText(message, text.c_str()) != TIBEMS_OK)
		std::logic_error("Error writting bytes to message");
}

std::string TibcoMessage::Body()
{
	tibemsMsgType msgType;
	tibems_status status = tibemsMsg_GetBodyType(message, &msgType);
	if (status != TIBEMS_OK)
		return std::string();

	if (msgType != TIBEMS_TEXT_MESSAGE)
		return std::string();

	const char *text = NULL;
	status = tibemsTextMsg_GetText(message, &text);
	return std::string(text);
}

tibemsDestination TibcoMessage::ReplyTo() const
{
	tibemsDestination responseDestination;
	if (tibemsMsg_GetReplyTo(message, &responseDestination) != TIBEMS_OK)
		std::logic_error("Error while getting reply-to destination");

	return responseDestination;
}

void TibcoMessage::ReplyTo(const tibemsDestination &replyDest)
{
	tibemsMsg_SetReplyTo(message, replyDest);
}

void TibcoMessage::CorrelationId(const std::string &correlation_id)
{
	if (tibemsMsg_SetCorrelationID(message, correlation_id.c_str()) != TIBEMS_OK)
		std::logic_error("Error while setting correlation ID to response message");
}

bool TibcoMessage::CorrelationIdIsSet() const
{
	const char *correlationId = NULL;
	if (tibemsMsg_GetCorrelationID(message, &correlationId) != TIBEMS_OK)
		return false;
	return (correlationId != NULL);
}

std::string TibcoMessage::CorrelationId() const
{
	const char *correlationId = NULL;
	if (tibemsMsg_GetCorrelationID(message, &correlationId) != TIBEMS_OK)
		return std::string();
	return std::string(correlationId);
}

TibcoMessage::~TibcoMessage()
{
	tibemsMsg_Destroy(message);
}