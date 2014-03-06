#include "viewback_client.h"

#ifdef _WIN32
#include <winsock2.h>
#include <pthread.h>
#endif

#include <stdio.h>

int main()
{
#ifdef _WIN32
	WSADATA wsadata;
	if (WSAStartup(MAKEWORD(2,2), &wsadata) != 0)
		return 1;

#ifdef PTW32_STATIC_LIB
	pthread_win32_process_attach_np();
#endif
#endif

	CViewbackClient vb;

	if (!vb.Initialize())
		return 1;

	while (true)
	{
		vb.Update();
	}

#ifdef _WIN32
	WSACleanup();
#endif

	return 0;
}
