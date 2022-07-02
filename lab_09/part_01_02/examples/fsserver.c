#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>

#define SOCK_NAME "socket.soc"

// struct sockaddr
// {
// 	unsigned short sa_family; // Семейство адресов, AF_xxx
// 	char sa_data[14];		  // 14 байтов для хранения адреса
// };

// Файлы сокетов - это файлы, обеспечивающие прямую связь между процессами, они могут
// передавать информацию между процессами, запущенными в разных средах или даже разных машинах.

// Прежде, чем передавать данные через сокент,
// его необходимо связать с адресом в выбранном
// домене - это именование сокета.

// socket() - создает "безымянный" сокет. Получает дескриптор сокета.
// AF_UNIX - домен. (cемейство сокетов) эффективное взаимодействия между процессами на одной машине.
// AF_UNIX - это семейство адресов файловых сокетов Unix.
// AF_UNIX - сокеты в файловом пространстве имен.
// Домен сокета обозначает тип соединения.

// SOCK_DGRAM - датаграмный сокет. Хранит границы сообщений.
// 0 - протокол, используемый для передачи данных.

// bind() - связывает сокет с заданным адресом.
// Связывать сокет с адресом необходимо в программе-сервере, а не в клиенте.
// Параметры:
// 1. дескриптор;
// 2. указатель на структуру sockaddr, содержащую адрес, на котором регистрируется сервер;
// 3. длина структуры, содержащей адрес;

// sa_data - имя файла сокета.

// После вызова bind() программа-сервер становится доступна для соединения по заданному адресу (имени файла)

int main()
{
	struct sockaddr srvr_name;
	struct sockaddr rcvr_name; // Данные об адресе клиента.
	int namelen;			   // Длина возвращаемой структуры с адресом.
	char buf[32];

	int sock = socket(AF_UNIX, SOCK_DGRAM, 0);
	if (sock < 0)
	{
		perror("socket failed");
		return -1;
	}

	srvr_name.sa_family = AF_UNIX;
	strcpy(srvr_name.sa_data, SOCK_NAME);
	if (bind(sock, &srvr_name, strlen(srvr_name.sa_data) + sizeof(srvr_name.sa_family)) < 0)
	{
		perror("bind failed");
		return -1;
	}

	// блокирует программу до тех пор, пока на входе не появятся новые данные.
	int bytes = recvfrom(sock, buf, sizeof(buf), 0, &rcvr_name, &namelen);

	printf("Получено: %s\nlen = %ld", buf, strlen(buf));

	close(sock);
	unlink(SOCK_NAME);

	return 0;
}