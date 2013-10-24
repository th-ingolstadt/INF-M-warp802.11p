/******************************************************************************
*
* File   :	wn_mex_udp_transport.c
* Authors:	Chris Hunter (chunter [at] mangocomm.com)
*			Patrick Murphy (murphpo [at] mangocomm.com)
*           Erik Welsh (welsh [at] mangocomm.com)
* License:	Copyright 2013, Mango Communications. All rights reserved.
*			Distributed under the WARP license  (http://warpproject.org/license)
*
******************************************************************************/
/**
*
* @file wn_mex_udp_transport.c
*
* Implements the basic socket layer in MEX for the WARPNet Transport protocol 
*
* Compile command:
*
*   Windows (Visual C++):
*       mex -g -O wn_mex_udp_transport.c -lwsock32 -lKernel32 -DWIN32
*
*   MAC / Unix:
*       mex -g -O wn_mex_udp_transport.c
*
*
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -------------------------------------------------------
* 1.00a ejw  7/15/13  Initial release
* 1.01a ejw  8/28/13  Updated to support MAC / Unix as well as Windows
*
*
******************************************************************************/



/***************************** Include Files *********************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <matrix.h>
#include <mex.h>   


#ifdef WIN32

#include <Windows.h>
#include <WinBase.h>
#include <winsock.h>

#else

#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

#endif



/*************************** Constant Definitions ****************************/

// Use to print debug message to the console
// #define _DEBUG_


#ifdef WIN32

// Define printf for future compatibility
#define printf                          mexPrintf
#define malloc(x)                       mxMalloc(x)
#define free(x)                         mxFree(x)
#define make_persistent(x)              mexMakeMemoryPersistent(x)
#define usleep(x)                       Sleep((x)/1000)
#define close(x)                        closesocket(x)
#define non_blocking_socket(x)        { unsigned long optval = 1; ioctlsocket( x, FIONBIO, &optval ); }
#define wn_usleep(x)                    wn_mex_udp_transport_usleep(x)
#define SOCKET                          SOCKET
#define get_last_error                  WSAGetLastError()
#define EWOULDBLOCK                     WSAEWOULDBLOCK
#define socklen_t                       int

#else

// Define printf for future compatibility
#define printf                          mexPrintf
#define malloc(x)                       mxMalloc(x)
#define free(x)                         mxFree(x)
#define make_persistent(x)              mexMakeMemoryPersistent(x)
#define usleep(x)                       usleep(x)
#define close(x)                        close(x)
#define non_blocking_socket(x)          fcntl( x, F_SETFL, O_NONBLOCK )
#define wn_usleep(x)                    usleep(x)
#define SOCKET                          int
#define get_last_error                  errno
#define INVALID_SOCKET                  0xFFFFFFFF
#define SOCKET_ERROR                    -1

#endif



// Version WARPNet MEX Transport Driver
#define WN_MEX_UDP_TRANSPORT_VERSION    "1.0.0a"

// WL_MEX_UDP_TRANSPORT Commands
#define TRANSPORT_REVISION              0
#define TRANSPORT_INIT_SOCKET           1
#define TRANSPORT_SET_SO_TIMEOUT        2
#define TRANSPORT_SET_SEND_BUF_SIZE     3
#define TRANSPORT_GET_SEND_BUF_SIZE     4
#define TRANSPORT_SET_RCVD_BUF_SIZE     5
#define TRANSPORT_GET_RCVD_BUF_SIZE     6
#define TRANSPORT_CLOSE                 7
#define TRANSPORT_SEND                  8
#define TRANSPORT_RECEIVE               9
#define TRANSPORT_READ_IQ              10
#define TRANSPORT_READ_RSSI            11
#define TRANSPORT_WRITE_IQ             12

// Maximum number of sockets that can be allocated
#define TRANSPORT_MAX_SOCKETS           5

// Maximum size of a packet
#define TRANSPORT_MAX_PKT_LENGTH        9050

// Socket state
#define TRANSPORT_SOCKET_FREE           0
#define TRANSPORT_SOCKET_IN_USE         1

// Transport defines
#define TRANSPORT_NUM_PENDING           20
#define TRANSPORT_MIN_SEND_SIZE         1000
#define TRANSPORT_SLEEP_TIME            10000
#define TRANSPORT_FLAG_ROBUST           0x0001
#define TRANSPORT_PADDING_SIZE          2
#define TRANSPORT_TIMEOUT               1000000
#define TRANSPORT_MAX_RETRY             50

// Sample defines
#define SAMPLE_CHKSUM_RESET             0x01
#define SAMPLE_CHKSUM_NOT_RESET         0x00

// WARP HW version defines
#define TRANSPORT_WARP_HW_v2            2
#define TRANSPORT_WARP_HW_v3            3

// WARP Buffers defines
#define TRANSPORT_WARP_RF_BUFFER_MAX    4


/*************************** Variable Definitions ****************************/

// Define types for different size data
typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;

typedef char            int8;
typedef short           int16;
typedef int             int32;


// Data packet structure
typedef struct
{
    char              *buf;       // Pointer to the data buffer
    int                length;    // Length of the buffer (buffer must be pre-allocated)
    int                offset;    // Offset of data to be sent or received
    struct sockaddr_in address;   // Address information of data to be sent / recevied    
} wn_trans_data_pkt;

// Socket structure
typedef struct
{
    SOCKET              handle;   // Handle to the socket
    int                 timeout;  // Timeout value
    int                 status;   // Status of the socket
    wn_trans_data_pkt  *packet;   // Pointer to a data_packet
} wn_trans_socket;

// WARPNet Transport Header
typedef struct
{
    uint16             padding;        // Padding for memory alignment
    uint16             dest_id;        // Destination ID
    uint16             src_id;         // Source ID
    uint8              rsvd;           // Reserved
    uint8              pkt_type;       // Packet type
    uint16             length;         // Length
    uint16             seq_num;        // Sequence Number
    uint16             flags;          // Flags
} wn_transport_header;

// WARPNet Command Header
typedef struct
{
    uint32             command_id;     // Command ID
    uint16             length;         // Length
    uint16             num_args;       // Number of Arguments
} wn_command_header;

// WARPNet Sample Header
typedef struct
{
    uint16             buffer_id;      // Buffer ID
    uint8              flags;          // Flags
    uint8              rsvd;           // Reserved
    uint32             start;          // Starting sample
    uint32             num_samples;    // Number of samples
} wn_sample_header;

// WARPNet Sample Tracket
typedef struct
{
    uint32             start_sample;   // Starting sample
    uint32             num_samples;    // Number of samples
} wn_sample_tracker;


/*********************** Global Variable Definitions *************************/

static int       initialized    = 0;   // Global variable to initialize the driver
static int       tx_buffer_size = 0;   // Global variable for TX buffer size
static int       rx_buffer_size = 0;   // Global variable for RX buffer size
wn_trans_socket  sockets[TRANSPORT_MAX_SOCKETS];
                                       // Global structure of socket connections

#ifdef WIN32
WSADATA          wsaData;              // Structure for WinSock setup communication 
#endif

/*************************** Function Prototypes *****************************/

// Main function
void         mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[]);

// Socket functions
void         print_version( void );
void         init_wn_mex_udp_transport( void );
int          init_socket( void );
void         set_so_timeout( int index, int value );
void         set_reuse_address( int index, int value );
void         set_broadcast( int index, int value );
void         set_send_buffer_size( int index, int size );
int          get_send_buffer_size( int index );
void         set_receive_buffer_size( int index, int size );
int          get_receive_buffer_size( int index );
void         close_socket( int index );
int          send_socket( int index, char *buffer, int length, char *ip_addr, int port );
int          receive_socket( int index, int length, char * buffer );

// Debug / Error functions
void         print_usage( void );
void         print_sockets( void );
void         print_buffer(unsigned char *buf, int size);
void         die( void );
void         die_with_error( char *errorMessage );
void         cleanup( void );

// Helper functions
void         convert_to_uppercase( char *input, char *output, unsigned int len );
unsigned int find_transport_function( char *input, unsigned int len );

uint16       endian_swap_16(uint16 value);
uint32       endian_swap_32(uint32 value);

void         wn_mex_udp_transport_usleep( int wait_time );
unsigned int wn_update_checksum(unsigned short int newdata, unsigned char reset );
int          wn_read_iq_sample_error( wn_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size );
int          wn_read_iq_find_error( wn_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size,
                                    uint32 *ret_num_samples, uint32 *ret_start_sample, uint32 *ret_num_pkts );

// WARPNet Functions
int          wn_read_baseband_buffer( int index, char *buffer, int length, char *ip_addr, int port,
                                      int num_samples, int start_sample, uint32 buffer_id, 
                                      uint32 *output_array, uint32 *num_cmds );

int          wn_write_baseband_buffer( int index, char *buffer, int max_length, char *ip_addr, int port,
                                       int num_samples, int start_sample, uint16 *samples_i, uint16 *samples_q, uint32 buffer_id,
                                       int num_pkts, int max_samples, int hw_ver, uint32 *num_cmds );

/******************************** Functions **********************************/



/*****************************************************************************/
/**
*  Function:  init_wn_mex_udp_transport
* 
*  Initialize the driver
*
******************************************************************************/

void init_wn_mex_udp_transport( void ) {
    int i;

    // Print initalization information
    printf("Loaded wn_mex_udp_transport version %s \n", WN_MEX_UDP_TRANSPORT_VERSION );

    // Initialize Socket datastructure
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        memset( &sockets[i], 0, sizeof(wn_trans_socket) );
        
        sockets[i].handle  = INVALID_SOCKET;
        sockets[i].status  = TRANSPORT_SOCKET_FREE;
        sockets[i].timeout = 0;
        sockets[i].packet  = NULL;
    }

#ifdef WIN32
    // Load the Winsock 2.0 DLL
    if ( WSAStartup(MAKEWORD(2, 0), &wsaData) != 0 ) {
        die_with_error("WSAStartup() failed");
    }
#endif
        
    // Set cleanup function to remove persistent data    
    mexAtExit(cleanup);
    
    // Set initialized flag to '1' so this is not run again
    initialized = 1;
}


/*****************************************************************************/
/**
*  Function:  init_socket
*
*  Initializes a socket and returns the index in to the sockets array
*
******************************************************************************/

int init_socket( void ) {
    int i;
    
    // Allocate a socket in the datastructure
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        if ( sockets[i].status == TRANSPORT_SOCKET_FREE ) {  break; }    
    }

    // Check to see if we cannot allocate a socket
    if ( i == TRANSPORT_MAX_SOCKETS) {
        die_with_error("Error:  Cannot allocate a socket");
    }
        
    // Create a best-effort datagram socket using UDP
    if ( ( sockets[i].handle = socket( PF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0) {
        die_with_error("socket() failed");
    }

    // Update the status field of the socket
    sockets[i].status = TRANSPORT_SOCKET_IN_USE;
    
    // Set the reuse_address and broadcast flags for all sockets
    set_reuse_address( i, 1 );
    set_broadcast( i, 1 );
    
    // Listen on the socket; Make sure we have a non-blocking socket
    listen( sockets[i].handle, TRANSPORT_NUM_PENDING );
    non_blocking_socket( sockets[i].handle );

    return i;    
}


/*****************************************************************************/
/**
*  Function:  set_so_timeout
*
*  Sets the Socket Timeout to the value (in ms)
*
******************************************************************************/
void set_so_timeout( int index, int value ) {

    sockets[index].timeout = value;
}


/*****************************************************************************/
/**
*  Function:  set_reuse_address
*
*  Sets the Reuse Address option on the socket
*
******************************************************************************/
void set_reuse_address( int index, int value ) {
    int optval;

    if ( value ) {
        optval = 1;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval) );
    } else {
        optval = 0;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_REUSEADDR, (const char *)&optval, sizeof(optval) );
    }    
}

/*****************************************************************************/
/**
*  Function:  set_broadcast
*
*  Sets the Broadcast option on the socket
*
******************************************************************************/
void set_broadcast( int index, int value ) {
    int optval;

    if ( value ) {
        optval = 1;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_BROADCAST, (const char *)&optval, sizeof(optval) );
    } else {
        optval = 0;
        setsockopt( sockets[index].handle, SOL_SOCKET, SO_BROADCAST, (const char *)&optval, sizeof(optval) );
    }    
}


/*****************************************************************************/
/**
*  Function:  set_send_buffer_size
*
*  Sets the send buffer size on the socket
*
******************************************************************************/
void set_send_buffer_size( int index, int size ) {
    int optval = size;

    // Set the global variable to what we requested the transmit buffer size to be
    tx_buffer_size = size;

    setsockopt( sockets[index].handle, SOL_SOCKET, SO_SNDBUF, (const char *)&optval, sizeof(optval) );
}


/*****************************************************************************/
/**
*  Function:  get_send_buffer_size
*
*  Gets the send buffer size on the socket
*
******************************************************************************/
int get_send_buffer_size( int index ) {
    int optval = 0;
    int optlen = sizeof(int);
    int retval = 0;
    
    if ( (retval = getsockopt( sockets[index].handle, SOL_SOCKET, SO_SNDBUF, (char *)&optval, (socklen_t *)&optlen )) != 0 ) {
        die_with_error("Error:  Could not get socket option - send buffer size"); 
    }
    
#ifdef _DEBUG_    
    printf("Send Buffer Size:  %d \n", optval );
#endif
    
    // Set the global variable to what the OS reports that it is
    tx_buffer_size = optval;

    return optval;
}


/*****************************************************************************/
/**
*  Function:  set_receive_buffer_size
*
*  Sets the receive buffer size on the socket
*
******************************************************************************/
void set_receive_buffer_size( int index, int size ) {
    int optval = size;

    // Set the global variable to what we requested the receive buffer size to be
    rx_buffer_size = size;

    setsockopt( sockets[index].handle, SOL_SOCKET, SO_RCVBUF, (const char *)&optval, sizeof(optval) );
}


/*****************************************************************************/
/**
*  Function:  get_receive_buffer_size
*
*  Gets the receive buffer size on the socket
*
******************************************************************************/
int get_receive_buffer_size( int index ) {
    int optval = 0;
    int optlen = sizeof(int);
    int retval = 0;
    
    if ( (retval = getsockopt( sockets[index].handle, SOL_SOCKET, SO_RCVBUF, (char *)&optval, (socklen_t *)&optlen )) != 0 ) {
        die_with_error("Error:  Could not get socket option - send buffer size"); 
    }
    
#ifdef _DEBUG_    
    printf("Rcvd Buffer Size:  %d \n", optval );
#endif
    
    // Set the global variable to what the OS reports that it is
    rx_buffer_size = optval;

    return optval;
}


/*****************************************************************************/
/**
*  Function:  close_socket
*
*  Closes the socket based on the index
*
******************************************************************************/
void close_socket( int index ) {

#ifdef _DEBUG_
    printf("Close Socket: %d\n", index);
#endif    

    if ( sockets[index].handle != INVALID_SOCKET ) {
        close( sockets[index].handle );
        
        if ( sockets[index].packet != NULL ) {
            free( sockets[index].packet );
        }
    } else {
        printf( "WARNING:  Connection %d already closed.\n", index );
    }

    sockets[index].handle  = INVALID_SOCKET;
    sockets[index].status  = TRANSPORT_SOCKET_FREE;
    sockets[index].timeout = 0;
    sockets[index].packet  = NULL;
}


/*****************************************************************************/
/**
*  Function:  send_socket
*
*  Sends the buffer to the IP address / Port that is passed in
*
******************************************************************************/
int send_socket( int index, char *buffer, int length, char *ip_addr, int port ) {

    struct sockaddr_in socket_addr;  // Socket address
    int                length_sent;
    int                size;

    // Construct the address structure
    memset( &socket_addr, 0, sizeof(socket_addr) );        // Zero out structure 
    socket_addr.sin_family      = AF_INET;                 // Internet address family
    socket_addr.sin_addr.s_addr = inet_addr(ip_addr);      // IP address 
    socket_addr.sin_port        = htons(port);             // Port 

    // If we are sending a large amount of data, we need to make sure the entire 
    // buffer has been sent.
    
    length_sent = 0;
    size        = 0xFFFF;
    
    if ( sockets[index].status != TRANSPORT_SOCKET_IN_USE ) {
        return length_sent;
    }

    while ( length_sent < length ) {
    
        // If we did not send more than MIN_SEND_SIZE, then wait a bit
        if ( size < TRANSPORT_MIN_SEND_SIZE ) {
            usleep( TRANSPORT_SLEEP_TIME );
        }

        // Send as much data as possible to the address
        size = sendto( sockets[index].handle, &buffer[length_sent], (length - length_sent), 0, 
                      (struct sockaddr *) &socket_addr, sizeof(socket_addr) );

        // Check the return value    
        if ( size == SOCKET_ERROR )  {
            if ( get_last_error != EWOULDBLOCK ) {
                die_with_error("Error:  Socket Error.");
            } else {
                // If the socket is not ready, then we did not send any bytes
                length_sent += 0;
            }
        } else {
            // Update how many bytes we sent
            length_sent += size;        
        }

        // TODO:  IMPLEMENT A TIMEOUT SO WE DONT GET STUCK HERE FOREVER
        //        FOR WARPNet 7.3.0, this is not implemented and has not 
        //        been an issue during testing.
    }
    
    return length_sent;
}


/*****************************************************************************/
/**
*  Function:  receive_socket
*
*  Reads data from the socket; will return 0 if no data is available
*
******************************************************************************/
int receive_socket( int index, int length, char * buffer ) {

    wn_trans_data_pkt  *pkt;           
    int                 size;
    int                 socket_addr_size = sizeof(struct sockaddr_in);
    
    // Allocate a packet in memory if necessary
    if ( sockets[index].packet == NULL ) {
        sockets[index].packet = (wn_trans_data_pkt *) malloc( sizeof(wn_trans_data_pkt) );
        
        make_persistent( sockets[index].packet );

        if ( sockets[index].packet == NULL ) {
            die_with_error("Error:  Cannot allocate memory for packet.");        
        }

        memset( sockets[index].packet, 0, sizeof(wn_trans_data_pkt));        
    }

    // Get the packet associcated with the index
    pkt = sockets[index].packet;

    // If we have a packet from the last recevie call, then zero out the address structure    
    if ( pkt->length != 0 ) {
        memset( &(pkt->address), 0, sizeof(socket_addr_size));
    }

    // Receive a response 
    size = recvfrom( sockets[index].handle, buffer, length, 0, 
                    (struct sockaddr *) &(pkt->address), (socklen_t *) &socket_addr_size );


    // Check on error conditions
    if ( size == SOCKET_ERROR )  {
        if ( get_last_error != EWOULDBLOCK ) {
            die_with_error("Error:  Socket Error.");
        } else {
            // If the socket is not ready, then just return a size of 0 so the function can be 
            // called again
            size = 0;
        }
    } else {
                    
        // Update the packet associated with the socket
        //   NOTE:  pkt.address was updated via the function call
        pkt->buf     = buffer;
        pkt->offset  = 0;
    }

    // Update the packet length so we can determine when we need to zero out pkt.address
    pkt->length  = size;

    return size;
}


/*****************************************************************************/
/**
*  Function:  cleanup
*
*  Function called by atMexExit to close everything
*
******************************************************************************/
void cleanup( void ) {
    int i;

    printf("MEX-file is terminating\n");

    // Close all sockets
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        if ( sockets[i].handle != INVALID_SOCKET ) {  close_socket( i ); }    
    }

#ifdef WIN32
    WSACleanup();  // Cleanup Winsock 
#endif

}


/*****************************************************************************/
/**
*  Function:  print_version
*
* This function will print the version of the wn_mex_udp_transport driver
*
******************************************************************************/
void print_version( ) {

    printf("WARPNet MEX UDP Transport v%s (compiled %s %s)\n", WN_MEX_UDP_TRANSPORT_VERSION, __DATE__, __TIME__);
    printf("Copyright 2013, Mango Communications. All rights reserved.\n");
    printf("Distributed under the WARP license:  http://warpproject.org/license  \n");
}


/*****************************************************************************/
/**
*  Function:  print_usage
*
* This function will print the usage of the wn_mex_udp_transport driver
*
******************************************************************************/
void print_usage( ) {

    printf("Usage:  WARPNet MEX Transport v%s \n", WN_MEX_UDP_TRANSPORT_VERSION );
    printf("Standard WARPNet transport functions: \n");
    printf("    1.                  wn_mex_udp_transport('version') \n");
    printf("    2. index          = wn_mex_udp_transport('init_socket') \n");
    printf("    3.                  wn_mex_udp_transport('set_so_timeout', index, timeout) \n");
    printf("    4.                  wn_mex_udp_transport('set_send_buf_size', index, size) \n");
    printf("    5. size           = wn_mex_udp_transport('get_send_buf_size', index) \n");
    printf("    6.                  wn_mex_udp_transport('set_rcvd_buf_size', index, size) \n");
    printf("    7. size           = wn_mex_udp_transport('get_rcvd_buf_size', index) \n");
    printf("    8.                  wn_mex_udp_transport('close', index) \n");
    printf("    9. size           = wn_mex_udp_transport('send', index, buffer, length, ip_addr, port) \n");
    printf("   10. [size, buffer] = wn_mex_udp_transport('receive', index, length ) \n");
    printf("\n");
    printf("Additional WARPNet MEX UDP transport functions: \n");
    printf("    1. [num_samples, cmds_used, samples]  = wn_mex_udp_transport('read_rssi' / 'read_iq', \n");
    printf("                                                index, buffer, length, ip_addr, port, \n");
    printf("                                                number_samples, buffer_id, start_sample) \n");
    printf("    2. cmds_used                          = wn_mex_udp_transport('write_iq', \n");
    printf("                                                index, cmd_buffer, max_length, ip_addr, port, \n");
    printf("                                                number_samples, sample_buffer, buffer_id, \n");
    printf("                                                start_sample, num_pkts, max_samples, hw_ver) \n");
    printf("\n");
    printf("See documentation for further details.\n");
    printf("\n");
}


/*****************************************************************************/
/**
*  Function:  die
*
*  Generates an error message and cause the program to halt
*
******************************************************************************/
void die( ) {
    mexErrMsgTxt("Error:  See description above.");
}


/*****************************************************************************/
/**
*  Function:  die_with_error
*
* This function will error out of the wn_mex_udp_transport function call
*
******************************************************************************/
void die_with_error(char *errorMessage) {
    printf("%s \n   Socket Error Code: %d\n", errorMessage, get_last_error );
    die();
}


#if _DEBUG_

/*****************************************************************************/
/**
*  Function:  print_sockets
*
*  Debug function to print socket table
*
******************************************************************************/
void print_sockets( void ) {
    int i;
    
    printf("Sockets: \n");    
    
    for ( i = 0; i < TRANSPORT_MAX_SOCKETS; i++ ) {
        printf("    socket[%d]:  handle = 0x%4x,   timeout = 0x%4x,  status = 0x%4x,  packet = 0x%8x\n", 
               i, sockets[i].handle, sockets[i].timeout, sockets[i].status, sockets[i].packet );
    }

    printf("\n");
}


/*****************************************************************************/
/**
*  Function:  print_buffer
*
*  Debug function to print a buffer
*
******************************************************************************/
void print_buffer(unsigned char *buf, int size) {
	int i;

	printf("Buffer: (0x%x bytes)\n", size);

	for (i=0; i<size; i++) {
        printf("%2x ", buf[i] );
        if ( (((i + 1) % 16) == 0) && ((i + 1) != size) ) {
            printf("\n");
        }
	}
	printf("\n\n");
}


/*****************************************************************************/
/**
*  Function:  print_buffer_16
*
*  Debug function to print a buffer
*
******************************************************************************/
void print_buffer_16(uint16 *buf, int size) {
	int i;

	printf("Buffer: (0x%x bytes)\n", (2*size));

	for (i=0; i<size; i++) {
        printf("%4x ", buf[i] );
        if ( (((i + 1) % 16) == 0) && ((i + 1) != size) ) {
            printf("\n");
        }
	}
	printf("\n\n");
}


/*****************************************************************************/
/**
*  Function:  print_buffer_32
*
*  Debug function to print a buffer
*
******************************************************************************/
void print_buffer_32(uint32 *buf, int size) {
	int i;

	printf("Buffer: (0x%x bytes)\n", (4*size));

	for (i=0; i<size; i++) {
        printf("%8x ", buf[i] );
        if ( (((i + 1) % 8) == 0) && ((i + 1) != size) ) {
            printf("\n");
        }
	}
	printf("\n\n");
}

#endif


/*****************************************************************************/
/**
*  Function:  endian_swap_16
*
* This function will perform an byte endian swap on a 16-bit value 
*
******************************************************************************/
uint16 endian_swap_16(uint16 value) {

	return (((value & 0xFF00) >> 8) | ((value & 0x00FF) << 8));
}


/*****************************************************************************/
/**
*  Function:  endian_swap_32
*
* This function will perform an byte endian swap on a 32-bit value 
*
******************************************************************************/
uint32 endian_swap_32(uint32 value) {
	uint16 lo;
	uint16 hi;

	// get each of the half words from the 32 bit word
	lo = (uint16) (value & 0x0000FFFF);
	hi = (uint16) ((value & 0xFFFF0000) >> 16);

	// byte swap each of the 16 bit half words
	lo = (((lo & 0xFF00) >> 8) | ((lo & 0x00FF) << 8));
	hi = (((hi & 0xFF00) >> 8) | ((hi & 0x00FF) << 8));

	// swap the half words before returning the value
	return (uint32) ((lo << 16) | hi);
}


/*****************************************************************************/
/**
*  Function:  convert_to_uppercase
*
* This function converts the input string to all uppercase letters
*
******************************************************************************/
void convert_to_uppercase(char *input, char *output, unsigned int len){
  unsigned int i;
  
  for ( i = 0; i < (len - 1); i++ ) { 
      output[i] = toupper( input[i] ); 
  }

  output[(len - 1)] = '\0';  
}


/*****************************************************************************/
/**
*  Function:  find_transport_function
*
* This function will return the wn_mex_udp_transport function number based on 
* the input string
*
******************************************************************************/
unsigned int find_transport_function( char *input, unsigned int len ) {
    unsigned int function = 0xFFFF;
    char * uppercase;
    
    uppercase = (char *) mxCalloc(len, sizeof(char));
    convert_to_uppercase( input, uppercase, len );

#ifdef _DEBUG_
    printf("Function :  %s\n", uppercase);
#endif    

    if ( !strcmp( uppercase, "VERSION"           ) && ( function == 0xFFFF ) ) { function = TRANSPORT_REVISION;          }
    if ( !strcmp( uppercase, "INIT_SOCKET"       ) && ( function == 0xFFFF ) ) { function = TRANSPORT_INIT_SOCKET;       }
    if ( !strcmp( uppercase, "SET_SO_TIMEOUT"    ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SET_SO_TIMEOUT;    }
    if ( !strcmp( uppercase, "SET_SEND_BUF_SIZE" ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SET_SEND_BUF_SIZE; }
    if ( !strcmp( uppercase, "GET_SEND_BUF_SIZE" ) && ( function == 0xFFFF ) ) { function = TRANSPORT_GET_SEND_BUF_SIZE; }
    if ( !strcmp( uppercase, "SET_RCVD_BUF_SIZE" ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SET_RCVD_BUF_SIZE; }
    if ( !strcmp( uppercase, "GET_RCVD_BUF_SIZE" ) && ( function == 0xFFFF ) ) { function = TRANSPORT_GET_RCVD_BUF_SIZE; }
    if ( !strcmp( uppercase, "CLOSE"             ) && ( function == 0xFFFF ) ) { function = TRANSPORT_CLOSE;             }
    if ( !strcmp( uppercase, "SEND"              ) && ( function == 0xFFFF ) ) { function = TRANSPORT_SEND;              }
    if ( !strcmp( uppercase, "RECEIVE"           ) && ( function == 0xFFFF ) ) { function = TRANSPORT_RECEIVE;           }
    if ( !strcmp( uppercase, "READ_IQ"           ) && ( function == 0xFFFF ) ) { function = TRANSPORT_READ_IQ;           }
    if ( !strcmp( uppercase, "READ_RSSI"         ) && ( function == 0xFFFF ) ) { function = TRANSPORT_READ_RSSI;         }
    if ( !strcmp( uppercase, "WRITE_IQ"          ) && ( function == 0xFFFF ) ) { function = TRANSPORT_WRITE_IQ;          }

    mxFree( uppercase );
    return function;
}


/*****************************************************************************/
/**
*
* This function is the mex function to implement the data transfer over sockets
*
* @param	nlhs - Number of left hand side (return) arguments
* @param	plhs - Pointers to left hand side (return) arguments
* @param	nrhs - Number of right hand side (function) arguments
* @param    prhs - Pointers to right hand side (function) arguments
*
* @return	None
*
* @note		This function requires the following pointers:
*   Input:
*       - Function Name
*       - Necessary function arguments (see individual function)
*
*   Output:
*       - Necessary function outputs (see individual function)
*
******************************************************************************/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
{

    //--------------------------------------------------------------------
    // Declare variables
    
    char    *func                   = NULL;
    mwSize  func_len                = 0;
    int     function                = 0xFFFF;
    int     handle                  = 0xFFFF;
    char   *buffer                  = NULL;
    uint16 *sample_I_buffer         = NULL;
    uint16 *sample_Q_buffer         = NULL;
    char   *ip_addr                 = NULL;
    int     size                    = 0;
    int     timeout                 = 0;
    int     length                  = 0;
    int     port                    = 0;
    int     num_samples             = 0;
    int     max_samples             = 0;
    int     start_sample            = 0;
    int     num_pkts                = 0;
    int     max_length              = 0;
    uint32  num_cmds                = 0;
    int     hw_ver                  = 0;
    uint32  buffer_id               = 0;
    uint32  start_sample_to_request = 0;
    uint32  num_samples_to_request  = 0;
    uint32  num_pkts_to_request     = 0;
    uint32  useful_rx_buffer_size   = 0;
    double  temp_val                = 0.0;
    uint32 *buffer_ids              = NULL;
    uint32 *output_array            = NULL;
    uint32 *command_args            = NULL;
    int     i;


    //--------------------------------------------------------------------
    // Initialization    
    
    if (!initialized) {
#ifdef _DEBUG_
            printf("Initialization \n");
#endif
    
       // Initialize driver
       init_wn_mex_udp_transport();           

#ifdef _DEBUG_
            printf("END Initialization \n");
#endif
    }


    //--------------------------------------------------------------------
    // Process input
    
    // Check for proper number of arguments 
    if( nrhs < 1 ) { print_usage(); die(); }

    // Input must be a string 
    if ( mxIsChar( prhs[0] ) != 1 ) {
        mexErrMsgTxt("Error: Input must be a string.");
    }

    // Input must be a row vector
    if ( mxGetM( prhs[0] ) != 1 ) {
        mexErrMsgTxt("Error: Input must be a row vector.");
    }
    
    // get the length of the input string
    func_len = (mwSize) ( mxGetM( prhs[0] ) * mxGetN( prhs[0] ) ) + 1;

    // copy the string data from prhs[0] into a C string func
    func = mxArrayToString( prhs[0] );
    
    if( func == NULL ) {
        mexErrMsgTxt("Error:  Could not convert input to string.");
    }


    //--------------------------------------------------------------------
    // Process commands
    
    function = find_transport_function( func, func_len );

    switch ( function ) {
    
        //------------------------------------------------------
        // wn_mex_udp_transport('version')
        //   - Arguments:
        //     - none
        //   - Returns:
        //     - none
        case TRANSPORT_REVISION :
#ifdef _DEBUG_
            printf("Function TRANSPORT_REVISION: \n");
#endif
            // Validate arguments
            if( nrhs != 1 ) { print_usage(); die(); }

            // Call function
            if( nlhs == 0 ) { 
                print_version(); 
            } else if( nlhs == 1 ) { 
                plhs[0] = mxCreateString( WN_MEX_UDP_TRANSPORT_VERSION );
            } else { 
                print_usage(); die(); 
            }

#ifdef _DEBUG_
            printf("END TRANSPORT_REVISION \n");
#endif
        break;

        //------------------------------------------------------
        // handle = wn_mex_udp_transport('init_socket')
        //   - Arguments:
        //     - none
        //   - Returns:
        //     - handle (int)     - index to the socket
        case TRANSPORT_INIT_SOCKET :
#ifdef _DEBUG_
            printf("Function TRANSPORT_INIT_SOCKET: \n");
#endif
            // Validate arguments
            if( nrhs != 1 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }

            // Call function
            handle = init_socket();
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = handle;

#ifdef _DEBUG_
            print_sockets();
            printf("END TRANSPORT_INIT_SOCKET \n");
#endif
        break;

        //------------------------------------------------------
        // wn_mex_udp_transport('set_so_timeout', handle, timeout)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - timeout (int)    - timeout value (in ms); 0 => infinite timeout
        //   - Returns:
        //     - none
        case TRANSPORT_SET_SO_TIMEOUT :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SET_SO_TIMEOUT\n");
#endif
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            timeout = (int) mxGetScalar(prhs[2]);

            // Call function
            set_so_timeout( handle, timeout );
        
#ifdef _DEBUG_
            print_sockets();
            printf("END TRANSPORT_SET_SO_TIMEOUT \n");
#endif
        break;

        //------------------------------------------------------
        // wn_mex_udp_transport('set_send_buf_size', handle, size)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - size (int)       - size of buffer (in bytes)
        //   - Returns:
        //     - none
        case TRANSPORT_SET_SEND_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SET_SEND_BUF_SIZE\n");
#endif        
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            size    = (int) mxGetScalar(prhs[2]);

            // Call function
            set_send_buffer_size( handle, size );

#ifdef _DEBUG_
            printf("END TRANSPORT_SET_SEND_BUF_SIZE \n");
#endif
        break;

        //------------------------------------------------------
        // size = wn_mex_udp_transport('get_send_buf_size', handle)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //   - Returns:
        //     - size (int)       - size of buffer (in bytes)
        case TRANSPORT_GET_SEND_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_GET_SEND_BUF_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }
            
            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);

            // Call function
            size = get_send_buffer_size( handle );
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_GET_SEND_BUF_SIZE \n");
#endif
        break;
        
        //------------------------------------------------------
        // wn_mex_udp_transport('set_rcvd_buf_size', handle, size)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - size (int)       - size of buffer (in bytes)
        //   - Returns:
        //     - none
        case TRANSPORT_SET_RCVD_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SET_RCVD_BUF_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            size    = (int) mxGetScalar(prhs[2]);

            // Call function
            set_receive_buffer_size( handle, size );
            
#ifdef _DEBUG_
            printf("END TRANSPORT_SET_RCVD_BUF_SIZE \n");
#endif
        break;
        
        //------------------------------------------------------
        // size = wn_mex_udp_transport('get_rcvd_buf_size', handle)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //   - Returns:
        //     - size (int)       - size of buffer (in bytes)
        case TRANSPORT_GET_RCVD_BUF_SIZE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_GET_RCVD_BUF_SIZE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }
            
            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);

            // Call function
            size = get_receive_buffer_size( handle );
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;
        
#ifdef _DEBUG_
            printf("END TRANSPORT_GET_RCVD_BUF_SIZE \n");
#endif
        break;

        //------------------------------------------------------
        // wn_mex_udp_transport('close', handle)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //   - Returns:
        //     - none
        case TRANSPORT_CLOSE :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_CLOSE\n");
#endif
            // Validate arguments
            if( nrhs != 2 ) { print_usage(); die(); }
            if( nlhs != 0 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);

            // Call function if socket not already closed
            //   NOTE:  When 'clear all' is executed in Matlab, it will
            //     call both the cleanup() function in the Mex that closes
            //     all sockets as well as the destroy function on the transport
            //     object that will call the wn_mex_udp_transport('close') function
            //     so we need to check if the socket is already closed to not
            //     print out an erroneous warning.
            if ( sockets[handle].handle != INVALID_SOCKET ) {
                close_socket( handle );
            }
        
#ifdef _DEBUG_
            printf("END TRANSPORT_CLOSE \n");
#endif
        break;

        //------------------------------------------------------
        // size = wn_mex_udp_transport('send', handle, buffer, length, ip_addr, port)
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - buffer (char *)  - Buffer of data to be sent
        //     - length (int)     - Length of data to be sent
        //     - ip_addr          - IP Address to send data to
        //     - port             - Port to send data to
        //   - Returns:
        //     - size (int)       - size of data sent (in bytes)
        case TRANSPORT_SEND :
#ifdef _DEBUG_
            printf("Function : TRANSPORT_SEND\n");
#endif
            // Validate arguments
            if( nrhs != 6 ) { print_usage(); die(); }
            if( nlhs != 1 ) { print_usage(); die(); }
            
            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            length  = (int) mxGetScalar(prhs[3]);
            port    = (int) mxGetScalar(prhs[5]);
            
            // Input must be a string 
            if ( mxIsChar( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input must be a string."); }
            if ( mxGetM( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input must be a row vector."); }
            ip_addr = mxArrayToString( prhs[4] );
            if( ip_addr == NULL ) { mexErrMsgTxt("Error:  Could not convert input to string."); }

#ifdef _DEBUG_
            printf("index = %d, length = %d, port = %d, ip_addr = %s \n", handle, length, port, ip_addr);
#endif
           
            // Input must be an array of uint8
            if ( mxIsUint8( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input must be an array of uint8"); }
            if ( mxGetM( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input must be a row vector."); }
            buffer = (char *) mxGetData( prhs[2] );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert input to array of char."); }
            
            // Call function
            size = send_socket( handle, buffer, length, ip_addr, port );
            
            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;

            mxFree( ip_addr );

#ifdef _DEBUG_
            print_buffer( buffer, length );
            printf("END TRANSPORT_SEND \n");
#endif
        break;
        
        //------------------------------------------------------
        // [size, buffer] = wn_mex_udp_transport('receive', handle, length )
        //   - Arguments:
        //     - handle (int)     - index to the requested socket
        //     - length (int)     - length of data to be received (in bytes)
        //   - Returns:
        //     - buffer (char *)  - Buffer of data received
        case TRANSPORT_RECEIVE :
                        
#ifdef _DEBUG_
            printf("Function : TRANSPORT_RECEIVE\n");
#endif
            // Validate arguments
            if( nrhs != 3 ) { print_usage(); die(); }
            if( nlhs != 2 ) { print_usage(); die(); }

            // Get input arguments
            handle  = (int) mxGetScalar(prhs[1]);
            length  = (int) mxGetScalar(prhs[2]);

#ifdef _DEBUG_
            printf("index = %d, length = %d \n", handle, length );
#endif

            // Allocate memory for the buffer
            buffer = (char *) malloc( sizeof( char ) * length );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not allocate receive buffer"); }
            for ( i = 0; i < length; i++ ) { buffer[i] = 0; }

            // Call function
            size = receive_socket( handle, length, buffer );            

            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;
            
            // Return value to MABLAB
            if ( size != 0 ) {
                plhs[1] = mxCreateNumericMatrix(size, 1, mxUINT8_CLASS, mxREAL);
                if( plhs[1] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            
                memcpy( (char *) mxGetData( plhs[1] ), buffer, size*sizeof(char) );
            } else {
                plhs[1] = mxCreateNumericMatrix(0, 0, mxUINT8_CLASS, mxREAL);            
            }

            // Free allocated memory
            free( buffer );
            
#ifdef _DEBUG_
            if ( size != 0 ) {
                printf("Buffer size = %d \n", size);
                print_buffer( (char *) mxGetData( plhs[1] ), size );
            }
            printf("END TRANSPORT_RECEIVE \n");
#endif
        break;


        //------------------------------------------------------
        //[num_samples, cmds_used, samples]  = wn_mex_udp_transport('read_rssi' / 'read_iq', 
        //                                        handle, buffer, length, ip_addr, port,
        //                                        number_samples, buffer_id, start_sample);
        //   - Arguments:
        //     - handle       (int)         - index to the requested socket
        //     - buffer       (char *)      - Buffer of data to be sent
        //     - length       (int)         - Length of data to be sent
        //     - ip_addr      (char *)      - IP Address to send data to
        //     - port         (int)         - Port to send data to
        //     - num_samples  (int)         - Number of samples requested
        //     - buffer_id    (int)         - WARP RF buffer to obtain samples from
        //     - start_sample (int)         - Starting address in the array for the samples
        //   - Returns:
        //     - num_samples  (int)         - Number of samples received
        //     - cmds_used    (int)         - Number of transport commands used to obtain samples
        //     - samples      (double *)    - Array of samples received
        case TRANSPORT_READ_IQ:
        case TRANSPORT_READ_RSSI:

#ifdef _DEBUG_
            printf("Function : TRANSPORT_READ_IQ \ TRANSPORT_READ_RSSI\n");
#endif
            // Validate arguments
            if( nrhs != 11 ) { print_usage(); die(); }
            if( nlhs !=  3 ) { print_usage(); die(); }

            // Get input arguments
            handle       = (int) mxGetScalar(prhs[1]);
            length       = (int) mxGetScalar(prhs[3]);
            port         = (int) mxGetScalar(prhs[5]);
            num_samples  = (int) mxGetScalar(prhs[6]);
            buffer_id    = (int) mxGetScalar(prhs[7]);
            start_sample = (int) mxGetScalar(prhs[8]);
            max_length   = (int) mxGetScalar(prhs[9]);
            num_pkts     = (int) mxGetScalar(prhs[10]);
            
            
            // Packet data must be an array of uint8
            if ( mxIsUint8( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input must be an array of uint8"); }
            if ( mxGetM( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Input must be a row vector."); }
            buffer = (char *) mxGetData( prhs[2] );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert input to array of char."); }

            // IP address input must be a string 
            if ( mxIsChar( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input must be a string."); }
            if ( mxGetM( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: Input must be a row vector."); }
            ip_addr = mxArrayToString( prhs[4] );
            if( ip_addr == NULL ) { mexErrMsgTxt("Error:  Could not convert input to string."); }
            

#ifdef _DEBUG_
            // Print initial debug information
            printf("Function = %d \n", function);
            printf("index = %d, length = %d, port = %d, ip_addr = %s \n", handle, length, port, ip_addr);
            printf("num_sample = %d, start_sample = %d, buffer_id = %d \n", num_samples, start_sample, buffer_id);
            print_buffer( buffer, length );
#endif
            
            // Allocate memory and initialize the output buffer
            output_array      = (uint32 *) malloc( sizeof( uint32 ) * num_samples );
            if( output_array == NULL ) { mexErrMsgTxt("Error:  Could not allocate receive buffer"); }
            for ( i = 0; i < num_samples; i++ ) { output_array[i] = 0; }

            
            // Set the useful RX buffer size to 90% of the RX buffer
            //     NOTE:  This is integer division so the rx_buffer_size will be truncated by the divide
            //
            useful_rx_buffer_size  = 9 * ( rx_buffer_size / 10 );
            

            // Update the buffer with the correct command arguments since it is too expensive to do in MATLAB
            command_args    = (uint32 *) ( buffer + sizeof( wn_transport_header ) + sizeof( wn_command_header ) );
            command_args[0] = endian_swap_32( buffer_id );
            command_args[3] = endian_swap_32( max_length );

                
            // Check to see if we have enough receive buffer space for the requested packets.
            // If not, then break the request up in to multiple requests.
            
            if( num_samples < ( useful_rx_buffer_size >> 2 ) ) {

                // Call receive function normally
                command_args[1] = endian_swap_32( start_sample );
                command_args[2] = endian_swap_32( num_samples );
                command_args[4] = endian_swap_32( num_pkts );

                // Call function
                size = wn_read_baseband_buffer( handle, buffer, length, ip_addr, port,
                                                num_samples, start_sample, buffer_id,
                                                output_array, &num_cmds );

            } else {

                // Since we are requesting more data than can fit in to the receive buffer, break this 
                // request in to multiple function calls, so we do not hit the timeout functions

                // Number of packets that can fit in the receive buffer
                num_pkts_to_request     = useful_rx_buffer_size / max_length;            // RX buffer size in bytes / Max packet size in bytes
                
                // Number of samples in a request (number of samples in a packet * number of packets in a request)
                num_samples_to_request  = (max_length >> 2) * num_pkts_to_request;
                
                // Starting sample
                start_sample_to_request = start_sample;

                // Error checking to make sure something bad did not happen
                if ( num_pkts_to_request > num_pkts ) {
                    printf("ERROR:  Read IQ / Read RSSI - Parameter mismatch \n");
                    printf("    Requested %d packet(s) and %d sample(s) in function call.  \n", num_pkts, num_samples);
                    printf("    Receive buffer can hold %d samples (ie %d packets).  \n", num_samples_to_request, num_pkts_to_request);
                    printf("    Since, the number of samples requested is greater than what the receive buffer can hold, \n");
                    printf("    the number of packets requested should be greater than what the receive buffer can hold. \n");
                    die_with_error("Error:  Read IQ / Read RSSI - Parameter mismatch.  See above for debug information.");
                }
                
                command_args[2] = endian_swap_32( num_samples_to_request );
                command_args[4] = endian_swap_32( num_pkts_to_request );

                for( i = num_pkts; i > 0; i -= num_pkts_to_request ) {

                    int j = i - num_pkts_to_request;
                
                    // If we are requesting the last set of packets, then just request the remaining samples
                    if ( j < 0 ) {
                        num_samples_to_request = num_samples - ( (num_pkts - i) * (max_length >> 2) );
                        command_args[2]        = endian_swap_32( num_samples_to_request );
                        
                        num_pkts_to_request    = i;
                        command_args[4]        = endian_swap_32( num_pkts_to_request );
                    }

                    command_args[1] = endian_swap_32( start_sample_to_request );

                    // Call function
                    size = wn_read_baseband_buffer( handle, buffer, length, ip_addr, port,
                                                    num_samples_to_request, start_sample_to_request, buffer_id,
                                                    output_array, &num_cmds );
                    
                    start_sample_to_request += num_samples_to_request;                    
                }
                
                size = num_samples;
            }


            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = size;
            
            plhs[1] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[1]) = num_cmds;

            if ( size != 0 ) {

                // Process returned output array
                if ( function == TRANSPORT_READ_IQ ) {

                    plhs[2] = mxCreateDoubleMatrix(size, 1, mxCOMPLEX);
                    if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }

                    // Need to unpack the WARPNet sample
                    //   NOTE:  This performs a conversion from an UFix_16_0 to a Fix_14_13 
                    //      (in WARPv3, we convert Fix_12_11 to Fix_14_13 by zeroing out the two LSBs)
                    //      Process:
                    //          1) Mask upper two bits
                    //          2) Sign exten the value so you have a true twos compliment 16 bit value
                    //          3) Divide by range / 2 to move the decimal point so resulting value is between +/- 1
                    for ( i = 0; i < size; i++ ) {
                        // I samples
                        temp_val = (double) ((int16) (((output_array[i] >> 16) & 0x3FFF) | 
                                                      (((output_array[i] >> 29) & 0x1) * 0xC000)));
                        mxGetPr(plhs[2])[i] = ( temp_val / 0x2000 );
                        
                        // Q samples
                        temp_val = (double) ((int16) ((output_array[i]        & 0x3FFF) | 
                                                      (((output_array[i] >> 13) & 0x1) * 0xC000)));
                        mxGetPi(plhs[2])[i] = ( temp_val / 0x2000 );                        
                    }
                    
                } else { // TRANSPORT_READ_RSSI

                    plhs[2] = mxCreateDoubleMatrix((2*size), 1, mxREAL);
                    if( plhs[2] == NULL ) { mexErrMsgTxt("Error:  Could not allocate return buffer"); }            
                
                    for ( i = 0; i < (2*size); i += 2 ) {
                        mxGetPr(plhs[2])[i]     = (double)((output_array[(i/2)] >> 16) & 0x03FF);
                        mxGetPr(plhs[2])[i + 1] = (double) (output_array[(i/2)]        & 0x03FF);
                    }
                }
            } else {
                // Return an empty array
                plhs[2] = mxCreateDoubleMatrix(0, 0, mxCOMPLEX);
            }

            
            // Free allocated memory
            free( ip_addr );
            free( output_array );
            
#ifdef _DEBUG_
            printf("END TRANSPORT_READ_IQ \ TRANSPORT_READ_RSSI\n");
#endif        
        break;        

        //------------------------------------------------------
        // cmds_used = wn_mex_udp_transport('write_iq', handle, cmd_buffer, max_length, ip_addr, port, 
        //                                          number_samples, sample_buffer, buffer_id, start_sample, num_pkts, max_samples, hw_ver);
        //
        //   - Arguments:
        //     - handle          (int)      - Index to the requested socket
        //     - cmd_buffer      (char *)   - Buffer of data to be used as the header for Write IQ command
        //     - max_length      (int)      - Length of max data packet (in bytes)
        //     - ip_addr         (char *)   - IP Address to send samples to
        //     - port            (int)      - Port to send samples to
        //     - num_samples     (int)      - Number of samples to send
        //     - sample_I_buffer (uint16 *) - Array of I samples to be sent
        //     - sample_Q_buffer (uint16 *) - Array of Q samples to be sent
        //     - buffer_id       (int)      - WARP RF buffer to send samples to
        //     - start_sample    (int)      - Starting address in the array of samples
        //     - num_pkts        (int)      - Number of Ethernet packets it should take to send the samples
        //     - max_samples     (int)      - Max number of samples transmitted per packet
        //                                    (last packet may not send max_sample samples)
        //     - hw_ver          (int)      - Hardware version;  Since different HW versions have different 
        //                                    processing capbilities, we need to know the HW version for timing
        //   - Returns:
        //     - cmds_used   (int)  - number of transport commands used to send samples
        case TRANSPORT_WRITE_IQ:

#ifdef _DEBUG_
            printf("Function : TRANSPORT_WRITE_IQ\n");
#endif
            // Validate arguments
            if( nrhs != 14 ) { print_usage(); die(); }
            if( nlhs !=  1 ) { print_usage(); die(); }

            // Get input arguments
            handle       = (int) mxGetScalar(prhs[1]);
            max_length   = (int) mxGetScalar(prhs[3]);
            port         = (int) mxGetScalar(prhs[5]);
            num_samples  = (int) mxGetScalar(prhs[6]);
            buffer_id    = (int) mxGetScalar(prhs[9]);
            start_sample = (int) mxGetScalar(prhs[10]);
            num_pkts     = (int) mxGetScalar(prhs[11]);
            max_samples  = (int) mxGetScalar(prhs[12]);
            hw_ver       = (int) mxGetScalar(prhs[13]);

            // Packet data must be an array of uint8
            if ( mxIsUint8( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Command Buffer input must be an array of uint8"); }
            if ( mxGetM( prhs[2] ) != 1 ) { mexErrMsgTxt("Error: Command Buffer input must be a row vector."); }
            buffer = (char *) mxGetData( prhs[2] );
            if( buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert command buffer input to array of char."); }

            // IP address input must be a string 
            if ( mxIsChar( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: IP Address input must be a string."); }
            if ( mxGetM( prhs[4] ) != 1 ) { mexErrMsgTxt("Error: IP Address input must be a row vector."); }
            ip_addr = mxArrayToString( prhs[4] );
            if( ip_addr == NULL ) { mexErrMsgTxt("Error:  Could not convert ip address input to string."); }
            
            // Sample I Buffer
            if ( mxGetN( prhs[7] ) != 1 ) { mexErrMsgTxt("Error: Sample buffer input must be a column vector."); }
            sample_I_buffer = (uint16 *) mxGetData( prhs[7] );
            if( sample_I_buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert sample buffer input to array"); }
            
            // Sample I Buffer
            if ( mxGetN( prhs[8] ) != 1 ) { mexErrMsgTxt("Error: Sample buffer input must be a column vector."); }
            sample_Q_buffer = (uint16 *) mxGetData( prhs[8] );
            if( sample_Q_buffer == NULL ) { mexErrMsgTxt("Error:  Could not convert sample buffer input to array"); }

            // NOTE:  Due to differences in rounding between Matlab and C when converting from double to int16
            //   we must preprocess the sample array so that it contains a uint32 consisting of the I and Q 
            //   for a given sample.  The code below can demonstrate the rounding difference between Matlab and C
            //  
            //
            // if ( mxIsComplex( prhs[7] ) != 1 ) { mexErrMsgTxt("Error: Sample Buffer input must be an array of complex doubles"); }            
            // if ( mxGetN( prhs[7] ) != 1 ) { mexErrMsgTxt("Error: Sample buffer input must be a column vector."); }
            //
            // temp_i = (double *) mxGetPr(prhs[7]);
            // temp_q = (double *) mxGetPi(prhs[7]);
            //
            // sample_buffer = (uint32 *) malloc( sizeof( uint32 ) * num_samples );
            // if( sample_buffer == NULL ) { mexErrMsgTxt("Error:  Could not allocate sample buffer"); }
            // for ( i = 0; i < num_samples; i++ ) { 
            //     sample_buffer[i] = (uint32)( ((int16)( temp_i[i] * (1 << 15) )) << 16 ) + ((int16)( temp_q[i] * ( 1 << 15 ) )); 
            // }
            // 

#ifdef _DEBUG_
            // Print initial debug information
            printf("Function = %d \n", function);
            printf("index = %d, length = %d, port = %d, ip_addr = %s \n", handle, length, port, ip_addr);
            printf("num_sample = %d, start_sample = %d, buffer_id = %d \n", num_samples, start_sample, buffer_id);
            printf("num_pkts = %d, max_samples = %d \n", num_pkts, max_samples);
            printf("wn_transport_header = %d, wn_command_header = %d, wn_sample_header = %d \n", 
                   sizeof( wn_transport_header ), sizeof( wn_command_header ), sizeof( wn_sample_header ));
            print_buffer( buffer, ( sizeof( wn_transport_header ) + sizeof( wn_command_header ) )  );
            print_buffer_32( sample_buffer, num_samples );
#endif
                        
            // Call function
            size = wn_write_baseband_buffer( handle, buffer, max_length, ip_addr, port,
                                             num_samples, start_sample, sample_I_buffer, sample_Q_buffer, buffer_id, num_pkts, max_samples, hw_ver,
                                             &num_cmds );

            // Return value to MABLAB
            plhs[0] = mxCreateDoubleMatrix(1,1,mxREAL);
            *mxGetPr(plhs[0]) = num_cmds;
            
            if ( size == 0 ) {
                mexErrMsgTxt("Error:  Did not send any samples");
            }
            
            // Free allocated memory
            free( ip_addr );
            
#ifdef _DEBUG_
            printf("END TRANSPORT_WRITE_IQ\n");
#endif        
        break;

        
        //------------------------------------------------------
        //  Default
        //
        //  Print error message
        //
        default:
            printf("Error:  Function %s not supported.", func);
            mexErrMsgTxt("Error:  Function not supported.");
        break; 
    }

    // Free allocated memory
    mxFree( func );
    
#ifdef _DEBUG_
    printf("DONE \n");
#endif

    // Return back to MATLAB
    return;
}




/*****************************************************************************/
//           Additional WL Mex UDP Transport functionality
/*****************************************************************************/




/*****************************************************************************/
/**
*
* This function will read the baseband buffers and construct the sample array
*
* @param	index          - Index in to socket structure which will receive samples
* @param	buffer         - WARPNet command to request samples
* @param	length         - Length (in bytes) of buffer
* @param    ip_addr        - IP Address of node to retrieve samples
* @param    port           - Port of node to retrieve samples
* @param    num_samples    - Number of samples to process (should be the same as the argument in the WARPNet command)
* @param    start_sample   - Index of starting sample (should be the same as the agrument in the WARPNet command)
* @param    buffer_id      - Which buffer(s) do we need to retrieve samples from
* @param    output_array   - Return parameter - array of samples to return
* @param    num_cmds       - Return parameter - number of ethernet send commands used to request packets 
*                                (could be > 1 if there are transmission errors)
*
* @return	size           - Number of samples processed (also size of output_array)
*
* @note		This function requires the following pointers:
*   Input:
*       - Buffer containing command to request samples (char *)
*       - IP Address (char *)
*       - Array of buffer IDs (uint32 *)
*       - Allocated memory to return samples (uint32 *)
*       - Location to return the number of send commands used to generate the samples (uint32 *)
*
*   Output:
*       - Number of samples processed 
*
******************************************************************************/
int wn_read_baseband_buffer( int index, 
                             char *buffer, int length, char *ip_addr, int port,
                             int num_samples, int start_sample, uint32 buffer_id,
                             uint32 *output_array, uint32 *num_cmds ) {

    // Variable declaration
    int i;
    int                   done               = 0;
    
    uint32                buffer_id_cmd      = 0;
    uint32                start_sample_cmd   = 0;
    uint32                total_sample_cmd   = 0;
    uint32                bytes_per_pkt      = 0;
    uint32                num_pkts           = 0;
    
    uint32                output_buffer_size = 0;
    uint32                samples_per_pkt    = 0;

    int                   sent_size          = 0;

    uint32                rcvd_pkts          = 0;
    int                   rcvd_size          = 0;
    int                   num_rcvd_samples   = 0;
    int                   sample_num         = 0;
    int                   sample_size        = 0;
    
    uint32                timeout            = 0;
    uint32                num_retrys         = 0;

    uint32                total_cmds         = 0;
    
    uint32                err_start_sample   = 0;
    uint32                err_num_samples    = 0;
    uint32                err_num_pkts       = 0;

    
    char                 *output_buffer;
    uint8                *samples;

    wn_transport_header  *transport_hdr;
    wn_command_header    *command_hdr;
    uint32               *command_args;
    wn_sample_header     *sample_hdr;
    wn_sample_tracker    *sample_tracker;

    // Compute some constants to be used later
    uint32                tport_hdr_size    = sizeof( wn_transport_header );
    uint32                cmd_hdr_size      = sizeof( wn_transport_header ) + sizeof( wn_command_header );
    uint32                all_hdr_size      = sizeof( wn_transport_header ) + sizeof( wn_command_header ) + sizeof( wn_sample_header );


    // Initialization
    transport_hdr  = (wn_transport_header *) buffer;
    command_hdr    = (wn_command_header   *) ( buffer + tport_hdr_size );
    command_args   = (uint32              *) ( buffer + cmd_hdr_size );
    
    buffer_id_cmd    = endian_swap_32( command_args[0] );
    start_sample_cmd = endian_swap_32( command_args[1] );
    total_sample_cmd = endian_swap_32( command_args[2] );
    bytes_per_pkt    = endian_swap_32( command_args[3] );            // Command contains payload size; add room for header
    num_pkts         = endian_swap_32( command_args[4] );

    output_buffer_size = bytes_per_pkt + 100;                        // Command contains payload size; add room for header
    samples_per_pkt    = ( bytes_per_pkt >> 2 );                     // Each WARPNet sample is 4 bytes
    
    
#ifdef _DEBUG_
    // Print command arguments    
    printf("index = %d, length = %d, port = %d, ip_addr = %s \n", index, length, port, ip_addr);
    printf("num_sample = %d, start_sample = %d, buffer_id = %d \n", num_samples, start_sample, buffer_id);
    printf("bytes_per_pkt = %d;  num_pkts = %d \n", bytes_per_pkt, num_pkts );
    print_buffer( buffer, length );
#endif

    // Perform a consistency check to make sure parameters are correct
    if ( buffer_id_cmd != buffer_id ) {
        printf("WARNING:  Buffer ID in command (%d) does not match function parameter (%d)\n", buffer_id_cmd, buffer_id);
    }
    if ( start_sample_cmd != start_sample ) {
        printf("WARNING:  Starting sample in command (%d) does not match function parameter (%d)\n", start_sample_cmd, start_sample);
    }
    if ( total_sample_cmd != num_samples ) {
        printf("WARNING:  Number of samples requested in command (%d) does not match function parameter (%d)\n", total_sample_cmd, num_samples);
    }

    
    // Malloc temporary buffer to process ethernet packets
    output_buffer  = (char *) malloc( sizeof( char ) * output_buffer_size );
    if( output_buffer == NULL ) { die_with_error("Error:  Could not allocate temp output buffer"); }
    
    // Malloc temporary array to track samples that have been received and initialize
    sample_tracker = (wn_sample_tracker *) malloc( sizeof( wn_sample_tracker ) * num_pkts );
    if( sample_tracker == NULL ) { die_with_error("Error:  Could not allocate sample tracker buffer"); }
    for ( i = 0; i < num_pkts; i++ ) { sample_tracker[i].start_sample = 0;  sample_tracker[i].num_samples = 0; }

    
    // Send packet to request samples
    sent_size   = send_socket( index, buffer, length, ip_addr, port );
    total_cmds += 1;

    if ( sent_size != length ) {
        die_with_error("Error:  Size of packet sent to request samples does not match length of packet.");
    }


    // Initialize loop variables
    rcvd_pkts = 0;
    timeout   = 0;
    
    // Process each return packet
    while ( !done ) {
        
        // If we hit the timeout, then try to re-request the remaining samples
        if ( timeout >= TRANSPORT_TIMEOUT ) {
        
            // If we hit the max number of retrys, then abort
            if ( num_retrys >= TRANSPORT_MAX_RETRY ) {

                printf("ERROR:  Exceeded %d retrys for current Read IQ / Read RSSI request \n", TRANSPORT_MAX_RETRY);
                printf("    Requested %d samples from buffer %d starting from sample number %d \n", num_samples, buffer_id, start_sample);
                printf("    Received %d out of %d packets from node before timeout.\n", rcvd_pkts, num_pkts);
                printf("    Please check the node and look at the ethernet traffic to isolate the issue. \n");                
            
                die_with_error("Error:  Reached maximum number of retrys without a response... aborting.");
                
            } else {

                // NOTE:  We print a warning here because the Read IQ / Read RSSI case in the mex function above
                //        will split Read IQ / Read RSSI requests based on the receive buffer size.  Therefore,
                //        any timeouts we receive here should be legitmate issues that should be explored.
                //
                printf("WARNING:  Read IQ / Read RSSI request timed out.  Retrying remaining samples. \n");
            
                // Find the first packet error and request the remaining samples
                if ( wn_read_iq_find_error( sample_tracker, num_samples, start_sample, rcvd_pkts, samples_per_pkt,
                                            &err_num_samples, &err_start_sample, &err_num_pkts ) ) {
                    
                    command_args[1] = endian_swap_32( err_start_sample );
                    command_args[2] = endian_swap_32( err_num_samples );
                    command_args[4] = endian_swap_32( num_pkts - ( rcvd_pkts - err_num_pkts ) );

                    // Since there was an error in the packets we have already received, then we need to adjust rcvd_pkts
                    rcvd_pkts        -= err_num_pkts;
                    num_rcvd_samples  = num_samples - err_num_samples;
                } else {
                    // If we did not find an error, then the first rcvd_pkts are correct and we should request
                    //   the remaining packets
                    command_args[1] = endian_swap_32( err_start_sample );
                    command_args[2] = endian_swap_32( err_num_samples );
                    command_args[4] = endian_swap_32( num_pkts - rcvd_pkts );
                }

                // Retransmit the read IQ request packet
                sent_size   = send_socket( index, buffer, length, ip_addr, port );
                
                if ( sent_size != length ) {
                    die_with_error("Error:  Size of packet sent to request samples does not match length of packet.");
                }
                
                // Update control variables
                timeout     = 0;
                total_cmds += 1;
                num_retrys += 1;
            }
        }
        
        // Recieve packet
        rcvd_size = receive_socket( index, output_buffer_size, output_buffer );

        // recevie_socket() handles all socket related errors and will only return:
        //   - zero if no packet is available
        //   - non-zero if packet is available
        if ( rcvd_size > 0 ) {
            sample_hdr  = (wn_sample_header *) ( output_buffer + cmd_hdr_size );
            samples     = (uint8 *) ( output_buffer + all_hdr_size );
            sample_num  = endian_swap_32( sample_hdr->start );
            sample_size = endian_swap_32( sample_hdr->num_samples );

#ifdef _DEBUG_
            printf("num_sample = %d, start_sample = %d \n", sample_size, sample_num);
#endif

            // If we are tracking packets, record which samples have been recieved
            sample_tracker[rcvd_pkts].start_sample = sample_num;
            sample_tracker[rcvd_pkts].num_samples  = sample_size;
            
            // Place samples in the array (Ethernet packet is uint8, output array is uint32) 
            //   NOTE: Need to pack samples in the correct order
            for( i = 0; i < (4 * sample_size); i += 4 ) {
                output_array[ sample_num + (i / 4) ] = (uint32) ( (samples[i    ] << 24) | 
                                                                  (samples[i + 1] << 16) | 
                                                                  (samples[i + 2] <<  8) | 
                                                                  (samples[i + 3]      ) );
            }
            
            num_rcvd_samples += sample_size;
            rcvd_pkts        += 1;
            timeout           = 0;

            // Exit the loop when we have enough packets
            if ( rcvd_pkts == num_pkts ) {
            
                // Check to see if we have any packet errors
                //     NOTE:  This check will detect duplicate packets or sample indexing errors
                if ( wn_read_iq_sample_error( sample_tracker, num_samples, start_sample, rcvd_pkts, samples_per_pkt ) ) {

                    // In this case, there is probably some issue in the transmitting node not getting the
                    // correct number of samples or messing up the indexing of the transmit packets.  
                    // The wn_read_iq_sample_error() printed debug information, so we need to retry the packets
                    
                    if ( num_retrys >= TRANSPORT_MAX_RETRY ) {
                    
                        die_with_error("Error:  Errors in sample request from board.  Max number of re-transmissions reached.  See above for debug information.");
                        
                    } else {

                        // Find the first packet error and request the remaining samples
                        if ( wn_read_iq_find_error( sample_tracker, num_samples, start_sample, rcvd_pkts, samples_per_pkt,
                                                    &err_num_samples, &err_start_sample, &err_num_pkts ) ) {

                            command_args[1] = endian_swap_32( err_start_sample );
                            command_args[2] = endian_swap_32( err_num_samples );
                            command_args[4] = endian_swap_32( err_num_pkts );

                            // Retransmit the read IQ request packet
                            sent_size   = send_socket( index, buffer, length, ip_addr, port );
                            
                            if ( sent_size != length ) {
                                die_with_error("Error:  Size of packet sent to request samples does not match length of packet.");
                            }
                            
                            // We are re-requesting err_num_pkts, so we need to subtract err_num_pkts from what we have already recieved
                            rcvd_pkts        -= err_num_pkts;
                            num_rcvd_samples  = num_samples - err_num_samples;

                            // Update control variables
                            timeout     = 0;
                            total_cmds += 1;
                            num_retrys += 1;
                        
                        } else {
                            // Die since we could not find the error
                            die_with_error("Error:  Encountered error in sample packets but could not determine the error.  See above for debug information.");
                        }
                    }
                } else {

                    done = 1;
                }
            }
            
        } else {
        
            // Increment the timeout counter
            timeout += 1;            
            
        }  // END if ( rcvd_size > 0 )
        
    }  // END while( !done )

    
    // Free locally allocated memory    
    free( output_buffer );    
    free( sample_tracker ); 

    // Finalize outputs   
    *num_cmds  += total_cmds;
    
    return num_rcvd_samples;
}



/*****************************************************************************/
/**
*  Function:  Read IQ sample check
*
*  Function to check if we received all the samples at the correct indecies
*
*  Returns:  0 if no errors
*            1 if if there is an error and prints debug information
*
******************************************************************************/
int wn_read_iq_sample_error( wn_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size ) {

    int i;
    unsigned int start_sample_total;

    unsigned int num_samples_sum  = 0;
    unsigned int start_sample_sum = 0;

    // Compute the value of the start samples:
    //   We know that the start samples should follow the pattern:
    //       [ x, (x + y), (x + 2y), (x + 3y), ... , (x + (N - 1)y) ]
    //   where x = start_sample, y = max_sample_size, and N = num_pkts.  This is due
    //   to the fact that the node will fill all packets completely except the last packet.
    //   Therefore, the sum of all element in that array is:
    //       (N * x) + ((N * (N -1 ) * Y) / 2

    start_sample_total = (num_pkts * start_sample) + (((num_pkts * (num_pkts - 1)) * max_sample_size) >> 1); 
    
    // Compute the totals using the sample tracker
    for( i = 0; i < num_pkts; i++ ) {
    
        num_samples_sum  += tracker[i].num_samples;
        start_sample_sum += tracker[i].start_sample;
    }

    // Check the totals
    if ( ( num_samples_sum != num_samples ) || ( start_sample_sum != start_sample_total ) ) {
    
        // Print warning debug information if there is an error
        if ( num_samples_sum != num_samples ) { 
            printf("WARNING:  Number of samples received (%d) does not equal number of samples requested (%d).  \n", num_samples_sum, num_samples);
        } else {
            printf("WARNING:  Sample packet indecies not correct.  Expected the sum of sample indecies to be (%d) but received a sum of (%d).  Retrying ...\n", start_sample_total, start_sample_sum);
        }
        
        // Print debug information
        printf("Packet Tracking Information: \n");
        printf("    Requested Samples:  Number: %8d    Start Sample: %8d  \n", num_samples, start_sample);
        printf("    Received  Samples:  Number: %8d  \n", num_samples_sum);
        
        for ( i = 0; i < num_pkts; i++ ) {
            printf("         Packet[%4d]:   Number: %8d    Start Sample: %8d  \n", i, tracker[i].num_samples, tracker[i].start_sample );
        }

        return 1;
    } else {
        return 0;
    }
}



/*****************************************************************************/
/**
*  Function:  Read IQ find first error
*
*  Function to check if we received all the samples at the correct indecies.
*  This will also update the ret_* parameters to indicate values to use when 
*  requesting a new set of packets.
*
*  Returns:  0 if no error was found
*            1 if an error was found
*
******************************************************************************/
int wn_read_iq_find_error( wn_sample_tracker *tracker, uint32 num_samples, uint32 start_sample, uint32 num_pkts, uint32 max_sample_size,
                           uint32 *ret_num_samples, uint32 *ret_start_sample, uint32 *ret_num_pkts ) {

    unsigned int i, j;

    unsigned int value_found;
    unsigned int return_value            = 1;
    unsigned int start_sample_to_request = start_sample;
    unsigned int num_samples_left        = num_samples;
    unsigned int num_pkts_left           = num_pkts;

    // Iterate thru the array to find the first index that does not match the expected value
    //     NOTE:  This is performing a naive search and could be optimized.  Since we are 
    //         already in an error condition, we chose simplicity over performance.
    
    for( i = 0; i < num_pkts; i++ ) {
    
        value_found = 0;
    
        // Find element in the array   
        for ( j = 0; j < num_pkts; j ++ ) {
            if ( start_sample_to_request == tracker[i].start_sample ) {
                value_found = 1;
            }            
        }

        // If we did not finde the value, then exit the loop
        if ( !value_found ) {
            break;
        }
        
        // Update element to search for
        start_sample_to_request += max_sample_size;
        num_samples_left        -= max_sample_size;
        num_pkts_left           -= 1;
    }

    // Set the return value
    if ( num_pkts_left == 0 ) {
        return_value  = 0;
    } 
        
    // Return parameters
    *ret_start_sample = start_sample_to_request;
    *ret_num_samples  = num_samples_left;
    *ret_num_pkts     = num_pkts_left;
    
    return return_value;
}



/*****************************************************************************/
/**
* This function will write the baseband buffers 
*
* @param	index          - Index in to socket structure which will receive samples
* @param	buffer         - WARPNet command (includes transport header and command header)
* @param	length         - Length (in bytes) max data packet to send (Ethernet MTU size - Ethernet header)
* @param    ip_addr        - IP Address of node to retrieve samples
* @param    port           - Port of node to retrieve samples
* @param    num_samples    - Number of samples to process (should be the same as the argument in the WARPNet command)
* @param    start_sample   - Index of starting sample (should be the same as the agrument in the WARPNet command)
* @param    samples_i      - Array of I samples to be sent
* @param    samples_q      - Array of Q samples to be sent
* @param    buffer_id      - Which buffer(s) do we need to send samples to (all dimensionality of buffer_ids is handled by Matlab)
* @param    num_pkts       - Number of packets to transfer (precomputed by calling SW)
* @param    max_samples    - Max samples to send per packet (precomputed by calling SW)
* @param    hw_ver         - Hardware version of node
* @param    num_cmds       - Return parameter - number of ethernet send commands used to request packets 
*                                (could be > 1 if there are transmission errors)
*
* @return	samples_sent   - Number of samples processed 
*
* @note		This function requires the following pointers:
*   Input:
*       - Buffer containing command header to send samples (char *)
*       - IP Address (char *)
*       - Buffer containing the samples to be sent (uint32 *)
*       - Location to return the number of send commands used to generate the samples (uint32 *)
*
*   Output:
*       - Number of samples processed 
*
******************************************************************************/
int wn_write_baseband_buffer( int index, 
                              char *buffer, int max_length, char *ip_addr, int port,
                              int num_samples, int start_sample, uint16 *samples_i, uint16 *samples_q, uint32 buffer_id,
                              int num_pkts, int max_samples, int hw_ver, uint32 *num_cmds ) {

    // Variable declaration
    int i, j;
    int                   done              = 0;
    int                   length            = 0;
    int                   sent_size         = 0;
    int                   sample_num        = 0;
    int                   offset            = 0;
    int                   need_resp         = 0;
    int                   slow_write        = 0;
    uint16                transport_flags   = 0;
    uint32                timeout           = 0;
    int                   buffer_count      = 0;

    // Packet checksum tracking
    uint32                checksum          = 0;
    uint32                node_checksum     = 0;

    // Keep track of packet sequence number
    uint16                seq_num           = 0;
    uint16                seq_start_num     = 0;

    // Receive Buffer variables
    int                   rcvd_size         = 0;
    int                   rcvd_max_size     = 100;
    int                   num_retrys        = 0;
    unsigned char        *rcvd_buffer;
    uint32               *command_args;

    // Buffer of data for ethernet packet and pointers to components of the packet
    unsigned char        *send_buffer;
    wn_transport_header  *transport_hdr;
    wn_command_header    *command_hdr;
    wn_sample_header     *sample_hdr;
    uint32               *sample_payload;

    // Sleep timer
    uint32                wait_time;
        
    // Compute some constants to be used later
    uint32                tport_hdr_size    = sizeof( wn_transport_header );
    uint32                tport_hdr_size_np = sizeof( wn_transport_header ) - TRANSPORT_PADDING_SIZE;
    uint32                cmd_hdr_size      = sizeof( wn_transport_header ) + sizeof( wn_command_header );
    uint32                cmd_hdr_size_np   = sizeof( wn_transport_header ) + sizeof( wn_command_header ) - TRANSPORT_PADDING_SIZE;
    uint32                all_hdr_size      = sizeof( wn_transport_header ) + sizeof( wn_command_header ) + sizeof( wn_sample_header );
    uint32                all_hdr_size_np   = sizeof( wn_transport_header ) + sizeof( wn_command_header ) + sizeof( wn_sample_header ) - TRANSPORT_PADDING_SIZE;


#ifdef _DEBUG_
    // Print command arguments    
    printf("index = %d, length = %d, port = %d, ip_addr = %s \n", index, length, port, ip_addr);
    printf("num_sample = %d, start_sample = %d, buffer_id = %d \n", num_samples, start_sample, buffer_id);
    printf("num_pkts = %d, max_samples = %d \n", num_pkts, max_samples);
#endif

    // Initialization

    // Malloc temporary buffer to receive ethernet packets    
    rcvd_buffer  = (unsigned char *) malloc( sizeof( char ) * rcvd_max_size );
    if( rcvd_buffer == NULL ) { die_with_error("Error:  Could not allocate temp receive buffer"); }

    // Malloc temporary buffer to process ethernet packets    
    send_buffer  = (unsigned char *) malloc( sizeof( char ) * max_length );
    if( send_buffer == NULL ) { die_with_error("Error:  Could not allocate temp send buffer"); }
    for( i = 0; i < cmd_hdr_size; i++ ) { send_buffer[i] = buffer[i]; }     // Copy current header to send buffer 

    // Set up pointers to all the pieces of the ethernet packet    
    transport_hdr  = (wn_transport_header *) send_buffer;
    command_hdr    = (wn_command_header   *) ( send_buffer + tport_hdr_size );
    sample_hdr     = (wn_sample_header    *) ( send_buffer + cmd_hdr_size   );
    sample_payload = (uint32              *) ( send_buffer + all_hdr_size   );

    // Get necessary values from the packet buffer so we can send multiple packets
    seq_num         = endian_swap_16( transport_hdr->seq_num ) + 1;    // Current sequence number is from the last packet
    transport_flags = endian_swap_16( transport_hdr->flags );

    // Initialize loop variables
    slow_write    = 0;
    need_resp     = 0;
    offset        = start_sample;
    seq_start_num = seq_num;
    
    // For each packet
    for( i = 0; i < num_pkts; i++ ) {
    
        // Determine how many samples we need to send in the packet
        if ( ( offset + max_samples ) <= num_samples ) {
            sample_num = max_samples;
        } else {
            sample_num = num_samples - offset;
        }

        // Determine the length of the packet (All WARPNet payload minus the padding for word alignment)
        length = all_hdr_size_np + (sample_num * sizeof( uint32 ));

        // Request that the board respond to the last packet or all packets if slow_write = 1
        if ( ( i == ( num_pkts - 1 ) ) || ( slow_write == 1 ) ) {
            need_resp       = 1;
            transport_flags = transport_flags | TRANSPORT_FLAG_ROBUST;
        } else {
            need_resp       = 0;
            transport_flags = transport_flags & ~TRANSPORT_FLAG_ROBUST;        
        }

        // Prepare transport
        //   NOTE:  The length of the packet is the maximum payload size supported by the transport.  However, the 
        //       maximum payload size returned by the node has the two byte Ethernet padding already subtracted out, so the 
        //       length of the transport command is the length minus the transport header plus the padding size, since
        //       we do not want to double count the padding.
        //
        transport_hdr->length   = endian_swap_16( length - tport_hdr_size_np );
        transport_hdr->seq_num  = endian_swap_16( seq_num );
        transport_hdr->flags    = endian_swap_16( transport_flags );
        
        // Prepare command
        //   NOTE:  See above comment about lenght.  Since there is one sample packet per command, we set the number
        //       number of command arguments to be 1.
        //
        command_hdr->length     = endian_swap_16( length - cmd_hdr_size_np );
        command_hdr->num_args   = endian_swap_16( 0x0001 );
        
        // Prepare sample packet
        sample_hdr->buffer_id   = endian_swap_16( buffer_id );
        if ( i == 0 ) {
            sample_hdr->flags       = SAMPLE_CHKSUM_RESET;
        } else {
            sample_hdr->flags       = SAMPLE_CHKSUM_NOT_RESET;        
        }
        sample_hdr->start       = endian_swap_32( offset );
        sample_hdr->num_samples = endian_swap_32( sample_num );

        // Copy the appropriate samples to the packet
        for( j = 0; j < sample_num; j++ ) {
            sample_payload[j] = endian_swap_32( ( samples_i[j + offset] << 16 ) + samples_q[j + offset] );    
        }

        // Add back in the padding so we can send the packet
        length += TRANSPORT_PADDING_SIZE;

        // Send packet 
        sent_size = send_socket( index, (char *) send_buffer, length, ip_addr, port );

        if ( sent_size != length ) {
            die_with_error("Error:  Size of packet sent to with samples does not match length of packet.");
        }
        
        // Update loop variables
        offset   += sample_num;
        seq_num  += 1;

        // Compute checksum
        // NOTE:  Due to a weakness in the Fletcher 32 checksum (ie it cannot distinguish between
        //     blocks of all 0 bits and blocks of all 1 bits), we need to add additional information
        //     to the checksum so that we will not miss errors on packets that contain data of all 
        //     zero or all one.  Therefore, we add in the start sample for each packet since that 
        //     is readily availalbe on the node.
        //
        if ( i == 0 ) {
            checksum = wn_update_checksum( ( ( offset - sample_num ) & 0xFFFF ), SAMPLE_CHKSUM_RESET ); 
        } else {
            checksum = wn_update_checksum( ( ( offset - sample_num ) & 0xFFFF ), SAMPLE_CHKSUM_NOT_RESET ); 
        }

        checksum = wn_update_checksum( ( samples_i[offset - 1] ^ samples_q[offset - 1] ), SAMPLE_CHKSUM_NOT_RESET );

        
        // If we need a response, then wait for it
        if ( need_resp == 1 ) {

            // Initialize loop variables
            timeout   = 0;
            done      = 0;
            rcvd_size = 0;
            
            // Process each return packet
            while ( !done ) {

                // If we hit the timeout, then try to re-transmit the packet
                if ( timeout >= TRANSPORT_TIMEOUT ) {
                
                    // If we hit the max number of retrys, then abort
                    if ( num_retrys >= TRANSPORT_MAX_RETRY ) {
                        die_with_error("Error:  Reached maximum number of retrys without a response... aborting.");                    
                    } else {
                        // Roll everything back and retransmit the packet
                        num_retrys += 1;
                        offset     -= sample_num;
                        i          -= 1;
                        break;
                    }
                }

                // Recieve packet (socket error checking done by function)
                rcvd_size = receive_socket( index, rcvd_max_size, (char *) rcvd_buffer );

                if ( rcvd_size > 0 ) {
                    command_args   = (uint32 *) ( rcvd_buffer + cmd_hdr_size );    
                    node_checksum  = endian_swap_32( command_args[0] );

                    // Compare the checksum values
                    if ( node_checksum != checksum ) {
                    
                        // If we have a checksum error in fast write mode, then switch to slow write and start over
                        if ( slow_write == 0 ) {
                            printf("WARNING:  Checksums do not match on pkt %d.  Expected = %x  Received = %x \n", i, checksum, node_checksum);
                            printf("          Starting over with slow write.  If this message occurs frequently, please \n");
                            printf("          adjust the wait_time in wn_write_baseband_buffer().  The node might not \n");
                            printf("          be able to keep up with the current rate of packets. \n");
                            
                            slow_write = 1;
                            offset     = start_sample;
                            i          = -1;
                            break;
                        } else {
                            die_with_error("Error:  Checksums do not match when in slow write... aborting.");
                        }
                    }
                    
                    timeout = 0;
                    done    = 1;
                } else {
                    // If we do not have a packet, increment the timeout counter
                    timeout++;
                }
            }  // END while( !done )
        }  // END if need_resp
        
        // This function can saturate the ethernet wire.  However, for small packets the 
        // node cannot keep up and therefore we need to delay the next transmission based on:
        // 1) packet size and 2) number of buffers being written
        
        // NOTE:  This is a simplified implementation based on experimental data.  It is by no means optimized for 
        //     all cases.  Since WARP v2 and WARP v3 hardware have drastically different internal architectures,
        //     we first have to understand which HW the Write IQ is being performed and scale the wait_times
        //     accordingly.  Also, since we can have one transmission of IQ data be distributed to multiple buffers,
        //     we need to increase the timeout if we are transmitting to more buffers on the node in order to 
        //     compensate for the extended processing time.  One other thing to note is that if you view the traffic that
        //     this generates on an oscilloscope (at least on Window 7 Professional 64-bit), it is not necessarily regular 
        //     but will burst 2 or 3 packets followed by an extended break.  Fortunately there is enough buffering in 
        //     the data flow that this does not cause a problem, but it makes it more difficult to optimize the data flow 
        //     since we cannot guarentee the timing of the packets at the node.  
        //
        //     If you start receiving checksum failures and need to adjust timing, please do so in the code below.
        //
        switch ( hw_ver ) {
            case TRANSPORT_WARP_HW_v2:
                // WARP v2 Hardware only supports small ethernet packets
 
                buffer_count = 0;

                // Count the number of buffers in the buffer_id
                for( j = 0; j < TRANSPORT_WARP_RF_BUFFER_MAX; j++ ) {
                    if ( ( ( buffer_id >> j ) & 0x1 ) == 1 ) {
                        buffer_count++;
                    }
                }
            
                // Note:  Performance drops dramatically if this number is smaller than the processing time on
                //     the node.  This is due to the fact that you perform a slow write when the checksum fails.  
                //     For example, if you change this from 160 to 140, the Avg Write IQ per second goes from
                //     ~130 to ~30.  If this number gets too large, then you will also degrade performance
                //     given you are waiting longer than necessary.
                //
                // Currently, wait times are set at:
                //     1 buffer  = 160 us
                //     2 buffers = 240 us
                //     3 buffers = 320 us
                //     4 buffers = 400 us
                // 
                // This is due to the fact that processing on the node is done thru a memcpy and takes 
                // a fixed amount of time longer for each additonal buffer that is transferred.
                //                
                wait_time = 80 + ( buffer_count * 80 );
                
                wn_usleep( wait_time );   // Takes in a wait time in micro-seconds
            break;
            
            case TRANSPORT_WARP_HW_v3:
                // In WARP v3 hardware, we need to account for both small packets as well as jumbo frames.  Also,
                // since the WARP v3 hardware uses a DMA to transfer packet data, the processing overhead is much
                // less than on v2 (hence the smaller wait times).  Through experimental testing, we found that 
                // for jumbo frames, the processing overhead was smaller than the length of the ethernet transfer
                // and therefore we do not need to wait at all.  We have not done exhaustive testing on ethernet 
                // packet size vs wait time.  So in this simplified implementation, if your Ethernet MTU size is 
                // less than 9000 bytes (ie approximately 0x8B8 samples) then we will insert a 40 us, or 50 us, 
                // delay between transmissions to give the board time to keep up with the flow of packets.
                // 
                if ( max_samples < 0x800 ) {
                    if ( buffer_id == 0xF ) {
                        wait_time = 50;
                    } else {
                        wait_time = 40;
                    }

                    wn_usleep( wait_time );   // Takes in a wait time in micro-seconds
                }
             break;
             
             default:
                printf("WARNING:  HW version of node (%d) is not recognized.  Please check your setup.\n", hw_ver);
             break;
        }
        
    }  // END for num_pkts


    if ( offset != num_samples ) {
        printf("WARNING:  Issue with calling function.  \n");
        printf("    Requested %d samples, sent %d sample based on other packet information: \n", num_samples, offset);
        printf("    Number of packets to send %d, Max samples per packet %d \n", num_pkts, max_samples);
    }
    
    // Free locally allocated memory    
    free( send_buffer );
    free( rcvd_buffer );

    // Finalize outputs
    if ( seq_num > seq_start_num ) {
        *num_cmds += seq_num - seq_start_num;
    } else {
        *num_cmds += (0xFFFF - seq_start_num) + seq_num;
    }
    
    return offset;
}



/*****************************************************************************/
/**
*  Function:  wn_update_checksum
*
*  Function to calculate a Fletcher-32 checksum to detect packet loss
*
******************************************************************************/
unsigned int wn_update_checksum(unsigned short int newdata, unsigned char reset ){
	// Fletcher-32 Checksum
	static unsigned int sum1 = 0;
	static unsigned int sum2 = 0;

	if( reset ){ sum1 = 0; sum2 = 0; }

	sum1 = (sum1 + newdata) % 0xFFFF;
	sum2 = (sum2 + sum1   ) % 0xFFFF;

	return ( ( sum2 << 16 ) + sum1);
}



#ifdef WIN32

/*****************************************************************************/
/**
*  Function:  uSleep
*
*  Since the windows Sleep() function only has a resolution of 1 ms, we need
*  to implement a usleep() function that will allow for finer timing granularity
*
******************************************************************************/
void wn_mex_udp_transport_usleep( int wait_time ) {
    static bool     init = false;
    static LONGLONG ticks_per_usecond;

    LARGE_INTEGER   ticks_per_second;    
    LONGLONG        wait_ticks;
    LARGE_INTEGER   start_time;
    LONGLONG        stop_time;
    LARGE_INTEGER   counter_val;

    // Initialize the function
    if ( !init ) {
    
        if ( QueryPerformanceFrequency( &ticks_per_second ) ) {
            ticks_per_usecond = ticks_per_second.QuadPart / 1000000;
            init = true;
        } else {
            printf("QPF() failed with error %d\n", GetLastError());
        }
    }

    if ( ticks_per_usecond ) {
    
        // Calculate how many ticks we have to wait
        wait_ticks = wait_time * ticks_per_usecond;
    
        // Save the performance counter value
        if ( !QueryPerformanceCounter( &start_time ) )
            printf("QPC() failed with error %d\n", GetLastError());

        // Calculate the stop time
        stop_time = start_time.QuadPart + wait_ticks;

        // Wait until the time has expired
        while (1) {        

            if ( !QueryPerformanceCounter( &counter_val ) ) {
                printf("QPC() failed with error %d\n", GetLastError());
                break;
            }
            
            if ( counter_val.QuadPart >= stop_time ) {
                break;
            }
        }
    }
}


#endif

