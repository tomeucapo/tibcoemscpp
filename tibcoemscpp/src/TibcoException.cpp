#include "stdafx.h"
#include "TibcoException.h"

using namespace TibcoEMSClient;

TibcoException::TibcoException(const std::string &what, const std::string &reply_text, boost::uint16_t class_id, boost::uint16_t method_id) throw() :
	std::runtime_error(what),
	m_reply_text(reply_text),
	m_class_id(class_id),
	m_method_id(method_id)
{}
