// Receiver
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>


#define SERVER_PORT 5655
#define SERVER_IP_ADDRESS "127.0.0.1"
#define BUFFER 5000

int main()
{   
    // Authenticaton with XOR.
    int First_ID = 4616;
    int Second_ID = 7501;
    int Auth = First_ID ^ Second_ID;
    int serverListener = 0, clientSocket = 0;
    struct sockaddr_in serv_addr;

    //Buffer Array size & splitting for two parts. 
    char recvBuff[BUFFER];

    //Socket decleration.
    serverListener = socket(AF_INET, SOCK_STREAM, 0);
    if(serverListener == -1)
    {
        printf("Could not create listening socket : %d",errno);
    }
    memset(&serv_addr, '0', sizeof(serv_addr));
    memset(recvBuff, '0', sizeof(recvBuff));

    //Socket init & binding.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (bind(serverListener, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        printf("Bind failed with error code : %d" ,	errno);
	    // TODO: close the socket
        return -1;
    }
    printf("Bind() success\n");
    listen(serverListener, 10);
    
    //Accept and incoming connection
    printf("Waiting for incoming TCP-connections...\n");
    clientSocket = accept(serverListener, (struct sockaddr*)NULL, NULL);
    printf("Connection accepted Client <--> Server\n");

    // Gonna accept the size file
    long part_size =0;
    int rcv_file = recv(clientSocket,&part_size,sizeof(long),0);
    sleep(2);
    printf("size part : %ld\n", part_size);

    char *FirstPart = malloc(part_size);
    char *SecondPart = malloc(part_size);
    //Variables for time measuring.
    int counter_sending_Cubic =0;
    int counter_sending_Reno =0;
    struct timeval start,end,start1,end1;
    double elapsedTimeCubic = 0;
    double avgElapsedTimeCubic=0;
    double elapsedTimeReno = 0;
    double avgElapsedTimeReno=0;

    //Actual file reciving loop.
    while(1)
    {
        //First part file sending.
        if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, "cubic", 5) == -1)
	        {
	            printf("setsockopt() failed with error code : %d\n", errno);
	            return 1;
	        }
        int offset = 0;
        gettimeofday(&start,NULL);
        while (offset < part_size) {
            int n = recv(clientSocket, recvBuff, BUFFER, 0);
            if (n < 0) {
                printf("\nError: recv failed\n");
                return 1;
            }
            memcpy(FirstPart + offset, recvBuff, n);
            offset += n;
        }
        gettimeofday(&end,NULL);
        elapsedTimeCubic = ((end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0);
        counter_sending_Cubic++;
        avgElapsedTimeCubic+=elapsedTimeCubic;
    	printf("Send the first part succesfuly with the CC : cubic and time : %f \n", elapsedTimeCubic);

        // Send the authentication to the Client
        printf("Sending the authentication to the client\n");
        int num_message = Auth;
	    int bytes_sent = send(clientSocket, &num_message, sizeof(num_message), 0);
	    if (bytes_sent == -1)
	        {
	            printf("Error in the sending of the authentication , the socker will close\n");
                close(serverListener);
	            close(clientSocket);
	            exit(1);
	        }

        //Second file part sending.
        if (setsockopt(clientSocket, SOL_SOCKET, SO_REUSEADDR, "reno", 4) == -1)
	        {
	            printf("setsockopt() failed with error code : %d\n", errno);
	            return 1;
	        }
        offset = 0;
        gettimeofday(&start1,NULL);
        while (offset < part_size) {
            int n2 = recv(clientSocket, recvBuff, BUFFER, 0);
            if (n2 < 0) {
                printf("\nError: recv failed\n");
                return 1;
            }
            memcpy(SecondPart + offset, recvBuff, n2);
            offset += n2;
        }
        gettimeofday(&end1,NULL);
        elapsedTimeReno = ((end1.tv_sec - start1.tv_sec) + (end1.tv_usec - start1.tv_usec) / 1000000.0);
        counter_sending_Reno++;
        avgElapsedTimeReno+=elapsedTimeReno;
    	printf("Send the Second part succesfuly with the CC : reno and time : %f \n\n", elapsedTimeReno);    
        int want_toExit =1;
        int receive2 = recv(clientSocket,&want_toExit,sizeof(int),0);
        if (receive2 == -1)
    	    {
    		    printf("recvfrom() [Exit or Continue] failed with error code  : %d", errno	);
    		    return -1;
    	    }
        //Recving continuous instructions.
        if(want_toExit == 0){
            printf("The avg time calc for the first part with CC 'cubic' : [ %f ]\n", (avgElapsedTimeCubic/counter_sending_Cubic));
            printf("The avg time calc for the second part with CC 'reno' : [ %f ]\n", (avgElapsedTimeReno/counter_sending_Reno));
            printf("End of the TCP connection with the client\n");
            free(FirstPart);
            free(SecondPart);
            sleep(1);
            close(clientSocket);
            close(serverListener);
            exit(1);
            
        }
    }
    close(serverListener);
    return 0;
}