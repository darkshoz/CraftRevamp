#include "pch.h"

#include "TCPStream.h"

#include "Networking/TCPServer.h"

TCPStream::TCPStream(TCPServer* server)
	: m_Server(server), m_Closing(false)
{
}

TCPStream::~TCPStream()
{
	Stop();
}

void TCPStream::Start(std::shared_ptr<uvw::TCPHandle>&& handle)
{
	m_Handle = std::move(handle);

	// Keep a reference in scope 
	auto thisRef = shared_from_this();
	m_Handle->on<uvw::DataEvent>([thisRef](const uvw::DataEvent& e, uvw::TCPHandle&)
		{
			if (thisRef->m_Closing)
				return;

			std::vector<uint8_t> data(e.length);
			std::memcpy(&data[0], e.data.get(), e.length);

			thisRef->OnDataReceived(std::move(data));
		}
	);

	m_Handle->on<uvw::EndEvent>([thisRef](const uvw::EndEvent&, uvw::TCPHandle&)
		{
			thisRef->Stop();
		}
	);

	m_Handle->noDelay(true);
	m_Handle->read();
}

void TCPStream::Stop()
{
	if (m_Closing)
		return;

	m_Closing = true;

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

	auto thisRef = shared_from_this();
	m_Server->RemoveStream(thisRef);

	OnDisconnect();
}

void TCPStream::Write(std::vector<uint8_t>&& data)
{
	// TODO: write timeout handling
	m_Handle->write((char*)data.data(), (uint32_t)data.size());
}
