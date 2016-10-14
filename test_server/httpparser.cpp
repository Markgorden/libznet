#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

#define BUFFER_SIZE 4096
enum CHECK_STATE { CHECK_STATE_REQUESTLINE = 0, CHECK_STATE_HEADER, CHECK_STATE_CONTENT };
enum LINE_STATUS { LINE_OK = 0, LINE_BAD, LINE_OPEN };
enum HTTP_CODE { NO_REQUEST, GET_REQUEST, BAD_REQUEST, FORBIDDEN_REQUEST, INTERNAL_ERROR, CLOSED_CONNECTION };
static const char* szret[] = { "I get a correct result\n", "Something wrong\n" };

#define MAGIC_LEN           4
#define COMMAD_LEN          2
#define SUCCESS_LOGIN_LEN   1
#define CONTENT_LEN         4
//#define char login_cmd_magic[4] = {0x11,0xff,0x33,0xff};
//#define char login_cmd_command

LINE_STATUS parse_line( char* buffer, int& checked_index, int& read_index )
{
    char temp;
    for ( ; checked_index < read_index; ++checked_index )
    {
        temp = buffer[ checked_index ];
        if ( temp == '\r' )
        {
            if ( ( checked_index + 1 ) == read_index )
            {
                return LINE_OPEN;
            }
            else if ( buffer[ checked_index + 1 ] == '\n' )
            {
                buffer[ checked_index++ ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if( temp == '\n' )
        {
            if( ( checked_index > 1 ) &&  buffer[ checked_index - 1 ] == '\r' )
            {
                buffer[ checked_index-1 ] = '\0';
                buffer[ checked_index++ ] = '\0';
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HTTP_CODE parse_requestline( char* szTemp, CHECK_STATE& checkstate )
{
    char* szURL = strpbrk( szTemp, " \t" );
    if ( ! szURL )
    {
        return BAD_REQUEST;
    }
    *szURL++ = '\0';

    char* szMethod = szTemp;
    if ( strcasecmp( szMethod, "GET" ) == 0 )
    {
        printf( "The request method is GET\n" );
    }
    else
    {
        return BAD_REQUEST;
    }

    szURL += strspn( szURL, " \t" );
    char* szVersion = strpbrk( szURL, " \t" );
    if ( ! szVersion )
    {
        return BAD_REQUEST;
    }
    *szVersion++ = '\0';
    szVersion += strspn( szVersion, " \t" );
    if ( strcasecmp( szVersion, "HTTP/1.1" ) != 0 )
    {
        return BAD_REQUEST;
    }

    if ( strncasecmp( szURL, "http://", 7 ) == 0 )
    {
        szURL += 7;
        szURL = strchr( szURL, '/' );
    }

    if ( ! szURL || szURL[ 0 ] != '/' )
    {
        return BAD_REQUEST;
    }

    //URLDecode( szURL );
    printf( "The request URL is: %s\n", szURL );
    checkstate = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

HTTP_CODE parse_headers( char* szTemp )
{
    if ( szTemp[ 0 ] == '\0' )
    {
        return GET_REQUEST;
    }
    else if ( strncasecmp( szTemp, "Host:", 5 ) == 0 )
    {
        szTemp += 5;
        szTemp += strspn( szTemp, " \t" );
        printf( "the request host is: %s\n", szTemp );
    }
    else
    {
        printf( "I can not handle this header\n" );
    }

    return NO_REQUEST;
}

HTTP_CODE parse_content( char* buffer, int& checked_index, CHECK_STATE& checkstate, int& read_index, int& start_line )
{
    LINE_STATUS linestatus = LINE_OK;
    HTTP_CODE retcode = NO_REQUEST;
    while( ( linestatus = parse_line( buffer, checked_index, read_index ) ) == LINE_OK )
    {
        char* szTemp = buffer + start_line;
        start_line = checked_index;
        switch ( checkstate )
        {
            case CHECK_STATE_REQUESTLINE:
            {
                retcode = parse_requestline( szTemp, checkstate );
                if ( retcode == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                break;
            }
            case CHECK_STATE_HEADER:
            {
                retcode = parse_headers( szTemp );
                if ( retcode == BAD_REQUEST )
                {
                    return BAD_REQUEST;
                }
                else if ( retcode == GET_REQUEST )
                {
                    return GET_REQUEST;
                }
                break;
            }
            default:
            {
                return INTERNAL_ERROR;
            }
        }
    }
    if( linestatus == LINE_OPEN )
    {
        return NO_REQUEST;
    }
    else
    {
        return BAD_REQUEST;
    }
}

void print_buf(char *buf, int len)
{
	//return;
	int idx = 0;
	//print_info("buf len:%d\n", len);
	printf("buf len:%d\n", len);
	for (idx = 0; idx < len; idx++)
	{
		if (idx % 16 == 0)
		{
			//print_info("\n");
			printf("\n");
		}
		//print_info("0x%02X ", (unsigned char)buf[idx]);
		printf("0x%02X ", (unsigned char)buf[idx]);
  
        
	}
	//print_info("\n");
	printf("\n");
}

int main( int argc, char* argv[] )
{
    if( argc <= 2 )
    {
        printf( "usage: %s ip_address port_number\n", basename( argv[0] ) );
        return 1;
    }
    const char* ip = argv[1];
    int port = atoi( argv[2] );
    
    struct sockaddr_in address;
    bzero( &address, sizeof( address ) );
    address.sin_family = AF_INET;
    inet_pton( AF_INET, ip, &address.sin_addr );
    address.sin_port = htons( port );
    
    int listenfd = socket( PF_INET, SOCK_STREAM, 0 );
    assert( listenfd >= 0 );
    
    int ret = bind( listenfd, ( struct sockaddr* )&address, sizeof( address ) );
    assert( ret != -1 );
    
    ret = listen( listenfd, 5 );
    assert( ret != -1 );
    
    struct sockaddr_in client_address;
    socklen_t client_addrlength = sizeof( client_address );
    printf("waiting connect ...\n");
    int fd;
    char *login_cmd;
    unsigned short command ;
    int content_len=0;
    
    while( 1 )
    {   
        fd = accept( listenfd, ( struct sockaddr* )&client_address, &client_addrlength );
        if(fd < 0)
        {
          printf("accept error !");
          continue;
        }
        printf("there is a connect:%s \r\n", inet_ntoa(client_address.sin_addr));
        
        
                            //
               
        
            //    if(send(fd, "Hello,you are connected0000!", 26,0) == -1)  
            //        printf("send error\n"); 
            //    else
             //       printf("send to client0000\n"); 
                
            char buffer[ BUFFER_SIZE ];
            memset( buffer, '\0', BUFFER_SIZE );
            int data_read = 0;
            int read_index = 0;
            int checked_index = 0;
            int start_line = 0;
            CHECK_STATE checkstate = CHECK_STATE_REQUESTLINE;
         
                data_read = recv( fd, buffer + read_index, BUFFER_SIZE - read_index, 0 );
                if ( data_read == -1 )
                {
                    printf( "reading failed\n" );
                    break;
                }
                else if ( data_read == 0 )
                {
                    //printf( "remote client has closed the connection\n" );
                    //break;
                }
            
                if (data_read > 0) 
                { 
                    printf("recv data from client len =%d\n",data_read);
                    print_buf(buffer, data_read);
                }
                
                
                char content_buf[]="10.36.65.179:8082,10.36.65.179:8083";
                content_len = strlen(content_buf)+2;
                int cmd_len = MAGIC_LEN+COMMAD_LEN+CONTENT_LEN+content_len;
                login_cmd  =(char *)malloc(cmd_len);
                
                command = 1;
                //magic
                login_cmd[0]=0xff;
                login_cmd[1]=0x33;
                login_cmd[2]=0xff;
                login_cmd[3]=0x11;
                
                //int magic = 0x11ff33ff;
                //memcpy(login_cmd+0,&magic,sizeof(unsigned int));
                char success_login = 0;
                int content_len_ip = content_len - 2;
                memcpy(login_cmd+4,&command,sizeof(unsigned short));                                                     //command
                memcpy(login_cmd+4 +sizeof(unsigned short),&content_len,sizeof(int));                                    //content_len + 2
                memcpy(login_cmd+4 +sizeof(unsigned short)+sizeof(int),&success_login,sizeof(char));                    //success_login_flage
                memcpy(login_cmd+4 +sizeof(unsigned short)+sizeof(int)+sizeof(char),&content_len_ip,sizeof(char));
                memcpy(login_cmd+4 +sizeof(unsigned short)+sizeof(int)+sizeof(char)+sizeof(char),content_buf,content_len-2);
                
                print_buf(login_cmd, cmd_len);

  
                    if(send(fd, login_cmd, cmd_len ,0) == -1)  
                    printf("send error\n"); 
                    else
                    printf("send to client\n"); 
                    
                
            
                free(login_cmd);
                //char * sendData = "data from server!!!!!\n";
                //int nAddrLen = sizeof(client_address);
               // sendto(fd, sendData, strlen(sendData), 0, (sockaddr *)&client_address, nAddrLen);    

            /*
            read_index += data_read;
            HTTP_CODE result = parse_content( buffer, checked_index, checkstate, read_index, start_line );
            if( result == NO_REQUEST )
            {
                continue;
            }
            else if( result == GET_REQUEST )
            {
                send( fd, szret[0], strlen( szret[0] ), 0 );
                break;
            }
            else
            {
                send( fd, szret[1], strlen( szret[1] ), 0 );
                break;
            }
            */
        
        
    }
    
    
    close( fd );
    close( listenfd );
    return 0;
}
