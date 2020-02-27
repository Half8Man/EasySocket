#include "EasyTcpClient.hpp"

const std::string kIp = "127.0.0.1";
const int kPort = 1234;

bool can_run = true;
void DealInput(EasyTcpClient* client)
{
	while (true)
	{
		std::string input;
		std::cout << "请输入命令：" << std::endl;
		std::cin >> input;

		if (input == "login")
		{
			LoginData login_data;
			strcpy(login_data.user_name, "wangjunhe");
			strcpy(login_data.password, "123456");
			client->SendData(&login_data, login_data.data_len);
		}
		else if (input == "logout")
		{
			LogoutData logout_data;
			strcpy(logout_data.user_name, "wangjunhe");
			client->SendData(&logout_data, logout_data.data_len);
		}
		else if (input == "exit")
		{
			printf("任务结束\n");
			can_run = false;
			break;
		}
		else
		{
			printf("错误的命令，请重新输入\n");
		}
	}
}

int main()
{
	EasyTcpClient client;
	client.InitSocket();
	client.Connect(kIp.c_str(), kPort);

	std::thread input_thread(DealInput, &client);
	input_thread.detach();

	while (client.IsAlive() && can_run)
	{
		if (client.OnRequest() < 0)
		{
			break;
		}
	}

	client.Close();

	return 0;
}