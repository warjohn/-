#include <iostream>
#include <fstream>
#include <sstream>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iomanip>
#include <stdexcept>
#include <filesystem>
#include <chrono>
#include <vector>
#include <unordered_map>
#include <functional>
#include <cstring>
#include <cstdlib>


class Client { 

    public: 

        Client(int serverPort, const std::string& serverAddress, std::string& dir)
            : Port(serverPort), Address(serverAddress), directory(dir) {
            if (serverAddress.empty() || dir.empty()) {
                throw std::invalid_argument("Server address or directory is empty");
            }
        }


        void run_loop() { 
            int socket_first = CreateNConnectSocket();
            ReceiveInChunks(socket_first);
            DeserializeFiles();
            GetRecentFiles();
            close(socket_first);
            std::cout << "First part is done!" << std::endl;
            int socket_second = CreateNConnectSocket();
            SendAndClose(socket_second);\
            std::cout << "Second part is done!" << std::endl;

        }

    private: 

        int Port;
        const std::string Address;
        std::string directory;
        char buffer[16384];
        std::string receivedString;
        std::vector<char> receivedData;
        std::vector<std::vector<std::string>> files_info;
        std::string line;
        std::vector<std::filesystem::path> recent_files;
        std::unordered_map<std::string, std::uintmax_t> server_files;
        

        int CreateNConnectSocket() { 
            struct sockaddr_in serverAddr;
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(Port);
            inet_pton(AF_INET, Address.c_str(), &serverAddr.sin_addr);

            // Создание сокета
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) {
                perror("Socket creation failed");
                return -1;
            }

            // Подключение к серверу
            if (connect(sock, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                perror("Connection failed");
                close(sock);
                return -1;
            }
            std::cout << "sock " << sock << std::endl;
            return sock;

        }

        void ReceiveInChunks(int& clientSocket) {
            while (true) {
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                std::cout << "bytesReceived - \t" << bytesReceived << std::endl;
                if (bytesReceived == 0) {
                    break;
                }
                for (int i = 0; i < bytesReceived; ++i) {
                    receivedData.push_back(buffer[i]);
                }
            }
            receivedData.push_back('\0'); 
            receivedString = std::string(receivedData.begin(), receivedData.end() - 1);
            std::cout << "receivedData size - \t" << receivedData.size() << std::endl;
        }

        void DeserializeFiles() {
            std::istringstream iss(receivedString);
            while (std::getline(iss, line)) {   
                std::istringstream line_stream(line);
                std::string file_path, file_size;

                if (std::getline(line_stream, file_path, '|') && std::getline(line_stream, file_size)) {
                    files_info.push_back({file_path, file_size});
                }
            }
            std::cout << "vector size --- " << files_info.size() << std::endl;  
        }

        void GetRecentFiles() {
            for (const auto& file_info : files_info) {
                if (file_info.size() == 2) {
                    server_files[file_info[0]] = std::stoull(file_info[1]);
                }
            }
            std::filesystem::path dir_path(directory);
            if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory)) {
                throw std::runtime_error("Invalid directory: " + dir_path.string());
            }
            // Рекурсивная обработка содержимого
            for (const auto& entry : std::filesystem::recursive_directory_iterator(directory)) {
                if (std::filesystem::is_regular_file(entry)) {
                    std::string file_name = entry.path().string();

                    std::string short_path  = file_name;
                    std::string prefix_to_remove = dir_path.string() + "/"; 

                    auto pos = short_path.find(prefix_to_remove);
                    if (pos != std::string::npos) {
                        short_path.erase(pos, prefix_to_remove.length());
                    }
                    std::uintmax_t file_size;
                    try {
                        file_size = std::filesystem::file_size(entry);
                    } catch (const std::filesystem::filesystem_error& e) {
                        std::cerr << "Error reading file size for " << entry.path() << ": " << e.what() << std::endl;
                        continue;
                    }

                    auto it = server_files.find(short_path);
                    if (it == server_files.end()) {
                        std::cout << "Файл не найден на сервере: " << short_path << std::endl;
                        recent_files.push_back(entry.path());
                    } else if (it->second != file_size) {
                        std::cout << "Изменён файл: " << short_path
                                << " (локальный размер: " << file_size
                                << ", серверный размер: " << it->second << ")" << std::endl;
                        recent_files.push_back(entry.path());
                    }
                }
                
            };
        }

        std::string hashFile(const std::string& filePath) {
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256_CTX sha256;
            SHA256_Init(&sha256);

            std::ifstream file(filePath, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Unable to open file: " + filePath);
            }

            char buffer[1024];
            while (file.read(buffer, sizeof(buffer))) {
                SHA256_Update(&sha256, buffer, sizeof(buffer));
            }
            SHA256_Update(&sha256, buffer, file.gcount());

            SHA256_Final(hash, &sha256);

            std::ostringstream oss;
            for (const auto& byte : hash) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
            }
            return oss.str();
        }

        std::string hashString(const std::string& data) {
            unsigned char hash[SHA256_DIGEST_LENGTH];
            SHA256_CTX sha256;
            SHA256_Init(&sha256);
            SHA256_Update(&sha256, data.c_str(), data.size());
            SHA256_Final(hash, &sha256);

            std::ostringstream oss;
            for (const auto& byte : hash) {
                oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
            }
            return oss.str();
        }

        void sendInChunks(int sock, const std::string &final_string, size_t chunkSize) {
            size_t totalSent = 0;
            size_t stringSize = final_string.size();

            while (totalSent < stringSize) {
                size_t remaining = stringSize - totalSent;
                size_t toSend = std::min(chunkSize, remaining);
                
                int bytesSent = send(sock, final_string.c_str() + totalSent, toSend, 0);
                if (bytesSent < 0) {
                    perror("Error sending data");
                    return;
                }
                totalSent += bytesSent;
            }
        }

        void sendFileInfo(int sock, const std::filesystem::path& path) {
            std::string filename = path.filename().string();
            std::size_t name_byte_size = filename.size();
            std::size_t chunkSize = 1024;

            // Чтение текста файла
            std::ifstream file(path, std::ios::binary);
            if (!file.is_open()) {
                throw std::runtime_error("Unable to open file: " + path.string());
            }

            std::ostringstream text_stream;
            text_stream << file.rdbuf();
            std::string text = text_stream.str();

            // Вычисление хеша текста
            std::string text_hash = hashFile(path.string());

            // Создание итоговой строки
            std::string final_string = filename + ';' + text + ';' + std::to_string(name_byte_size) + ';' + text_hash + ';' + path.string() + ';';
            
            // Хеш всей строки
            std::string filename_hash = hashString(filename);
            std::string final_text_hash = hashString(text);
            std::string name_byte_size_hash = hashString(std::to_string(name_byte_size));
            std::string final_text_hash_hash = hashString(text_hash);

            final_string += filename_hash; // Добавляем хеш всей строки в конец
            final_string += final_text_hash;
            final_string += name_byte_size_hash;
            final_string += final_text_hash_hash + ';';
            
            // Отправка строки на сервер
            sendInChunks(sock, final_string, chunkSize);
        }

        void SendAndClose(int& clientSocket) { 
            try {
                for (const auto& path : recent_files) {
                    sendFileInfo(clientSocket, path); 
                }
            } catch (const std::exception& e) {
                std::cerr << e.what() << std::endl;
            } 
            close(clientSocket);
        }
};



int main(int argc, char* argv[]) {

    std::string directory = argv[1];
    const std::string serverAddress = argv[2]; // Адрес сервера
    const int serverPort = atoi(argv[3]); // Порт сервера

    Client client(serverPort, serverAddress, directory); 
    client.run_loop();

    return 0;
}

