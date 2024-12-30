#pragma once
#include <asio.hpp>
#include "Networking/Utilities.h"

namespace Core
{
	class AsioContext : public Context
	{
		friend class Session;
	public:
		AsioContext(asio::io_context& Context) : context(Context) {}
	private:
		asio::io_context& context;
	};

	class AsioSocket : public Socket
	{
		friend class Session;
	public:
		AsioSocket(asio::ip::tcp::socket& Socket) : socket(Socket) {}
	private:
		asio::ip::tcp::socket& socket;
	};
}