#pragma once
#include <boost/utility.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>

#ifdef __MVS__  /* TIBEMSOS_ZOS  */
#include <tibems.h>
#include <emsadmin.h>
#else
#include <tibems/tibems.h>
#include <tibems/emsadmin.h>
#endif

namespace TibcoEMSClient
{
	class TibcoMessage : boost::noncopyable
	{
	private:
		tibemsMsg message;

	public:
		typedef boost::shared_ptr<TibcoMessage> ptr_t;

	public:
		static ptr_t Create()
		{
			return boost::make_shared<TibcoMessage>();
		}

		static ptr_t Create(const std::string &body)
		{
			return boost::make_shared<TibcoMessage>(body);
		}

		static ptr_t Create(const tibemsMsg &msg)
		{
			return boost::make_shared<TibcoMessage>(msg);
		}

		TibcoMessage();
		TibcoMessage(const std::string &body);
		TibcoMessage(const tibemsMsg &msg);
		virtual ~TibcoMessage();

		std::string Body();
		tibemsDestination ReplyTo() const;
		void ReplyTo(const tibemsDestination &replyDest);

		std::string CorrelationId() const;
		void CorrelationId(const std::string &correlation_id);
		bool CorrelationIdIsSet() const;

		tibemsMsg Message() const { return message; };
	};
}