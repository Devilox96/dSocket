//
// Created by devilox on 12/17/20.
//
//-----------------------------//
#include "dSocket.h"
//-----------------------------//
dSocket::~dSocket() {
    if (mSocket != -1) {
        shutdown(mSocket, 2);
        close(mSocket);
    }

    mSocket = 0;
}
//-----------------------------//
/**
 * Function for initial socket creation
 * @param tProtocol Specified protocol (TCP / UDP)
 * @return Status
 */
dSocketResult dSocket::init(dSocketProtocol tProtocol) {
    mProtocol = tProtocol;

    switch (tProtocol) {
        case dSocketProtocol::TCP:
            mSocket = socket(AF_INET, SOCK_STREAM, 0);
            break;
        case dSocketProtocol::UDP:
            mSocket = socket(AF_INET, SOCK_DGRAM, 0);
            break;
        case dSocketProtocol::UNDEFINED:
            return dSocketResult::WRONG_PROTOCOL;
            break;
    }

    if (mSocket < 0) {
        if (mVerbose) {
            std::cerr << "dSocket::init" << std::endl;
        }

        return dSocketResult::CREATE_FAILURE;
    }

    return dSocketResult::SUCCESS;
}

/**
 * Function for disabling Nagle algorithm for TCP socket (enabled by default) to
 * reduce writing latency if needed
 * @param tEnable Flag (true to disable Nagle algorithm)
 * @return Status
 */
dSocketResult dSocket::setNoDelayOption(bool tEnable) const {
    if (mProtocol != dSocketProtocol::TCP) {
        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    int Flag = static_cast <int>(tEnable);

    if (setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, &Flag, sizeof(Flag)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::setNoDelayOption" << std::endl;
        }

        return dSocketResult::SET_OPTION_FAILURE;
    }

    return dSocketResult::SUCCESS;
}
/**
 * Function for allowing a socket to be bound to the same port right after it was closed (TCP)
 * or for sharing a datagram between sockets bound to the same port (UDP)
 * @param tEnable Flag
 * @return Status
 */
dSocketResult dSocket::setReuseOption(bool tEnable) const {
    int Flag = static_cast <int>(tEnable);

    if (setsockopt(mSocket, SOL_SOCKET, SO_REUSEADDR, &Flag, sizeof(Flag)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::setReuseOption" << std::endl;
        }

        return dSocketResult::SET_OPTION_FAILURE;
    }

    return dSocketResult::SUCCESS;
}

/**
 * Function for filling socket structures and, in case of server, binding to the specified
 * port
 * @param tType Server or client
 * @param tPort Port
 * @param tServerAddress Address to connect to (ignored in case of server type)
 * @return Status
 */
dSocketResult dSocket::finalize(dSocketType tType, uint16_t tPort, const std::string& tServerAddress) {
    if (mProtocol == dSocketProtocol::UNDEFINED) {
        if (mVerbose) {
            std::cerr << "dSocket::finalize" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    mType = tType;

    switch (tType) {
        case dSocketType::UNDEFINED:
            if (mVerbose) {
                std::cerr << "dSocket::finalize" << std::endl;
            }

            return dSocketResult::NO_SOCKET_TYPE;
        case dSocketType::SERVER:
            mStruct.sin_family          = AF_INET;
            mStruct.sin_addr.s_addr     = INADDR_ANY;
            mStruct.sin_port            = htons(tPort);

            if (bind(mSocket, (struct sockaddr*)&mStruct, sizeof(mStruct)) == -1) {
                if (mVerbose) {
                    std::cerr << "dSocket::finalize" << std::endl;
                }

                return dSocketResult::BIND_FAILURE;
            }

            if (mProtocol == dSocketProtocol::TCP) {
                if (listen(mSocket, 1) == -1) {
                    if (mVerbose) {
                        std::cerr << "dSocket::finalize" << std::endl;
                    }

                    return dSocketResult::LISTEN_FAILURE;
                }
            }

            break;
        case dSocketType::CLIENT:
            mStruct.sin_family          = AF_INET;
            mStruct.sin_port            = htons(tPort);

            if (inet_pton(AF_INET, tServerAddress.data(), &mStruct.sin_addr) <= 0) {
                if (mVerbose) {
                    std::cerr << "dSocket::finalize" << std::endl;
                }

                return dSocketResult::ADDRESS_CONVERSION_FAILURE;
            }

            break;
    }

    return dSocketResult::SUCCESS;
}

void dSocket::acceptConnection() {
///---TODO---///
}
/**
 * Function for connecting to server, also sets the socket to non-blocking mode
 * @param tTimeoutMs Select timeout value
 * @return Status
 */
dSocketResult dSocket::connectToServer(uint32_t tTimeoutMs) {
    if (mProtocol != dSocketProtocol::TCP) {
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    if (mType == dSocketType::SERVER) {
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

#if __linux__
    int Result;
    int Flags;
    timeval Timeout {
            .tv_sec = tTimeoutMs / 1000,
            .tv_usec = tTimeoutMs % 1000 * 1000
    };

    if ((Flags = fcntl(mSocket, F_GETFL, nullptr)) < 0) {
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::GET_FLAGS_FAILURE;
    }

    if (fcntl(mSocket, F_SETFL, Flags | O_NONBLOCK) < 0) {
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::SET_FLAGS_FAILURE;
    }

    if ((Result = connect(mSocket, (struct sockaddr*)&mStruct, sizeof(mStruct))) < 0) {
        if (errno == EINPROGRESS) {
            fd_set WaitSet;

            FD_ZERO(&WaitSet);
            FD_SET(mSocket, &WaitSet);

            Result = select(mSocket + 1, nullptr, &WaitSet, nullptr, &Timeout);
        }
    } else {
        Result = 1;
    }

    if (fcntl(mSocket, F_SETFL, Flags) < 0) {
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::SET_FLAGS_FAILURE;
    }

    if (Result < 0) {
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::CONNECTION_FAILURE;
    } else if (Result == 0) {
        errno = ETIMEDOUT;
        if (mVerbose) {
            std::cerr << "dSocket::connectToServer" << std::endl;
        }

        return dSocketResult::CONNECTION_TIMEOUT;
    } else {
        socklen_t Length = sizeof(Flags);

        if (getsockopt(mSocket, SOL_SOCKET, SO_ERROR, &Flags, &Length) < 0) {
            if (mVerbose) {
                std::cerr << "dSocket::connectToServer" << std::endl;
            }

            return dSocketResult::GET_OPTION_FAILURE;
        }

        if (Flags) {
            if (mVerbose) {
                std::cerr << "dSocket::connectToServer" << std::endl;
            }

            switch (Flags) {
                case 111:
                    return dSocketResult::CONNECTION_REFUSED;
                case 113:
                    return dSocketResult::HOST_UNREACHABLE;
                default:
                    std::cerr << "Unknown (SO_ERROR " << Flags << ")" << std::endl;
                    return dSocketResult::UNKNOWN;
            }
        }
    }
#elif _WIN32
    if (connect(mClientSocket, (struct sockaddr*)&mClientStruct, sizeof(mClientStruct)) < 0) {
        throw dSocketException(dSocketException::CONNECT_ERROR, strerror(errno));
    }

    ///---DO SOMETHING---///
#endif

    return dSocketResult::SUCCESS;
}
//-----------------------------//
/**
 * Function for reading data from the TCP socket
 * @param tDstBuffer Buffer to put data into
 * @param tBufferSize Buffer size
 * @param tReadBytes Number of bytes actually received
 * @return Status
 */
dSocketResult dSocket::readTCP(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes) const {
    if (mProtocol != dSocketProtocol::TCP) {
        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t ReadBytes;

    if ((ReadBytes = read(mSocket, tDstBuffer, tBufferSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::readTCP" << std::endl;
        }

        return dSocketResult::READ_ERROR;
    }

    *tReadBytes = ReadBytes;
    return dSocketResult::SUCCESS;
}
/**
 * Function for writing data to the TCP socket
 * @param tSrcBuffer Buffer with the data to send
 * @param tBufferSize Buffer size
 * @param tWrittenBytes Number of bytes actually written
 * @return Status
 */
dSocketResult dSocket::writeTCP(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes) const {
    if (mProtocol != dSocketProtocol::TCP) {
        if (mVerbose) {
            std::cerr << "dSocket::writeTCP" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t WrittenBytes;

    if ((WrittenBytes = send(mSocket, tSrcBuffer, tBufferSize, MSG_NOSIGNAL)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::writeTCP" << std::endl;
        }

        return dSocketResult::WRITE_ERROR;
    }

    *tWrittenBytes = WrittenBytes;
    return dSocketResult::SUCCESS;
}

/**
 * Function for reading data from the UDP server that this socket is connected to
 * @param tDstBuffer Buffer to put data into
 * @param tBufferSize Buffer size
 * @param tReadBytes Number of bytes actually received
 * @return Status
 */
dSocketResult dSocket::readUDP(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes) const {
    if (mType != dSocketType::CLIENT) {
        if (mVerbose) {
            std::cerr << "dSocket::readUDP" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::readUDP" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t ReadBytes;
    socklen_t StructSize = sizeof(mStruct);

    if ((ReadBytes = recvfrom(mSocket, tDstBuffer, tBufferSize, 0, (struct sockaddr*)&mStruct, &StructSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::readUDP" << std::endl;
        }

        return dSocketResult::READ_ERROR;
    }

    *tReadBytes = ReadBytes;
    return dSocketResult::SUCCESS;
}
/**
 * Function for writing data to the UDP server that this socket is connected to
 * @param tSrcBuffer Buffer with the data to send
 * @param tBufferSize Buffer size
 * @param tWrittenBytes Number of bytes actually written
 * @return Status
 */
dSocketResult dSocket::writeUDP(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes) const {
    if (mType != dSocketType::CLIENT) {
        if (mVerbose) {
            std::cerr << "dSocket::writeTCP" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::writeUDP" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t WrittenBytes;

    if ((WrittenBytes = sendto(mSocket, tSrcBuffer, tBufferSize, 0, (const struct sockaddr*)&mStruct, sizeof(mStruct))) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::writeUDP" << std::endl;
        }

        return dSocketResult::WRITE_ERROR;
    }

    *tWrittenBytes = WrittenBytes;
    return dSocketResult::SUCCESS;
}

/**
 * Function for reading data from the specified UDP client
 * @param tDstBuffer Buffer to put data into
 * @param tBufferSize Buffer size
 * @param tReadBytes Number of bytes actually received
 * @param tClientStruct Client data structure
 * @param tClientStructSize Client data structure size
 * @return Status
 */
dSocketResult dSocket::readUDP(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes, sockaddr* tClientStruct, socklen_t* tClientStructSize) const {
    if (mType != dSocketType::SERVER) {
        if (mVerbose) {
            std::cerr << "dSocket::readUDP" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::readUDP" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t ReadBytes;

    if ((ReadBytes = recvfrom(mSocket, tDstBuffer, tBufferSize, 0, tClientStruct, tClientStructSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::readUDP" << std::endl;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return dSocketResult::RECV_TIMEOUT;
        }

        return dSocketResult::READ_ERROR;
    }

    *tReadBytes = ReadBytes;
    return dSocketResult::SUCCESS;
}
/**
 * Function for writing data to the specified UDP client
 * @param tSrcBuffer Buffer with the data to send
 * @param tBufferSize Buffer size
 * @param tWrittenBytes Number of bytes actually written
 * @param tClientStruct Client data structure
 * @param tClientStructSize Client data structure size
 * @return Status
 */
dSocketResult dSocket::writeUDP(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes, const sockaddr* tClientStruct, socklen_t tClientStructSize) const {
    if (mType != dSocketType::SERVER) {
        if (mVerbose) {
            std::cerr << "dSocket::writeUDP" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::writeUDP" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t WrittenBytes;

    if ((WrittenBytes = sendto(mSocket, tSrcBuffer, tBufferSize, 0, tClientStruct, tClientStructSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::writeUDP" << std::endl;
        }

        return dSocketResult::WRITE_ERROR;
    }

    *tWrittenBytes = WrittenBytes;
    return dSocketResult::SUCCESS;
}
//-----------------------------//
/**
 * Function converts x.x.x.x format IPv4 address to a 4-byte number for more
 * efficient storing
 * @param tAddress Address string
 * @return 4-byte number
 */
uint32_t dSocket::convertIpv4ToUint(const std::string& tAddress) {
    uint16_t A, B, C, D;
    char Delimiter;
    std::stringstream Stream(tAddress);
    uint32_t Number = 0;

    Stream >> A >> Delimiter >> B >> Delimiter >> C >> Delimiter >> D;

    Number = (Number << 8) + static_cast <uint8_t>(A);
    Number = (Number << 8) + static_cast <uint8_t>(B);
    Number = (Number << 8) + static_cast <uint8_t>(C);
    Number = (Number << 8) + static_cast <uint8_t>(D);

    return Number;
}
/**
 * Function converts 4-byte number IPv4 address to a human-readable string x.x.x.x
 * @param tAddress Address number
 * @return human-readable string
 */
std::string dSocket::convertUintToIpv4(uint32_t tAddress) {
    uint16_t A, B, C, D;
    char Delimiter = '.';

    A = tAddress >> 24;
    B = (tAddress >> 16) - (A << 8);
    C = (tAddress >> 8) - (A << 16) - (B << 8);
    D = (tAddress << 24) >> 24;

    return std::to_string(A) + Delimiter + std::to_string(B) + Delimiter + std::to_string(C) + Delimiter + std::to_string(D);
}
