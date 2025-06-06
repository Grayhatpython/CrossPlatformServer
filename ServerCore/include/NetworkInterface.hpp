#pragma once

namespace servercore
{
	class INetworkObject;
	class INetworkEvent
	{
	public:
		INetworkEvent(NetworkEventType type)
			: _type(type)
		{

		}
		virtual ~INetworkEvent() = default;

	public:
		NetworkEventType				GetNetworkEventType() const { return _type; }

		void							SetOwner(std::shared_ptr<INetworkObject> owner) { _owner = owner; }
		std::shared_ptr<INetworkObject> GetOwner() { return _owner; }

	protected:
		NetworkEventType					_type = NetworkEventType::None;
		std::shared_ptr<INetworkObject>		_owner;
	};

	class INetworkObject : public std::enable_shared_from_this<INetworkObject>
	{
	public:
		virtual ~INetworkObject() = default;

	public:
    	virtual NetworkObjectType GetNetworkObjectType() const = 0;

#if defined(PLATFORM_WINDOWS)
		virtual HANDLE GetHandle()  = 0;
#elif defined(PLATFORM_LINUX)
		virtual FileDescriptor GetFileDescriptor() = 0;
#endif

#if defined(PLATFORM_WINDOWS)
		virtual void Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode, int32 numOfBytes = 0)  = 0;
#elif defined(PLATFORM_LINUX)
		virtual void Dispatch(INetworkEvent* networkEvent, bool succeeded, int32 errorCode)  = 0;
#endif
	};

	class INetworkDispatcher
	{
	public:
		virtual ~INetworkDispatcher() = default;

	public:
		virtual bool Register(std::shared_ptr<INetworkObject> networkObject)  = 0;
		virtual DispatchResult Dispatch(uint32 timeoutMs = TIMEOUT_INFINITE)  = 0;
		virtual void PostExitSignal()  = 0;
	};

	class SendBuffer;
	struct SendContext
	{
		std::shared_ptr<SendBuffer> sendBuffer;

#if defined(PLATFORM_WINDOWS)
		WSABUF wsaBuf{};
#elif defined(PLATFORM_LINUX)
		struct iovec iovecBuf {};
#endif
	};

}