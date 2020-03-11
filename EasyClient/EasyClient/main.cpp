﻿#include <thread>
#include <atomic>

#include "CELLTimeStamp.hpp"
#include "EasyTcpClient.h"

const std::string kIp = "127.0.0.1";
const int kPort = 1234;

const int client_count = 1;
const int thread_count = 1;

bool can_run = true;

std::atomic_int send_count = 0;
std::atomic_int ready_count = 0;

EasyTcpClient* clients[client_count];

void DealInput(/*EasyTcpClient* client*/)
{

	//while (true)
	//{
	//	std::string input;
	//	std::cout << "请输入命令：" << std::endl;
	//	std::cin >> input;

	//	if (input == "login")
	//	{
	//		LoginData login_data;
	//		strcpy(login_data.user_name, "wangjunhe");
	//		strcpy(login_data.password, "123456");
	//		client->SendData(&login_data);
	//	}
	//	else if (input == "logout")
	//	{
	//		LogoutData logout_data;
	//		strcpy(logout_data.user_name, "wangjunhe");
	//		client->SendData(&logout_data);
	//	}
	//	else if (input == "exit")
	//	{
	//		printf("任务结束\n");
	//		can_run = false;
	//		break;
	//	}
	//	else
	//	{
	//		printf("错误的命令，请重新输入\n");
	//	}
	//}

	while (true)
	{
		std::string input;
		std::cin >> input;

		if (input == "exit")
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

void SendThread(int id)
{
	int begin = (id - 1) * (client_count / thread_count);
	int end = id * (client_count / thread_count);

	for (int i = begin; i < end; i++)
	{
		clients[i] = new EasyTcpClient;
	}

	for (int i = begin; i < end; i++)
	{
		clients[i]->InitSocket();
	}

	for (int i = begin; i < end; i++)
	{
		clients[i]->Connect(kIp.c_str(), kPort);
	}

	// 等待其他线程准备完毕
	ready_count++;
	while (ready_count < thread_count)
	{
		std::chrono::milliseconds time(10);
		std::this_thread::sleep_for(time);
	}

	LoginData login_data = {};
	strcpy(login_data.user_name, "wangjunhe");
	strcpy(login_data.password, "123456");

	while (can_run)
	{
		for (int i = begin; i < end; i++)
		{
			clients[i]->OnRun();
		}

		//// 休眠1000毫秒
		//std::chrono::milliseconds sleep_time(1);
		//std::this_thread::sleep_for(sleep_time);

		for (int i = begin; i < end; i++)
		{
			if (clients[i]->SendData(&login_data) != SOCKET_ERROR)
			{
				send_count++;
			}
		}
	}

	for (int i = begin; i < end; i++)
	{
		clients[i]->Close();
	}
}

int main()
{
	//EasyTcpClient client;
	//client.InitSocket();
	//client.Connect(kIp.c_str(), kPort);

	//std::thread input_thread(DealInput, &client);
	//input_thread.detach();

	//while (client.IsRun() && can_run)
	//{
	//	if (client.OnRun() < 0)
	//	{
	//		break;
	//	}
	//}

	//system("pause");

	//client.Close();

	// 启动输入线程
	std::thread input_thread(DealInput);
	input_thread.detach();

	// 启动发送线程
	for (int i = 0; i < thread_count; i++)
	{
		std::thread send_thread(SendThread, i + 1);
		send_thread.detach();
	}

	CELLTimeStamp time_stamp;

	while (can_run)
	{
		//auto time = time_stamp.GetElapsedSecond();
		//if (time >= 1.0)
		//{
		//	printf("thread<%d>, clients<%d>, time<%f>, send_count<%d>\n", thread_count, client_count, time, send_count.load());
		//	send_count = 0;
		//	time_stamp.Update();
		//}

		//Sleep(1);
	}

	system("pause");
	return 0;
}