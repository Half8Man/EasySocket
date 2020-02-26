#ifndef __MESSAGE_HPP__
#define __MESSAGE_HPP__

enum Cmd
{
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

	char user_name[64];
	char password[64];
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

	char user_name[64];
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

	int sock;
};

#endif // !__MESSAGE_HPP__
