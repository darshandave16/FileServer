#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <netinet/in.h>

#define PORT 8080
#define BUFFER_SIZE 1024

int parse_and_validate_command(char *command, int *uUnzip, int *uRecvFile) {
    char *cmd = strtok(command, " \n");
    if (!cmd) {
        printf("Invalid command\n");
        return -1;
    }
    if (strcmp(cmd, "findfile") == 0) {
        char *filename = strtok(NULL, " \n");
        if (!filename) {
            printf("Usage: findfile filename\n");
            return -1;
        }
        *uRecvFile = 1;
        return 0;

    } else if (strcmp(cmd, "sgetfiles") == 0) {
        char *size1_str = strtok(NULL, " \n");
        char *size2_str = strtok(NULL, " \n");
        if (!size1_str || !size2_str) {
            printf("Usage: sgetfiles size1 size2 [-u]\n");
            return -1;
        }
        int size1 = atoi(size1_str);
        int size2 = atoi(size2_str);
        if (size1 <= 0 || size2 <= 0 || size1 > size2) {
            printf("Invalid size range\n");
            return -1;
        }
        // TODO: handle sgetfiles command
        char *unzip = strtok(NULL, " \n");
        if (unzip && strcmp(unzip, "-u") == 0) {
            // TODO: handle -u flag
            *uUnzip = 0;
        }
        *uRecvFile = 0;
        return 0;

    } else if (strcmp(cmd, "dgetfiles") == 0) {
        char *date1_str = strtok(NULL, " \n");
        char *date2_str = strtok(NULL, " \n");
        if (!date1_str || !date2_str) {
            printf("Usage: dgetfiles date1 date2 [-u]\n");
            return -1;
        }
        // TODO: validate date1 and date2 format
        // TODO: handle dgetfiles command
        char *unzip = strtok(NULL, " \n");
        if (unzip && strcmp(unzip, "-u") == 0) {
            // TODO: handle -u flag
            *uUnzip = 0;
        }
        *uRecvFile = 0;
        return 0;
    } else if (strcmp(cmd, "getfiles") == 0) {
        char *filename = strtok(NULL, " \n");
        int count = 0;
        while (filename && count < 6) {
            if(strcmp(filename, "-u") == 0)
            {
                *uUnzip = 0;
                count++;
                continue;
            }
            count++;

            filename = strtok(NULL, " \n");
            
        }
        if (count == 0) {
            printf("Usage: getfiles file1 [file2 ... file6] [-u]\n");
            return -1;
        }
        // TODO: handle getfiles command
        char *unzip = strtok(NULL, " \n");
        if (unzip && strcmp(unzip, "-u") == 0) {
            // TODO: handle -u flag
            *uUnzip = 0;
        }
        *uRecvFile = 0;
        return 0;
    } else if (strcmp(cmd, "gettargz") == 0) {
        char *ext = strtok(NULL, " \n");
        int count = 0;
        while (ext && count < 6) {
            if(strcmp(ext, "-u") == 0)
            {
                *uUnzip = 0;
                count++;
                continue;
            }
            count++;
            ext = strtok(NULL, " \n");
        }
        if (count == 0) {
            printf("Usage: gettargz extension1 [extension2 ... extension6] [-u]\n");
            return -1;
        }
        // TODO: handle gettargz command
        char *unzip = strtok(NULL, " \n");
        if (unzip && strcmp(unzip, "-u") == 0) {
            // TODO: handle -u flag
         *uUnzip = 0;
        }
        *uRecvFile = 0;
        return 0;
    } else if (strcmp(cmd, "quit") == 0) {
        return -2;
    }
    printf("Invalid Command\n");
    return -1;
}

int main(int argc, char const *argv[])
{
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char buffer1[1024] = {0};
    char tempbuf[1024] = {0};
    char filename[1024] = {0};

   
   
    // Creating socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }
    
    memset(&serv_addr, '0', sizeof(serv_addr));
   
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
       
    // Converting IPv4 and IPv6 addresses from text to binary form
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0)
    {
        perror("invalid address/ address not supported");
        exit(EXIT_FAILURE);
    }
    // Connecting to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }
    // Reading welcome message from server
    valread = read(sock, buffer, 1024);
    printf("%s\n", buffer);
    
    // Sending messages to server
    while(1)
    {
        memset(buffer, 0, sizeof(buffer));
        memset(tempbuf, 0, sizeof(buffer));
        printf("\nEnter Command: ");
        fgets(buffer, 1024, stdin);
        int uUnzip = 1;
        printf("client : %s",buffer);
        strcpy(tempbuf, buffer);

        int uRecvFile = 1;
        int status = parse_and_validate_command(tempbuf, &uUnzip, &uRecvFile);

        if(status == 0)
        {
            send(sock, buffer, strlen(buffer), 0);
        }
        else if (status == -2 )
        {
            send(sock, buffer, strlen(buffer), 0);
            printf("Disconnected from the server\n");
            break;
        }else
        {
            continue;
        }

        //after sending command
        memset(buffer, 0, sizeof(buffer));
        memset(tempbuf, 0, sizeof(tempbuf));
        memset(filename, 0, sizeof(filename));
        if(uRecvFile == 0)
        {

            int bytes_received = 0;
            // Open file for writing
            FILE *fp;
                // receive file name and size from server
             recv(sock, buffer, BUFFER_SIZE, 0);
            char *file_name = strtok(buffer, " ");
            int file_size = atoi(strtok(NULL, " "));
            printf("\nreceiving file %s of size %d",file_name,  file_size);
            // open file for writing
            fp = fopen(file_name, "wb");
            if (fp == NULL) {
                perror("file open failed");
                exit(EXIT_FAILURE);
            }

            int count = 0;
            // receive file data from server
            while (bytes_received < file_size) {
                int bytes_read = recv(sock, buffer, BUFFER_SIZE, 0);
                fwrite(buffer, sizeof(char), bytes_read, fp);
                bytes_received += bytes_read;
                printf("\n i = %d bytes received = %d total size = %d",count,bytes_received,file_size);
            }

        // Close file
        fclose(fp);
        printf("File received and saved temp.tar.gz \n");

        if(uUnzip == 0)
        {
            system("tar -xvf temp.tar.gz");
            printf("temp.tar.gz is unziped\n");
            uUnzip = 1;
        }

        // Clear buffer and filename for next file
        memset(buffer, 0, BUFFER_SIZE);
        memset(filename, 0, BUFFER_SIZE);
        }else
        {
            memset(buffer, 0, sizeof(buffer));
            valread = read(sock, buffer, 1024);
            printf("Server: %s", buffer);
        }
    }
    close(sock);
    return 0;
}
