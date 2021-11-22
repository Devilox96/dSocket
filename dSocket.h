//
// Created by devilox on 12/17/20.
//
//-----------------------------//
#ifndef DSOCKET_H
#define DSOCKET_H
//-----------------------------//
#include <iostream>
#include <sstream>
#include <fcntl.h>
//-----------------------------//
#if __linux__
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/tcp.h>
    #include <unistd.h>
#elif _WIN32
    #include <winsock2.h>
#else
    #error OS is not supported
#endif
//-----------------------------//
enum class dSocketProtocol {
    UNDEFINED,
    TCP,
    UDP
};
enum class dSocketType {
    UNDEFINED,
    SERVER,
    CLIENT
};
enum class dSocketResult {
    SUCCESS                         = 0x0000,
    CREATE_FAILURE,
    WSA_FAILURE,
    GET_OPTION_FAILURE,
    SET_OPTION_FAILURE,
    NO_SOCKET_TYPE,
    BIND_FAILURE,
    LISTEN_FAILURE,
    ADDRESS_CONVERSION_FAILURE,
    WRONG_SOCKET_TYPE,
    WRONG_PROTOCOL,
    GET_FLAGS_FAILURE,
    SET_FLAGS_FAILURE,
    CONNECTION_TIMEOUT,
    CONNECTION_FAILURE,
    CONNECTION_REFUSED,
    HOST_UNREACHABLE,
    READ_ERROR,
    WRITE_ERROR,
    RECV_TIMEOUT,
    UNKNOWN                         = 0xFFFF
};
//-----------------------------//
class dSocket {
public:
    explicit dSocket(bool tVerbose = false) : mVerbose(tVerbose) {}
    ~dSocket();

    //----------//

    dSocketResult init(dSocketProtocol tProtocol);

    [[nodiscard]] dSocketResult setNoDelayOption(bool tEnable);
    [[nodiscard]] dSocketResult setReuseOption(bool tEnable);

    dSocketResult finalize(dSocketType tType, uint16_t tPort, const std::string& tServerAddress = "");

    int acceptConnection();
    dSocketResult connectToServer(uint32_t tTimeoutMs);

    //----------//

    dSocketResult readTCP(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes);
    dSocketResult writeTCP(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes);

    dSocketResult readTCP(int tSocket, uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes);
    dSocketResult writeTCP(int tSocket, const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes);

    //----------//

    dSocketResult readUDP(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes);
    dSocketResult writeUDP(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes);

    dSocketResult readUDP(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes, sockaddr* tClientStruct, socklen_t* tClientStructSize);
    dSocketResult writeUDP(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes, const sockaddr* tClientStruct, socklen_t tClientStructSize);

    //----------//

    [[nodiscard]] std::string getLastError() const;

    //----------//

    static uint32_t convertIpv4ToUint(const std::string& tAddress);
    static std::string convertUintToIpv4(uint32_t tAddress);
private:
#if __linux__
    int32_t             mSocket         = 0;
#elif _WIN32
    WSAData         mWSA;
    SOCKET          mSocket     = 0;
#endif
    sockaddr_in         mStruct         = {};
    dSocketType         mType           = dSocketType::UNDEFINED;
    dSocketProtocol     mProtocol       = dSocketProtocol::UNDEFINED;
    bool                mVerbose        = false;

    int                 mLastErrno      = 0;
};
//-----------------------------//
#endif