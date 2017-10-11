#pragma once
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include "TibcoConnection.h"

#ifdef _MSC_VER
# pragma warning ( push )
# pragma warning ( disable: 4275 4251 )
#endif

namespace TibcoEMSClient
{
	class SimpleRpcServer : boost::noncopyable
	{
	public:
		typedef boost::shared_ptr<SimpleRpcServer> ptr_t;

		friend ptr_t boost::make_shared<SimpleRpcServer>(TibcoEMSClient::TibcoSession::ptr_t const &a1, std::string const &a2);

		static ptr_t Create(TibcoSession::ptr_t mySession, const std::string &rpc_name = "")
		{
			return boost::make_shared<SimpleRpcServer>(mySession, rpc_name);
		}

	public:
		explicit SimpleRpcServer(TibcoSession::ptr_t mySession, const std::string &rpc_name);
		virtual ~SimpleRpcServer();

		std::string GetRpcName() const
		{
			return m_incoming_tag;
		}

		bool GetNextIncomingMessage(TibcoMessage::ptr_t &nextMsg);

		void RespondToMessage(TibcoMessage::ptr_t request, TibcoMessage::ptr_t response);
		void RespondToMessage(TibcoMessage::ptr_t request, const std::string response);

	private:
		TibcoSession::ptr_t session;
		const std::string m_incoming_tag;
	};
}

#ifdef _MSC_VER
# pragma warning ( pop )
#endif
