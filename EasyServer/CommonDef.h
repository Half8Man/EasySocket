#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

// 接受数据缓冲区最小单元大小
const int kRecvBufferSize = 10240;

// 发送数据缓冲区最小单元大小
const int kSendBufferSize = 10240;

// cell server 数量
const int kCellServerCount = 4;

// 死亡时间 毫秒
const int kClinetDeadTime = 500;

enum Cmd
{
	kCmdHeartBeatC2s,
	kCmdHeartBeatS2c,
	kCmdLogin,
	kCmdLoginRet,
	kCmdLogout,
	kCmdLogoutRet,
	kCmdNewUserJoin,
	kCmdError
};

struct DataHeader
{
	int data_len;
	int cmd;
};

// DataPackage
struct LoginData : public DataHeader
{
	LoginData()
	{
		data_len = sizeof(LoginData);
		cmd = Cmd::kCmdLogin;
	}

	char user_name[64] = {};
	char password[64] = {};
};

struct LoginRetData : public DataHeader
{
	LoginRetData()
	{
		data_len = sizeof(LoginRetData);
		cmd = Cmd::kCmdLoginRet;
		ret = 0;
	}

	int ret;
};

struct LogoutData : public DataHeader
{
	LogoutData()
	{
		data_len = sizeof(data_len);
		cmd = Cmd::kCmdLogout;
	}

	char user_name[64] = {};
};

struct LogoutRetData : public DataHeader
{
	LogoutRetData()
	{
		data_len = sizeof(LogoutRetData);
		cmd = Cmd::kCmdLogoutRet;
		ret = 0;
	}

	int ret;
};

struct NewUserJoinData : public DataHeader
{
	NewUserJoinData()
	{
		data_len = sizeof(NewUserJoinData);
		cmd = Cmd::kCmdNewUserJoin;
	}

	int sock = 0;
};

struct HeartBeatC2sData : public DataHeader
{
	HeartBeatC2sData()
	{
		data_len = sizeof(HeartBeatC2sData);
		cmd = Cmd::kCmdHeartBeatC2s;
	}
};

struct HeartBeatS2cData : public DataHeader
{
	HeartBeatS2cData()
	{
		data_len = sizeof(HeartBeatS2cData);
		cmd = Cmd::kCmdHeartBeatS2c;
	}
};

#endif // !__COMMON_DEF_H__