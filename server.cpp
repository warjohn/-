#include <iostream>
#include <cstring>
#include <string>
#include <sstream>
#include <openssl/sha.h>
#include <fstream>
#include <netinet/in.h>
#include <unistd.h>
#include <iomanip>
#include <chrono>
#include <vector>
#include <filesystem>
#include <pstl/glue_algorithm_defs.h>

namespace fs = std::filesystem;


class Server { 
    public:
        Server(const int& serverPort, int serverSocket, int clientSocket, const std::filesystem::path& dir)
            : Port(serverPort), serverSocket(serverSocket), clientSocket(clientSocket), directory(dir) {
            if (dir.empty()) {
                throw std::invalid_argument("Server address or directory is empty");
            }
        }

        void StarteventLoop() { 
            while (true) { 
                InitServer();
                GetFilesWithSizes();
                SerializeFiles();
                AcceptClient();
                sendInChunks();
                std::cout << "server have sent all files - connection close" << std::endl;
                if (shutdown(clientSocket, 2) < 0) {
                    throw std::runtime_error("Socker was not closed");
                }
                Listen();
                AcceptClient_second();
                handleClient();
                close(clientSocket_second);
            }
            close(serverSocket);
        }

    private: 

        const int Port;
        int serverSocket;
        int clientSocket;
        int clientSocket_second;
        const std::filesystem::path& directory;
        std::vector<std::vector<std::string>> files_info;
        std::ostringstream oss;
        size_t chunkSize = 1024;
        char buffer[16384];
        std::size_t part_size = 6;
        char delimiter = ';';
        struct sockaddr_in serverAddr, clientAddr;
        socklen_t addr_size = sizeof(clientAddr);


        void resetVariables(){
            files_info.clear();
            files_info.shrink_to_fit(); 
            oss.str("");
            oss.clear();
            memset(buffer, 0, sizeof(buffer));
            std::cout << "All variables have been reset." << std::endl;
        }


        void InitServer() {
            serverSocket = socket(AF_INET, SOCK_STREAM, 0);
            if (serverSocket < 0) {
                throw std::runtime_error("Socket creation failed");
            }

            // Настройка адреса и порта
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_addr.s_addr = INADDR_ANY;
            serverAddr.sin_port = htons(Port);

            if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
                throw std::runtime_error("Bind failed");
            }

            if (listen(serverSocket, 5) < 0) {
                throw std::runtime_error("Listen failed");
            }
            
            std::cout << "Server listening on port " << Port << std::endl;
        }

        void GetFilesWithSizes() {
            try {
                for (const auto& entry : fs::recursive_directory_iterator(directory)) {
                    if (fs::is_regular_file(entry)) {
                        std::string file_name = entry.path().string(); 
                        std::string short_path  = file_name;
                        std::string prefix_to_remove = directory.string() + "/"; 
                        
                        auto pos = short_path.find(prefix_to_remove);
                        if (pos != std::string::npos) {
                            short_path.erase(pos, prefix_to_remove.length());
                        }

                        std::uintmax_t file_size = fs::file_size(entry);
                        files_info.push_back({short_path, std::to_string(file_size)}); 
                    }
                }
            } catch (const std::exception& e) {
                std::cerr << "Error reading directory: " << e.what() << std::endl;
            }
            std::cout << "server file size vector --- " << files_info.size() << std::endl;
        }

        void SerializeFiles() {
            for (const auto& file : files_info) {
                oss << file[0] << "|" << file[1] << "\n"; // имя файла | размер файла
            }
        }

        void AcceptClient() {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
            if (clientSocket < 0) {
                throw std::runtime_error("Accept failed");
            }
        }
        
        void AcceptClient_second() {
            clientSocket_second = accept(serverSocket, (struct sockaddr*)&clientAddr, &addr_size);
            if (clientSocket_second < 0) {
                throw std::runtime_error("Accept failed");
            }
            std::cout << "server accept the second connection" << std::endl;
        }

        void Listen(){
            if (listen(serverSocket, 5) < 0) {
                throw std::runtime_error("Listen failed");
            }
            std::cout << "Server start listening answer from client" << std::endl;
        }

        void sendInChunks() {
            size_t totalSent = 0;
            size_t stringSize = oss.str().size();

            while (totalSent < stringSize) {
                size_t remaining = stringSize - totalSent;
                size_t toSend = std::min(chunkSize, remaining);
                
                int bytesSent = send(clientSocket, oss.str().c_str() + totalSent, toSend, 0);
                if (bytesSent < 0) {
                    perror("Error sending data");
                    return;
                } else if (bytesSent < chunkSize) {
                    break;
                }
                totalSent += bytesSent; 
                std::cout << "totalSent - \t" << totalSent << std::endl;
            }
        }

        std::string receiveInChunks(int clientSocket) {
            std::vector<char> receivedData;

            while (true) {
                int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
                if (bytesReceived == 0) {
                    break;
                }
                for (int i = 0; i < bytesReceived; ++i) {
                    receivedData.push_back(buffer[i]);
                }
            }

            receivedData.push_back('\0');
            std::string receivedString(receivedData.begin(), receivedData.end() - 1);
            return receivedString;
        }

        std::vector<std::string> split(const std::string& str) {
            std::vector<std::string> tokens;
            std::stringstream ss(str);
            std::string token;

            while (std::getline(ss, token, delimiter)) {
                tokens.push_back(token);
            }

            return tokens;
        }

        std::vector<std::vector<std::string>> splitStringVector(const std::vector<std::string>& original) {
            if (original.size() % part_size != 0) {
                throw std::out_of_range("Data from client is not a multiple of " + std::to_string(part_size) + ", please check again.");
            }

            std::vector<std::vector<std::string>> parts;
            for (size_t i = 0; i < original.size(); i += part_size) {
                std::vector<std::string> part(original.begin() + i, original.begin() + i + part_size);
                parts.push_back(part);
            }

            return parts;
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

        void processStringPart(const std::vector<std::string>& part) {
            std::cout << "Processing part: ";
            std::cout << std::endl;

            std::string filename = part[0]; 
            std::string text = part[1];
            int name_byte_size = std::stoi(part[2]);
            std::string filehasg = part[3];
            std::string path = part[4];
            std::string strig_hasg = part[5];


            size_t pos = path.find('/');
            if (pos != std::string::npos) {
                path = path.substr(pos + 1);
            }

            std::cout << filename << std::endl;
            std::cout << name_byte_size << std::endl;
            //checks

            if (filename.size() != name_byte_size){
                throw std::runtime_error("received filename is not correct, check again");   
            }

            std::string full_string = hashString(filename) + hashString(text) + hashString(std::to_string(name_byte_size)) + hashString(filehasg);

            std::cout << "Length of strig_hasg: " << strig_hasg.size() << std::endl;
            std::cout << "Length of full_string: " << full_string.size() << std::endl;

            if (strig_hasg != full_string) {
                throw std::runtime_error("received data is not correct, check again");
            }
            std::filesystem::path full_path = directory / path;

            std::cout << "File Path - " << full_path << std::endl;

            if (!std::filesystem::exists(full_path.parent_path())) {
                std::filesystem::create_directories(full_path.parent_path());
            }

            // Создание файла и запись текста
            std::ofstream outFile(full_path);
            if (!outFile.is_open()) {
                std::cerr << "Unable to create file: " << filename << std::endl;
                return;
            }
            
            outFile << text;
            outFile.close();
            std::cout << "File " << filename << " created successfully and verified." << std::endl;

        }

        void handleClient() {
            std::string data = receiveInChunks(clientSocket_second);
            std::cout << "data - \t" << data << std::endl;
            std::vector<std::string> result = split(data);
            std::cout << result.size() << "\n";

            try {
                // Разделяем вектор
                std::vector<std::vector<std::string>> parts = splitStringVector(result);
                
                // Обрабатываем каждую часть
                for (const auto& part : parts) {
                    processStringPart(part);
                }

            } catch (const std::out_of_range& e) {
                std::cerr << e.what() << std::endl;
            }

            std::cout << "All_right " << std::endl;

        }
};


int main(int argc, char* argv[]) {
    const int port = atoi(argv[1]);
    int serverSocket, clientSocket;
    std::filesystem::path dir = "server_data";

    Server server(port, serverSocket, clientSocket, dir);
    server.StarteventLoop();

    return 0;
}
