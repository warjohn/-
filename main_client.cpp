#include <iostream>
#include <fstream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>

using namespace std;

void initializeSSL() {
    SSL_load_error_strings();   
    OpenSSL_add_ssl_algorithms();
}

void cleanupSSL() {
    EVP_cleanup();
}

int main() {
    initializeSSL();

    SSL_CTX* ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        cerr << "Не удалось создать SSL_CTX." << endl;
        return 1;
    }

    // Загрузите корневой сертификат (если необходимо)
    // if (SSL_CTX_load_verify_locations(ctx, "ca.crt", nullptr) <= 0) {
    //     cerr << "Не удалось загрузить корневой сертификат." << endl;
    //     SSL_CTX_free(ctx);
    //     return 1;
    // }

    int clientSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket == -1) {
        cerr << "Не удалось создать сокет." << endl;
        SSL_CTX_free(ctx);
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1"); // Используйте IP-адрес сервера
    
    if (connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        cerr << "Ошибка соединения" << endl;
        close(clientSocket);
        SSL_CTX_free(ctx);
        return 1;
    }

    cout << "Соединение установлено" << endl;

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, clientSocket);
    
    if (SSL_connect(ssl) <= 0) {
        cerr << "Ошибка SSL_connect." << endl;
        SSL_free(ssl);
        close(clientSocket);
        SSL_CTX_free(ctx);
        return 1;
    }

    // Открываем файл для чтения
    ifstream file("client_file.txt", ios::binary);
    if (!file.is_open()) {
        cerr << "Не удалось открыть файл." << endl;
        SSL_free(ssl);
        close(clientSocket);
        SSL_CTX_free(ctx);
        return 1;
    }
    
    char buffer[1024];
    while (!file.eof()) {
        file.read(buffer, sizeof(buffer));  // Читаем данные из файла
        int bytesRead = file.gcount();      // Получаем количество считанных байтов
        if (bytesRead > 0) {
            int bytesSent = 0;
            while (bytesSent < bytesRead) {
                int result = SSL_write(ssl, buffer + bytesSent, bytesRead - bytesSent);  // Отправляем данные на сервер
                if (result <= 0) {
                    cerr << "Ошибка при отправке данных." << endl;
                    SSL_free(ssl);
                    close(clientSocket);
                    SSL_CTX_free(ctx);
                    return 1;
                }
                bytesSent += result;  // Увеличиваем количество отправленных байтов
            }
        }
    }

    file.close();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(clientSocket);
    SSL_CTX_free(ctx);
    cleanupSSL();

    cout << "Команда завершила выполнение!" << endl;
    return 0;
}
