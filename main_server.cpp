#include <iostream>
#include <fstream>
#include "sys/socket.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

using namespace std;

int main() {

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    
    /*
    socketfd: Это файловый дескриптор для сокета.
    AF_INET: Указывает семейство протоколов IPv4.
    SOCK_STREAM: Определяет тип сокета TCP.
    */

    if (serverSocket == -1) {
        cerr << "Не удалось создать сокет." << endl;
        return 1;
    }

    sockaddr_in serverAddress; //Это тип данных, используемый для хранения адреса сокета.
    serverAddress.sin_family = AF_INET;  // Указываем семейство адресов (IPv4)
    serverAddress.sin_port = htons(8080);  /* Указываем порт (с преобразованием в сетевой порядок байт) Эта функция используется для преобразования unsigned int из машинного порядка байтов в сетевой порядок.*/
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);  //  Это используется, когда мы не хотим привязывать сокет к определенному IP-адресу, а вместо этого хотим прослушивать все доступные IP-адреса.
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)); //Затем мы связываем сокет с помощью вызова bind(),
    listen(serverSocket, 5); //Затем мы сообщаем приложению прослушивать сокет, на который ссылается serverSocket

    int clientSocket = accept(serverSocket, nullptr, nullptr); //Вызов accept() используется для принятия запроса на соединение, полученного на сокете, который прослушивало приложение

    ofstream file("received_file.txt", ios::binary);
    if (!file.is_open()) {
        cerr << "Не удалось создать файл." << endl;
    }

    char buffer[1024];
    int bytesReceived;
    while ((bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0)) > 0) {
        cout << "\tДанные передачи: " << buffer << endl;
        file.write(buffer, bytesReceived);  // Записываем полученные данные в файл
    }


    //char buffer[1024] = {0};
    //recv(clientSocket, buffer, sizeof(buffer), 0);
    //cout << "Message from client: " << buffer << endl;
    /*
    Затем мы начинаем получать данные от клиента. 
    Мы можем указать необходимый размер буфера, чтобы в нем было достаточно места для приема данных, отправляемых клиентом
    */
    file.close();
    close(serverSocket);

    cout << "Команда завершила выполнение!" << endl;
}