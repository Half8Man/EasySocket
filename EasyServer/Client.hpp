#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "HeaderFile.h"
#include "CommonDef.h"

class Client
{
public:
	explicit Client(SOCKET sock)
		: client_sock_(sock), recv_last_pos_(0), send_last_pos_(0)
	{
		memset(recv_data_buffer_, 0, sizeof(recv_data_buffer_));
		memset(send_data_buffer_, 0, sizeof(send_data_buffer_));
	}

	virtual ~Client() = default;

	inline char *GetRecvDataBuffer()
	{
		return recv_data_buffer_;
	}

	inline char *GetSendDataBuffer()
	{
		return send_data_buffer_;
	}

	inline void SetRecvLastPos(int pos)
	{
		recv_last_pos_ = pos;
	}

	inline int GetRecvLastPos() const
	{
		return recv_last_pos_;
	}

	inline void SetSendLastPos(int pos)
	{
		send_last_pos_ = pos;
	}

	inline int GetSendLastPos() const
	{
		return send_last_pos_;
	}

	inline SOCKET GetSock() const
	{
		return client_sock_;
	}

	// 定时定量发送数据
	int SendData(DataHeader *data)
	{
		int ret = SOCKET_ERROR;

		if (data)
		{
			// 要发送的数据长度
			int send_data_len = data->data_len;
			// 要发送的数据
			const char *send_data = (const char *)data;

			while (true)
			{
				if (send_last_pos_ + send_data_len >= kSendBufferSize)
				{
					// 计算可以拷贝的数据长度
					int copy_len = kSendBufferSize - send_last_pos_;

					// 拷贝数据
					memcpy(send_data_buffer_ + send_last_pos_, send_data, copy_len);

					// 计算剩余数据位置
					send_data += copy_len;

					// 计算剩余数据长度
					send_data_len -= copy_len;

					// 发送数据
					ret = send(client_sock_, send_data_buffer_, kSendBufferSize, 0);

					// 数据尾部位置置零
					send_last_pos_ = 0;

					if (ret == SOCKET_ERROR)
					{
						return ret;
					}
				}
				else
				{
					// 拷贝数据到发送缓冲区尾部
					memcpy(send_data_buffer_ + send_last_pos_, send_data, send_data_len);

					// 数据尾部位置偏移
					send_last_pos_ += send_data_len;

					break;
				}
			}
		}

		return ret;
	}

	void ResetHeartBeatDelay()
	{
		heart_beat_delay_ = 0;
	}

	bool CheckHeart(time_t dt)
	{
		heart_beat_delay_ += dt;

//		printf("heart_beat_delay_ == %I64d\n", heart_beat_delay_);

		return heart_beat_delay_ >= kClientDeadTime;
	}

private:
	SOCKET client_sock_ = INVALID_SOCKET;

	// 接收缓冲区
	char recv_data_buffer_[kRecvBufferSize] = {};

	// 发送缓冲区
	char send_data_buffer_[kSendBufferSize] = {};

	// 接收缓冲区的数据尾部位置
	int recv_last_pos_ = 0;

	// 发送缓冲区的数据尾部位置
	int send_last_pos_ = 0;

	// 心跳死亡计时
	time_t heart_beat_delay_ = 0;
};

#endif // !__CLIENT_H__