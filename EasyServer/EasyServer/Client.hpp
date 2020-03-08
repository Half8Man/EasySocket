#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "HeaderFile.h"
#include "CommonDef.h"

class Client
{
public:
	Client(SOCKET sock)
		:client_sock_(sock), last_pos_(0)
	{
		memset(data_buffer_, 0, sizeof(data_buffer_));
	}

	virtual ~Client()
	{
	}

	inline char* GetDataBuffer()
	{
		return data_buffer_;
	}

	inline void SetLastPos(int pos)
	{
		last_pos_ = pos;
	}

	inline int GetLastPos() const
	{
		return last_pos_;
	}

	inline SOCKET GetSock() const
	{
		return client_sock_;
	}

	// ·¢ËÍÊý¾Ý
	int SendData(DataHeader* data)
	{
		if (data)
			return send(client_sock_, (const char*)data, data->data_len, 0);

		return SOCKET_ERROR;
	}

private:
	SOCKET client_sock_ = INVALID_SOCKET;

	char data_buffer_[kBufferSize * 10] = {};

	int last_pos_ = 0;
};

#endif // !__CLIENT_H__