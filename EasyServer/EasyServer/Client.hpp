#ifndef __CLIENT_H__
#define __CLIENT_H__

#include "HeaderFile.h"
#include "CommonDef.h"

class Client
{
public:
	Client(SOCKET sock)
		:client_sock_(sock), recv_last_pos_(0), send_last_pos_(0)
	{
		memset(recv_data_buffer_, 0, sizeof(recv_data_buffer_));
		memset(send_data_buffer_, 0, sizeof(send_data_buffer_));
	}

	virtual ~Client()
	{
	}

	inline char* GetRecvDataBuffer()
	{
		return recv_data_buffer_;
	}

	inline char* GetSendDataBuffer()
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

	// ��ʱ������������
	int SendData(DataHeader* data)
	{
		int ret = SOCKET_ERROR;

		if (data)
		{
			// Ҫ���͵����ݳ���
			int send_data_len = data->data_len;
			// Ҫ���͵�����
			const char* send_data = (const char*)data;

			while (true)
			{
				if (send_last_pos_ + send_data_len >= kSendBufferSize)
				{
					// ������Կ��������ݳ���
					int copy_len = kSendBufferSize - send_last_pos_;

					// ��������
					memcpy(send_data_buffer_ + send_last_pos_, send_data, copy_len);

					// ����ʣ������λ��
					send_data += copy_len;

					// ����ʣ�����ݳ���
					send_data_len -= copy_len;

					// ��������
					ret = send(client_sock_, send_data_buffer_, kSendBufferSize, 0);

					// ����β��λ������
					send_last_pos_ = 0;

					if (ret == SOCKET_ERROR)
					{
						return ret;
					}
				}
				else
				{
					// �������ݵ����ͻ�����β��
					memcpy(send_data_buffer_ + send_last_pos_, send_data, send_data_len);

					// ����β��λ��ƫ��
					send_last_pos_ += send_data_len;

					break;
				}
			}
		}

		return ret;
	}

private:
	SOCKET client_sock_ = INVALID_SOCKET;

	// ���ջ�����
	char recv_data_buffer_[kRecvBufferSize] = {};

	// ���ͻ�����
	char send_data_buffer_[kSendBufferSize] = {};

	// ���ջ�����������β��λ��
	int recv_last_pos_ = 0;

	// ���ͻ�����������β��λ��
	int send_last_pos_ = 0;
};

#endif // !__CLIENT_H__