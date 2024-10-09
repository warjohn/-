#include <iostream>
#include <fstream>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

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

    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) {
        cerr << "Не удалось создать SSL_CTX." << endl;
        return 1;
    }

    // Загрузите сертификат и ключ
    if (SSL_CTX_use_certificate_file(ctx, "server.crt", SSL_FILETYPE_PEM) <= 0) {
        cerr << "Не удалось загрузить сертификат." << endl;
        SSL_CTX_free(ctx);
        return 1;
    }

    if (SSL_CTX_use_PrivateKey_file(ctx, "server.key", SSL_FILETYPE_PEM) <= 0) {
        cerr << "Не удалось загрузить закрытый ключ." << endl;
        SSL_CTX_free(ctx);
        return 1;
    }

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1) {
        cerr << "Не удалось создать сокет." << endl;
        SSL_CTX_free(ctx);
        return 1;
    }

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 5);

    int clientSocket = accept(serverSocket, nullptr, nullptr);
    if (clientSocket < 0) {
        cerr << "Не удалось принять соединение." << endl;
        close(serverSocket);
        SSL_CTX_free(ctx);
        return 1;
    }

    SSL* ssl = SSL_new(ctx);
    SSL_set_fd(ssl, clientSocket);
    
    if (SSL_accept(ssl) <= 0) {
        cerr << "Ошибка SSL_accept." << endl;
        SSL_free(ssl);
        close(clientSocket);
        close(serverSocket);
        SSL_CTX_free(ctx);
        return 1;
    }

    ofstream file("received_file.txt", ios::binary);
    if (!file.is_open()) {
        cerr << "Не удалось создать файл." << endl;
        SSL_free(ssl);
        close(clientSocket);
        close(serverSocket);
        SSL_CTX_free(ctx);
        return 1;
    }

    char buffer[1024];
    int bytesReceived;
    while ((bytesReceived = SSL_read(ssl, buffer, sizeof(buffer))) > 0) {
        cout << "\tДанные передачи: " << buffer << endl;
        file.write(buffer, bytesReceived);
    }

    file.close();
    SSL_shutdown(ssl);
    SSL_free(ssl);
    close(clientSocket);
    close(serverSocket);
    SSL_CTX_free(ctx);
    cleanupSSL();

    cout << "Команда завершила выполнение!" << endl;
    return 0;
}
