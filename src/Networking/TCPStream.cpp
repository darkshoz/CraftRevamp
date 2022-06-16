#include "pch.h"

#include "TCPStream.h"

#include "Networking/TCPServer.h"

TCPStream::TCPStream(TCPServer* server)
	: m_Server(server)
{
}

void TCPStream::Start(std::shared_ptr<uvw::TCPHandle>&& handle)
{
	m_Handle = std::move(handle);
	m_Handle->on<uvw::DataEvent>([this](const uvw::DataEvent& e, uvw::TCPHandle&)
		{
			std::vector<uint8_t> data(e.length);
			std::memcpy(&data[0], e.data.get(), e.length);

			OnDataReceived(std::move(data), (size_t)e.length);
		}
	);

	m_Handle->on<uvw::EndEvent>([this](const uvw::EndEvent&, uvw::TCPHandle&)
		{
			Stop();
		}
	);

	m_Handle->noDelay(true);
	m_Handle->read();
}

void TCPStream::Stop()
{
	auto handleRef = std::make_shared<decltype(m_Handle)>(m_Handle);
	auto closeHandle = [handleRef]()
	{
		(*handleRef)->once<uvw::CloseEvent>([handleRef](const uvw::CloseEvent&, uvw::TCPHandle&)
			{
				*handleRef = {};
			}
		);

		(*handleRef)->close();
	};

	m_Handle->once<uvw::ShutdownEvent>([closeHandle](const uvw::ShutdownEvent&, uvw::TCPHandle&)
		{
			closeHandle();
		}
	);

	// We still need to close the handle if any error occurs
	m_Handle->once<uvw::ErrorEvent>([closeHandle](const uvw::ErrorEvent&, uvw::TCPHandle&)
		{
			closeHandle();
		}
	);

	m_Handle->stop();
	m_Handle->shutdown();
	m_Handle = {};

	m_Server->RemoveClient(this);
}

void TCPStream::Write(std::unique_ptr<char>&& data, size_t size)
{
	// TODO: write timeout handling
	m_Handle->write(data.get(), size);
}
