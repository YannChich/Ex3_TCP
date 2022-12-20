// Sender
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include <time.h>

#define SERVER_PORT 5655
#define SERVER_IP_ADDRESS "127.0.0.1"
#define BUFFER 5000


long sizefile(FILE* p_file){
    fseek(p_file, 0, SEEK_END);
    long size = ftell(p_file);
    fseek(p_file, 0, SEEK_SET);
    if(size%2==0){
        return size;
    }
    return size+1;
}

int main()
{
    int sock = 0, n = 0;
    char sendBuff[BUFFER];
    struct sockaddr_in serv_addr;

    // Authenticaton with XOR.
    int First_ID = 4616;
    int Second_ID = 7501;
    int Auth = First_ID ^ Second_ID;

    // Open the file
    FILE* file = fopen("file.txt","r");

    //Socket decleration.
    memset(sendBuff, '0', sizeof(sendBuff));
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return 1;
    }
    //Socket init.
    memset(&serv_addr, '0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if(inet_pton(AF_INET,(const char*)SERVER_IP_ADDRESS, &serv_addr.sin_addr)<=0)
    {
        printf("\n inet_pton error occured\n");
        return 1;
    }

    //File partitioning -> Arrays.
    long size_part = sizefile(file)/2;
    char *first_part = malloc(size_part);
    if(first_part == NULL){
        printf("Cannot find memory\n");
    }
    fread(first_part,1,size_part,file);
    char *second_part = malloc(size_part);
    if(second_part == NULL){
        printf("Cannot find memory\n");
    }
    fseek(file, size_part, SEEK_SET);
    fread(second_part,1,size_part,file);
    fseek(file, 0, SEEK_SET);

    //Establishing a connection with server.
    if( connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
       printf("\n Error : Connect Failed \n");
       return 1;
    }
    printf("connected to server\n");

    //send the file size
    int send_file = send(sock,&size_part,sizeof(long),0);

    int Want_Exit =1;
    //Actual file sending loop.
    while(1){
        if (setsockopt(sock, IPPROTO_TCP , TCP_CONGESTION , "cubic", 5) == -1)
	        {
	            printf("setsockopt() failed with error code : %d\n", errno);
	            return 1;
	        }
        int offset = 0;
        printf("sender : sending the first part of the file\n");
        while (offset < size_part) {
            int n = send(sock, first_part + offset, BUFFER, 0);
            if (n < 0) {
                printf("\nError: send failed\n");
                return 1;
            }
            offset += n;
        }
        // Rcv the authentication
        printf("Waiting for the authentication of the server\n");
        int password_server = 0;
        int rcv_server = recv(sock,&password_server,sizeof(password_server),0);
        if(rcv_server == -1){
            printf("Error in receipt of the authentication , the socket will close");
            close(sock);
            exit(1);

        } 
        if(Auth != password_server){
            printf("Wrong password , sorry the socket will close\n");
            close(sock);
            exit(1);
        }
        else{
            printf("Password : Valid !\n");
        }

        //Changing CC Algo to "reno"
        if (setsockopt(sock, IPPROTO_TCP, TCP_CONGESTION, "reno", 4) == -1)
	        {
	            printf("setsockopt() failed with error code : %d\n", errno);
	            return 1;
	        }
		offset = 0;
        printf("sender : sending the second part of the file\n");
        while (offset < size_part) {
            int n2 = send(sock, second_part + offset, BUFFER, 0);
            if (n2 < 0) {
                printf("\nError: send failed\n");
                return 1;
            }
            offset += n2;
        }
        //Scanning for continuous instructions.
        printf("To continue submit [1] else [0]: ");
        scanf("%d", &Want_Exit);
        if(Want_Exit==0){
            send(sock,&Want_Exit,sizeof(int),0);
            free(first_part);
            free(second_part);
            printf("End of TCP connection with the server\n");
            close(sock);
            exit(1);
        
        }
        else{
            send(sock,&Want_Exit,sizeof(int),0);
        }
    }
    sleep(1);
    close(sock);
    return 0;
}