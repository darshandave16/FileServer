#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ftw.h>
#include <fcntl.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define MAX_PATH 1024


const char *search_file_name;
off_t search_file_size;
time_t search_file_creation_time;
// Define the callback function for nftw
int search_callback(const char *path, const struct stat *sb, int typeflag, struct FTW *ftwbuf) {
    // Check if this is the file we're looking for
    if (typeflag == FTW_F && strcmp(path + ftwbuf->base, search_file_name) == 0) {
        search_file_size = sb->st_size;
        search_file_creation_time = sb->st_ctime;
        return 1;  // Stop searching
    } else {
        return 0;  // Continue searching
    }
}
int search_file(const char *root_dir, const char *file_name, off_t *file_size, time_t *creation_time) {
    // Define a struct to hold the search results
   search_file_name = file_name; search_file_size = -1; search_file_creation_time = -1;

    // Call nftw to perform the search
    if (nftw(root_dir, search_callback, 10, FTW_PHYS) == -1) {
        return -1;  // Error occurred
    }

    // Check if the file was found
    if (search_file_size == -1) {
        return 0;  // File not found
    } else {
        *file_size = search_file_size;
        *creation_time = search_file_creation_time;
        return 1;  // Success
    }
}

void handleFileTar(char* findCommand)
{
     // Create temporary file to store file paths
    FILE *fp = fopen("temp.txt", "w");
    if (fp == NULL) {
        perror("Failed to create temporary file");
        exit(1);
    }
    int a;
    a = dup(STDOUT_FILENO);

    if (dup2(fileno(fp), STDOUT_FILENO) == -1) {
        perror("Failed to redirect output");
        exit(1);
    }
    FILE *fp1 = popen(findCommand, "r");
    if (fp1 == NULL) {
        perror("Failed to execute command");
        // return 1;
        return;
    }
    char buffer[4096];
    while (fgets(buffer, sizeof(buffer), fp1) != NULL) {
        // Do something with each file path, e.g. print it
        printf("%s", buffer);
    }
    pclose(fp1);
    fclose(fp);
    dup2(a, STDOUT_FILENO);

    // Create archive of files in temporary file
    int pid = fork();
    if (pid == -1) {
        perror("Failed to fork");
        exit(1);
    } else if (pid == 0) {
        // Child process
        execlp("tar", "tar", "-czf", "temp.tar.gz", "-T", "temp.txt", NULL);
        perror("Failed to execute tar");
        exit(1);
    } else {
        // Parent process
        int status;
        waitpid(pid, &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "tar completed \n");
            // printf("created tar ");
            //popen("rm temp.txt", "r");
            // exit(1);
        }
        // printf("created tar ");
        // popen("rm temp.txt", "r");
    }
}

int parse_and_Execute_command(char *command, int new_socket) {
    char *cmd = strtok(command, " \n");
    if (!cmd) {
        printf("Invalid command\n");
        return -2;
    }
    if (strcmp(cmd, "findfile") == 0) {
        char *filename = strtok(NULL, " \n");
        if (!filename) {
            printf("Usage: findfile filename\n");
            return -2;
        }
        char res[1024];
        off_t file_size = -1;
        time_t creation_time = -1;
        search_file(getenv("HOME"), filename, &file_size, &creation_time);
        if(file_size == -1 || creation_time == -1)
        {
            snprintf(res, sizeof(res), "File not found");
        }else
        {
            snprintf(res, sizeof(res), "%s, %lld Bytes, %s",filename, file_size, ctime(&creation_time));
        }
        // Sending response back to client
        // char buffer[1024] = {0};
        // printf("Server: ");
        // fgets(buffer, 1024, stdin);
        send(new_socket, res, strlen(res), 0);
        return 1;
    } else if (strcmp(cmd, "sgetfiles") == 0) {
        char *size1_str = strtok(NULL, " \n");
        char *size2_str = strtok(NULL, " \n");
        if (!size1_str || !size2_str) {
            printf("Usage: sgetfiles size1 size2 [-u]\n");
            return -2;
        }
        long size1 = atol(size1_str);
        long size2 = atol(size2_str);
        if (size1 <= 0 || size2 <= 0 || size1 > size2) {
            printf("Invalid size range\n");
            return -2;
        }
        char commandBuffer[1024];
       // char *commandBuffer = "find . -type f -size +999 -size -10001";
        snprintf(commandBuffer, sizeof(commandBuffer), "find %s -type f -size +%ld -size -%ld",getenv("HOME"), size1, size2);
        
        handleFileTar(commandBuffer);
        return 0;

    } else if (strcmp(cmd, "dgetfiles") == 0) {
        char *date1_str = strtok(NULL, " \n");
        char *date2_str = strtok(NULL, " \n");
        if (!date1_str || !date2_str) {
            printf("Usage: dgetfiles date1 date2 [-u]\n");
            return -2;
        }
        char commandBuffer[1024];
        // char *commandBuffer = "find ~ -type f -newermt date1 ! -newermt date2";
        snprintf(commandBuffer, sizeof(commandBuffer), "find %s -type f -newermt %s ! -newermt %s",getenv("HOME"), date1_str, date2_str);

        handleFileTar(commandBuffer);
        return 0;
    } else if (strcmp(cmd, "getfiles") == 0) {
        char *filename = strtok(NULL, " \n");
        int count = 0;
        char args[1024];
        memset(args, 0, sizeof(args));
        while (filename && count < 6) {
            if(strcmp(filename, "-u") == 0)
            {
                count++;
                continue;
            }
            if(count != 0)
                strcat(args, " -o ");
            strcat(args, " -name '");
            strcat(args, filename);
            strcat(args, "'");
            count++;
            filename = strtok(NULL, " \n");

        }

        if (count == 0) {
            printf("Usage: getfiles file1 [file2 ... file6] [-u]\n");
            return -2;
        }
        char commandBuffer[1024];
        // char *commandBuffer = "find ~ -type f -newermt date1 ! -newermt date2";
        snprintf(commandBuffer, sizeof(commandBuffer), "find %s %s",getenv("HOME"), args);

        handleFileTar(commandBuffer);
        return 0;
    } else if (strcmp(cmd, "gettargz") == 0) {
        char *ext = strtok(NULL, " \n");
        int count = 0;
        char args[1024];
        memset(args, 0, sizeof(args));
        while (ext && count < 6) {
            if(strcmp(ext, "-u") == 0)
            {
                count++;
                continue;
            }
            if(count != 0)
                strcat(args, " -o ");
            strcat(args, " -name '*.");
            strcat(args, ext);
            strcat(args, "'");
            count++;
            ext = strtok(NULL, " \n");
        }
        if (count == 0) {
            printf("Usage: gettargz extension1 [extension2 ... extension6] [-u]\n");
            return -2;
        }
        char commandBuffer[1024];
        // char *commandBuffer = "find ~ -type f -newermt date1 ! -newermt date2";
        snprintf(commandBuffer, sizeof(commandBuffer), "find %s %s",getenv("HOME"), args);

        handleFileTar(commandBuffer);
        return 0;
    } else if (strcmp(cmd, "quit") == 0) {
        return -2;
    }
    printf("Invalid Command\n");
    return -2;
}


void process_client (int sockfd)
{
    char buffer[1024] = {0};
    char tempBuff[1024] = {0};
    int valread;
    // Reading messages from client
    while(1)
    {
        FILE *fp;
        int file_size = 0;
        int bytes_sent = 0;
        memset(buffer, 0, sizeof(buffer));
        memset(tempBuff, 0, sizeof(tempBuff));

        valread = read(sockfd, buffer, 1024);
        printf("command received: %s", buffer);
        strcpy(tempBuff, buffer);
        char* filename = "temp.tar.gz";

         int status = parse_and_Execute_command(tempBuff, sockfd);
        if((status == -2) || (strcmp(buffer, "exit\n") == 0))
        {
            printf("Client disconnected\n");
            break;
        }
        if(status == 0)
        {        
            // open file for reading
            int file_fd, file_size;

            if ((file_fd = open(filename, O_RDONLY)) < 0) {
                perror("file open failed");
                exit(EXIT_FAILURE);
            }

            // get file size
            if ((file_size = lseek(file_fd, 0, SEEK_END)) < 0) {
                perror("file size failed");
                exit(EXIT_FAILURE);
            }
            lseek(file_fd, 0, SEEK_SET);

            // send file size to client
            sprintf(buffer, "%s %d", filename, file_size);
            send(sockfd, buffer, strlen(buffer), 0);
            // printf("sending file: %s of size : %d", filename, file_size);
            // send file data to client
            int bytes_sent = 0, bytes_read;
            while (bytes_sent < file_size) {
                // printf("sending chunk...");
                bytes_read = read(file_fd, buffer, BUFFER_SIZE);
                if (bytes_read < 0) {
                    perror("read failed");
                    exit(EXIT_FAILURE);
                }
                // printf("%s",buffer);
                send(sockfd, buffer, bytes_read, 0);
                // printf("send in bytes : %d of size : %d", bytes_sent, file_size);
                bytes_sent += bytes_read;
            }

            // close file
            close(file_fd);
        }
        memset(buffer, 0, sizeof(buffer));
        memset(tempBuff, 0, sizeof(tempBuff));
    }
}
int main(int argc, char const *argv[])
{
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char *welcome_message = "Welcome to the File Server\n";
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket creation error");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
       
    // Binding socket to the specified port
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address))<0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    // Listening for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    
    while(1)
    {
        // Accepting incoming connections
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0)
        {
            perror("accept failed");
            exit(EXIT_FAILURE);
        }
        
        int pid = fork();
        if (pid < 0) {
            perror("fork failed");
            close(new_socket);
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Child process handles client requests
            printf("New client connected\n");
            // Sending welcome message to client
            send(new_socket, welcome_message, strlen(welcome_message), 0);
            close(server_fd);
            process_client(new_socket);
            close(new_socket);
            exit(0);
        } 
        close(new_socket);
    }
    return 0;
}
