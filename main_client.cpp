#include <iostream>
#include <fstream>
#include "sys/socket.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;



int main() {

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    
    connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    
    if (connect == 0){
        cout <<  "Соединение установленно" << endl;
    }
    else {
        cout <<  "Ошибка соединения" << endl;
    }
    
    // Открываем файл для чтения
    ifstream file("client_file.txt", ios::binary);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл." << endl;
    }

    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));  // Читаем данные из файла
        cout << buffer << endl;
        int bytesRead = file.gcount();      // Получаем количество считанных байтов
        cout << bytesRead << endl;
        send(clientSocket, buffer, bytesRead, 0);  // Отправляем данные на сервер
    }

    //const char* message = "Hello, server!";
    //send(clientSocket, message, strlen(message), 0);
    file.close();
    close(clientSocket);
    return 0;
}   