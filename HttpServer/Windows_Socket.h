#include <stdint.h>
#include <stddef.h>

using SOCKET_T = void *;

bool Startup(void);
bool Cleanup(void);

bool OpenSocket(SOCKET_T &socketOpen, int32_t &i32ErrorCode);
bool CloseSocket(SOCKET_T &socketClose, int32_t &i32ErrorCode);

bool BindSocket(SOCKET_T socketBind, uint16_t u16ServerPort, uint32_t u32ServerAddr, int32_t &i32ErrorCode);
bool ListenSocket(SOCKET_T socketListen, int32_t i32MaxPendingConnections, int32_t &i32ErrorCode);
bool AcceptSocket(SOCKET_T socketAccept, SOCKET_T &socketClient, uint16_t &u16ClientPort, uint32_t &u32ClientAddr, int32_t &i32ErrorCode);

bool SendData(SOCKET_T socketSend, const void *pDataBuffer, int32_t &i32BufferSize, int32_t &i32ErrorCode);
bool SendDataAll(SOCKET_T socketSend, const void *pDataBuffer, int32_t i32BufferSize, int32_t &i32ErrorCode);

bool RecvData(SOCKET_T socketRecv, void *pDataBuffer, int32_t &i32BufferSize, int32_t &i32ErrorCode);
bool RecvDataAll(SOCKET_T socketRecv, void *pDataBuffer, int32_t i32BufferSize, int32_t &i32ErrorCode);
