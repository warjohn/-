FROM ubuntu:24.04

WORKDIR /socket

RUN apt-get update && apt-get install -y \
    g++ \
    libcpprest-dev \
    libboost-all-dev \
    libssl-dev \
    libcrypto++-dev \
    cmake 

COPY server.cpp .

RUN g++ -fdiagnostics-color=always -g server.cpp -o server -I/usr/include -L/usr/lib -lssl -lcrypto -I/in.h -std=c++17

EXPOSE 8009

RUN mkdir -p WORKDIR/server_data

CMD [ "./server 8009" ]
    
