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

dSocketResult dSocket::setTimeoutOption(uint32_t tMilliseconds) const {
    timeval tv{
            .tv_sec = tMilliseconds / 1000,
            .tv_usec = tMilliseconds % 1000 * 1000
    };

    if (setsockopt(mSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
        perror("Error");
    }

    return dSocketResult::SUCCESS;
}
dSocketResult dSocket::setNoDelayOption(bool tEnable) const {
    int Flag = static_cast <int>(tEnable);

    if (setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, &Flag, sizeof(Flag)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::setNoDelayOption" << std::endl;
        }

        return dSocketResult::SET_OPTION_FAILURE;
    }

    return dSocketResult::SUCCESS;
}
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

}
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

    //---DO SOMETHING---//
#endif

    return dSocketResult::SUCCESS;
}
//-----------------------------//
dSocketResult dSocket::readFromSocket(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes) const {
    if (mProtocol != dSocketProtocol::TCP) {
        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t ReadBytes;

    if ((ReadBytes = read(mSocket, tDstBuffer, tBufferSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromSocket" << std::endl;
        }

        return dSocketResult::READ_ERROR;
    }

    *tReadBytes = ReadBytes;
    return dSocketResult::SUCCESS;
}
dSocketResult dSocket::writeToSocket(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes) const {
    if (mProtocol != dSocketProtocol::TCP) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToSocket" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t WrittenBytes;

    if ((WrittenBytes = send(mSocket, tSrcBuffer, tBufferSize, MSG_NOSIGNAL)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToSocket" << std::endl;
        }

        return dSocketResult::WRITE_ERROR;
    }

    *tWrittenBytes = WrittenBytes;
    return dSocketResult::SUCCESS;
}

dSocketResult dSocket::readFromAddress(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes) const {
    if (mType != dSocketType::CLIENT) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromAddress" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromAddress" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t ReadBytes;
    socklen_t StructSize = sizeof(mStruct);

    if ((ReadBytes = recvfrom(mSocket, tDstBuffer, tBufferSize, 0, (struct sockaddr*)&mStruct, &StructSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromAddress" << std::endl;
        }

        return dSocketResult::READ_ERROR;
    }

    *tReadBytes = ReadBytes;
    return dSocketResult::SUCCESS;
}
dSocketResult dSocket::writeToAddress(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes) const {
    if (mType != dSocketType::CLIENT) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToSocket" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToAddress" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t WrittenBytes;

    if ((WrittenBytes = sendto(mSocket, tSrcBuffer, tBufferSize, 0, (const struct sockaddr*)&mStruct, sizeof(mStruct))) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToAddress" << std::endl;
        }

        return dSocketResult::WRITE_ERROR;
    }

    *tWrittenBytes = WrittenBytes;
    return dSocketResult::SUCCESS;
}

dSocketResult dSocket::readFromAddress(uint8_t* tDstBuffer, size_t tBufferSize, ssize_t* tReadBytes, sockaddr* tClientStruct, socklen_t* tClientStructSize) const {
    if (mType != dSocketType::SERVER) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromAddress" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromAddress" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t ReadBytes;

    if ((ReadBytes = recvfrom(mSocket, tDstBuffer, tBufferSize, 0, tClientStruct, tClientStructSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::readFromAddress" << std::endl;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return dSocketResult::RECV_TIMEOUT;
        }

        return dSocketResult::READ_ERROR;
    }

    *tReadBytes = ReadBytes;
    return dSocketResult::SUCCESS;
}
dSocketResult dSocket::writeToAddress(const uint8_t* tSrcBuffer, size_t tBufferSize, ssize_t* tWrittenBytes, const sockaddr* tClientStruct, socklen_t tClientStructSize) const {
    if (mType != dSocketType::SERVER) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToAddress" << std::endl;
        }

        return dSocketResult::WRONG_SOCKET_TYPE;
    }

    if (mProtocol != dSocketProtocol::UDP) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToAddress" << std::endl;
        }

        return dSocketResult::WRONG_PROTOCOL;
    }

    //----------//

    ssize_t WrittenBytes;

    if ((WrittenBytes = sendto(mSocket, tSrcBuffer, tBufferSize, 0, tClientStruct, tClientStructSize)) == -1) {
        if (mVerbose) {
            std::cerr << "dSocket::writeToAddress" << std::endl;
        }

        return dSocketResult::WRITE_ERROR;
    }

    *tWrittenBytes = WrittenBytes;
    return dSocketResult::SUCCESS;
}
//-----------------------------//
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
std::string dSocket::convertUintToIpv4(uint32_t tAddress) {
    uint16_t A, B, C, D;
    char Delimiter = '.';

    A = tAddress >> 24;
    B = (tAddress >> 16) - (A << 8);
    C = (tAddress >> 8) - (A << 16) - (B << 8);
    D = (tAddress << 24) >> 24;

    return std::to_string(A) + Delimiter + std::to_string(B) + Delimiter + std::to_string(C) + Delimiter + std::to_string(D);
}
