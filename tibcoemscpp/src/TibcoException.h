#pragma once


namespace TibcoEMSClient
{
	class TibcoException :
		public std::runtime_error
	{
	public:
		explicit TibcoException(const std::string &what, const std::string &reply_text, boost::uint16_t class_id, boost::uint16_t method_id) throw();
		virtual ~TibcoException() throw() {}

		virtual bool is_soft_error() const throw() = 0;
		virtual boost::uint16_t reply_code() const throw() = 0;
		virtual boost::uint16_t class_id() const throw()
		{
			return m_class_id;
		}
		virtual boost::uint16_t method_id() const throw()
		{
			return m_method_id;
		}
		virtual std::string reply_text() const throw()
		{
			return m_reply_text;
		}

	protected:
		std::string m_reply_text;
		boost::uint16_t m_class_id;
		boost::uint16_t m_method_id;
	};

	class ConnectionException : public TibcoException
	{
	public:
		explicit ConnectionException(const std::string &what, const std::string &reply_text, boost::uint16_t class_id, boost::uint16_t method_id) throw() :
			TibcoException(what, reply_text, class_id, method_id) {}

		virtual bool is_soft_error() const throw()
		{
			return false;
		}
	};
}