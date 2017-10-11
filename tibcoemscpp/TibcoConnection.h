#pragma once
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/make_shared.hpp>
#include "TibcoMessage.h"

namespace TibcoEMSClient
{
	class TibcoConnection : boost::noncopyable
	{
	private:
		tibemsErrorContext gErrorContext;
		tibemsConnection connection;	

	public:
	    typedef boost::shared_ptr<TibcoConnection> ptr_t;

		static ptr_t Create(const std::string &url = "tcp://127.0.0.1:7222")
		{
			return boost::make_shared<TibcoConnection>(url);
		}

	public:
		explicit TibcoConnection(const std::string &url);
		virtual ~TibcoConnection();

		bool Start();
		void CreateSession(tibemsSession &session);
	};

	class TibcoSession : boost::noncopyable
	{
	private:
		tibemsErrorContext errorContext;
		tibemsSession session;
		tibemsMsgConsumer consumer;
		TibcoConnection::ptr_t conn;

		typedef boost::unordered_map<std::string, tibemsMsgProducer> ProducersMap;		
		ProducersMap producers;

	public:
	    typedef boost::shared_ptr<TibcoSession> ptr_t;	

		friend ptr_t boost::make_shared<TibcoSession>(TibcoConnection::ptr_t const &a1);

		static ptr_t Create(TibcoConnection::ptr_t conn)
		{
			return boost::make_shared<TibcoSession>(conn);
		}
	
	private:
		tibemsMsgProducer GetProducer(const std::string &destName, const tibemsDestination &dest);

	public:
		explicit TibcoSession(TibcoConnection::ptr_t myConn);
		virtual ~TibcoSession();

		tibemsDestination CreateTemporaryQueue() const;
		void BasicConsumer(const std::string &queue_name);
		void BasicProducer(const std::string &queue_name);
		tibemsMsgRequestor BasicRequestor(const tibemsDestination &dest);
		
		TibcoMessage::ptr_t BasicConsumeMessage();
		void BasicPublish(const tibemsDestination &dest, TibcoMessage::ptr_t msgResponse);
	};
}
