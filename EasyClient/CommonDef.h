#ifndef __COMMON_DEF_H__
#define __COMMON_DEF_H__

// 数据缓冲区最小单元大小
const int kBufferSize = 10240;

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
	LoginData() : DataHeader()
	{
		data_len = sizeof(LoginData);
		cmd = Cmd::kCmdLogin;
	}

	char user_name[64] = {};
	char password[64] = {};
};

struct LoginRetData : public DataHeader
{
	LoginRetData() : DataHeader()
	{
		data_len = sizeof(LoginRetData);
		cmd = Cmd::kCmdLoginRet;
		ret = 0;
	}

	int ret;
};

struct LogoutData : public DataHeader
{
	LogoutData() : DataHeader()
	{
		data_len = sizeof(data_len);
		cmd = Cmd::kCmdLogout;
	}

	char user_name[64] = {};
};

struct LogoutRetData : public DataHeader
{
	LogoutRetData() : DataHeader()
	{
		data_len = sizeof(LogoutRetData);
		cmd = Cmd::kCmdLogoutRet;
		ret = 0;
	}

	int ret;
};

struct NewUserJoinData : public DataHeader
{
	NewUserJoinData() : DataHeader()
	{
		data_len = sizeof(NewUserJoinData);
		cmd = Cmd::kCmdNewUserJoin;
	}

	int sock = 0;
};

struct HeartBeatC2sData : public DataHeader
{
	HeartBeatC2sData() : DataHeader()
	{
		data_len = sizeof(HeartBeatC2sData);
		cmd = Cmd::kCmdHeartBeatC2s;
	}
};

struct HeartBeatS2cData : public DataHeader
{
	HeartBeatS2cData() : DataHeader()
	{
		data_len = sizeof(HeartBeatS2cData);
		cmd = Cmd::kCmdHeartBeatS2c;
	}
};

#endif // !__COMMON_DEF_H__