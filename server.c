#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#include <zlib.h>
#include <limits.h>

/* *************************************Constants********************************/ 
#define MAX_TOKENS 8
#define CHUNK_SIZE 16384
#define MAX_CLIENTS 8
/* *******************************************************************************/ 

/* *******************Functions declarations for client processing ***************/ 
void findFile(char* file, int sd);
void sgetFiles(int size1, int size2, bool unzip, int sd);
void dgetFiles(char* date1, char* date2, bool unzip, int sd);
void getFiles(char* filenames[], int numFiles, bool unzip, int sd);
void getTargz(char* extensions[], int numFiles, bool unzip, int sd);
/* *******************************************************************************/ 

/* *******************Functions declarations for server processing ***************/ 
void getConnectedClient(int);
int processClient(char msg[], int sd);
bool getAck(int sd);
void sendAck(int sd);
/* *******************************************************************************/ 

/* *********************************Messages ************************************/ 
char exitMessage[] = "quit\n";
char welcomeMessage[] = " *************************Welcome to the Server******************************\n\n \
> ****************Enter quit to exit*********\n \\n";
char fileFoundMessage[] = "File Found!";
char fileNotFound[] = "File Not Found!";
char downloadingMessage[] = "Downloading Files";
char errorMessage[] = "Try Again : Command is not valid!";
/* *******************************************************************************/ 

/* ************************************Flages************************************/ 
char getAction[] = "-1";
char unzipAction[] = "-3";
char fileFoundAction[] = "@#$%";
char fileNotFoundAction[] = "-2";
char ack[] = "-ack";
/* *******************************************************************************/ 


// Server root directory
char rootDir[] = ".";

/*
   funtion to send requested files to client from server and bases on argumnet whether to unzip or not
*/
void send_file(int sd, bool unzip) {

    //zip file name
    char file[] = "temp.tar.gz";

    // creating dir name
    char dirName[255];
    strcpy(dirName, rootDir);
    strcat(dirName, "/");
    
    // opening dir
    DIR* dp;
    dp = opendir(dirName);
    struct dirent* dirp;
    bool isFound = false;

    // read all dirrectories in opened dir
    while ((dirp = readdir(dp)) != NULL)
    {
        // find that file
        if (strcmp(dirp->d_name, file) == 0 && dirp->d_type != DT_DIR)
        {
            isFound = true;
            printf("> '%s' found\n", file);
            
            //  for client  donwloadFile() 
            write(sd, getAction, sizeof(getAction));
            printf("> downloading in client \n", file);
            getAck(sd);

            // sending file name
            write(sd, file, strlen(file));
            printf("> File name sent\n");
            getAck(sd);

            // open dir
            strcat(dirName, file);
            int fd = open(dirName, O_RDONLY, S_IRUSR | S_IWUSR);

            long fileSize = lseek(fd, 0L, SEEK_END);

            // send size of file
            write(sd, &fileSize, sizeof(long));
            printf("> fileSize sent\n");
            getAck(sd);

            // create buffer of fileSize
            char* buffer = malloc(fileSize);
            lseek(fd, 0L, SEEK_SET);
            read(fd, buffer, fileSize);
            write(sd, buffer, fileSize);
            getAck(sd);

            if (unzip == true) {
                write(sd, unzipAction, sizeof(unzipAction));
                printf("Unziping filedn");
                getAck(sd);
            }
            free(buffer);
            close(fd);
            printf("> '%s' sent\n", file);
        }
    }
    if (!isFound)
    {
        printf("> '%s' not found\n", file);
        write(sd, fileNotFound, sizeof(fileNotFound));
    }

    closedir(dp);
}

/* creating tar of requested files*/
void compress_files(char* file_list[], int num_files) {
    char command[1000] = "tar -czvf temp.tar.gz";
    for (int i = 0; i < num_files; i++) {
        strcat(command, " ");
        strcat(command, file_list[i]);
    }
    system(command);
}

/* getting connected client*/
void getConnectedClient(int sd) {
    char message[255];
    int n;
    
    write(sd, welcomeMessage, sizeof(welcomeMessage));
    
    while (1)
        // getting msg from client
        if (n = read(sd, message, 255)) {
            message[n] = '\0';
            printf("Client - %d: %s", getpid(), message);
            if (!strcasecmp(message, exitMessage))
            {
                printf("Client - %d disconnected!\n", getpid());
                close(sd);
                exit(0);
            }

            processClient(message, sd);
        }
}

/* function to process clinet */
int processClient(char msg[], int sd) {
    char* command, * fileName;

    // creating copy of msg
    char tempMessage[255];
    strcpy(tempMessage, msg);

    /* max arg in command*/
    char* tokens[MAX_TOKENS]; 
    int numTokens = 0;

    /* variable to check if unzip arg is there or not*/
    bool unzip = 0; 

    // Tokenize the command
    char* token = strtok(tempMessage, " \n");
    while (token != NULL && numTokens < MAX_TOKENS) {
        if (strcmp(token, "-u"
        ) == 0) {
            unzip = true;
            printf("unzip found\n");
        }
        else {
            tokens[numTokens++] = token;
        }
        token = strtok(NULL, " \n");
    }

    // calling the commands
    if (numTokens == 2 && strcmp(tokens[0], "findfile") == 0) {
        findFile(tokens[1], sd);
    }
    else if (numTokens <= 4 && strcmp(tokens[0], "sgetfiles") == 0) {
        int size1 = atoi(tokens[1]);
        int size2 = atoi(tokens[2]);
        sgetFiles(size1, size2, unzip, sd);
    }
    else if (numTokens <= 4 && strcmp(tokens[0], "dgetfiles") == 0) {
        char* date1 = tokens[1];
        char* date2 = tokens[2];
        dgetFiles(date1, date2, unzip, sd);
    }
    else if (strcmp(tokens[0], "getfiles") == 0) {
        char* files[] = { NULL };
        int numFiles = 0;
        int i;
        for (i = 1; i < numTokens && numFiles < 6; i++) {
            if (strcmp(tokens[i], "-u") == 0) {
                // The -u flag is not a file name, so skip it
                continue;
            }
            files[numFiles++] = tokens[i];
        }
        getFiles(files, numFiles, unzip, sd);
    }
    else if (numTokens >= 2 && numTokens <= 7 && strcmp(tokens[0], "gettargz") == 0) {
        char* extensions[] = { NULL }; // Array to store up to 6 extensions
        int numExtensions = 0;
        int i;
        for (i = 1; i < numTokens && numExtensions < 6; i++) {
            if (strcmp(tokens[i], "-u") == 0) {
                // The -u flag is not an extension, so skip it
                continue;
            }
            extensions[numExtensions++] = tokens[i];
        }
        getTargz(extensions, numExtensions, unzip, sd);
    }
    else {
        write(sd, errorMessage, strlen(errorMessage));
    }
}

/*function to find the requested file*/
void findFile(char* file, int sd) {

    char dirName[255];
    strcpy(dirName, rootDir);
    strcat(dirName, "/");
    DIR* dp;
    dp = opendir(dirName);

    if (dp == NULL) {
        printf("Invalid Directory\n");
        return;
    }

    struct dirent* dirp;
    struct stat filestat;
    bool isFound = false;

    // read all dirs
    while ((dirp = readdir(dp)) != NULL) {
        // find that file
        if (strcmp(dirp->d_name, ".") == 0 && strcmp(dirp->d_name, "..") == 0) {
            continue;
        }

        strcat(dirName, dirp->d_name);
        if (stat(dirName, &filestat) < 0) {
            printf("Error due to file status\n");
            return;
        }
        if (S_ISDIR(filestat.st_mode)) {
            // skip the directory
        }
        else if (strcmp(dirp->d_name, file) == 0) {
            char file_info[1000];
            sprintf(file_info, "Found file %s\n  Size: %ld bytes\n  Created: %s", file, filestat.st_size, ctime(&filestat.st_ctime));
            write(sd, file_info, strlen(file_info));
            isFound = true;
            break;
        }
        strcpy(dirName, rootDir);
        strcat(dirName, "/");
    }
    if (!isFound)
    {
        printf("> '%s' not found\n", file);
        write(sd, fileNotFound, sizeof(fileNotFound));
    }

    closedir(dp);
}

/*function to get files of requested size range */
void sgetFiles(int size1, int size2, bool unzip, int sd) {
    DIR* dir;
    struct dirent* ent;
    struct stat st;
    char path[1000];
    int file_count = 0;
    char* file_list[1000];

    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            if (lstat(ent->d_name, &st) == -1) {
                continue;
            }
            if (!S_ISDIR(st.st_mode)) {
              // Check file size and add to list if it's within the range
                int file_size = st.st_size;
                if (file_size >= size1 && file_size <= size2) {
                    file_list[file_count] = strdup(ent->d_name);
                    file_count++;
                }
            }
        }
        closedir(dir);
    }
    else {
   // Failed to open directory
        perror("");
        exit(EXIT_FAILURE);
    }
    if (file_count == 0) {
        write(sd, fileNotFound, sizeof(fileNotFound));
        printf("None of the files were found.\n");
    }
    else {
        char files_found[] = "The following files with the specified sizes were found:";
        write(sd, files_found, sizeof(files_found));
        printf("The following files were found:\n");
        printf("%d", file_count);
        for (int i = 0; i < file_count; i++) {
            printf("%s\n", file_list[i]);
            write(sd, file_list[i], sizeof(file_list[i]));
        }
    }

    if (file_count > 0) {
        compress_files(file_list, file_count);
        send_file(sd, unzip);
    }


}

/*function to get files of requested date range*/
void dgetFiles(char* date1, char* date2, bool unzip, int sd) {
    DIR* dir;
    struct dirent* ent;
    struct stat st;
    char path[1000];
    int file_count = 0;
    char* file_list[1000];

    // Parse date strings into time_t values
    struct tm date1_tm = { 0 };
    struct tm date2_tm = { 0 };
    strptime(date1, "%Y-%m-%d", &date1_tm);
    strptime(date2, "%Y-%m-%d", &date2_tm);
    time_t date1_t = mktime(&date1_tm);
    time_t date2_t = mktime(&date2_tm);

    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
                continue;
            }
            if (lstat(ent->d_name, &st) == -1) {
                continue;
            }
            if (!S_ISDIR(st.st_mode)) {
              // Check file modification time and add to list if it's within the range
                time_t file_time = st.st_mtime;
                if (file_time >= date1_t && file_time <= date2_t) {
                    file_list[file_count] = strdup(ent->d_name);
                    file_count++;
                }
            }
        }
        closedir(dir);
    }

    else {
   // Failed to open directory
        perror("");
        exit(EXIT_FAILURE);
    }
    if (file_count == 0) {
        write(sd, fileNotFound, sizeof(fileNotFound));
        printf("None of the files were found.\n");
    }
    else {
        char files_found[] = "The following files with the specified dates were found:";
        write(sd, files_found, sizeof(files_found));
        printf("The following files were found:\n");
        for (int i = 0; i < file_count; i++) {
            printf("%s\n", file_list[i]);
            write(sd, file_list[i], sizeof(file_list[i]));
        }
    }
    if (file_count > 0) {
        compress_files(file_list, file_count);
        send_file(sd, unzip);
    }
}

/*function to get files of requested file names*/
void getFiles(char* filenames[], int numFiles, bool unzip, int sd) {
    DIR* dir;
    struct dirent* ent;
    int file_count = 0;
    char* file_list[1000];

    // Check if each file exists in the current directory
    for (int i = 0; i < numFiles; i++) {

        if ((dir = opendir(".")) != NULL) {
            while ((ent = readdir(dir)) != NULL) {
                if (strcmp(ent->d_name, filenames[i]) == 0) {
                    file_list[file_count] = strdup(ent->d_name);
                    file_count++;
                }
            }
            closedir(dir);
        }
        else {
            // Failed to open directory
            perror("");
            exit(EXIT_FAILURE);
        }
    }

    // Display results
    if (file_count == 0) {
        write(sd, fileNotFound, sizeof(fileNotFound));
        printf("None of the files were found.\n");
    }
    else {
        char files_found[] = "The following files with the specified extensions were found:";
        write(sd, files_found, sizeof(files_found));
        printf("The following files were found:\n");
        for (int i = 0; i < file_count; i++) {
            printf("%s\n", file_list[i]);
            write(sd, file_list[i], sizeof(file_list[i]));
        }
    }

    // Compress the found files if necessary
    if (file_count > 0) {
        compress_files(file_list, file_count);
        send_file(sd, unzip);
    }
}

/*function to get tar file of requested extensions*/
void getTargz(char* extensions[], int numFiles, bool unzip, int sd) {
    DIR* dir;
    struct dirent* ent;
    int file_count = 0;
    char* file_list[1000];

    // Check if each file with the specified extension exists in the current directory
    if ((dir = opendir(".")) != NULL) {
        while ((ent = readdir(dir)) != NULL) {
            for (int i = 0; i < numFiles; i++) {
                const char* extension = extensions[i];
                size_t extension_length = strlen(extension);
                size_t name_length = strlen(ent->d_name);
                if (name_length > extension_length && strcmp(ent->d_name + name_length - extension_length, extension) == 0) {
                    file_list[file_count] = strdup(ent->d_name);
                    file_count++;
                    break;
                }
            }
        }
        closedir(dir);
    }
    else {
        // Failed to open directory
        perror("");
        exit(EXIT_FAILURE);
    }

    // Display results
    if (file_count == 0) {
        char none_extensions[] = "None of the files with the specified extensions were found.\n";
        write(sd, none_extensions, sizeof(none_extensions));
        printf("None of the files with the specified extensions were found.\n");
    }
    else {
        char extensions_found[] = "The following files with the specified extensions were found:";
        write(sd, extensions_found, sizeof(extensions_found));
        printf("The following files with the specified extensions were found:\n");
        for (int i = 0; i < file_count; i++) {
            printf("%s\n", file_list[i]);
            write(sd, file_list[i], sizeof(file_list[i]));
        }
    }

    // Compress the found files if necessary
    if (file_count > 0) {
        compress_files(file_list, file_count);
        send_file(sd, unzip);
    }
}

/*function to send ack to client*/
void sendAck(int sd) {
    write(sd, ack, sizeof(ack));
}

/*function to get ack from client*/
bool getAck(int sd) {
    char ackMsg[sizeof(ack)];
    read(sd, ackMsg, sizeof(ackMsg));
    if (strcmp(ack, ackMsg) == 0)
        return true;
    else
        return false;
}


int main(int argc, char* argv[]) {
    int sd, client, portNumber, status;
    struct sockaddr_in socAddr;

    // check for arg validation
    if (argc != 2) {
        printf("Please run with: %s <Port Number>\n", argv[0]);
        exit(0);
    }

    // create socket 
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }

    // create socket address
    socAddr.sin_family = AF_INET;
    socAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    // on port no - portNumber
    sscanf(argv[1], "%d", &portNumber);
    socAddr.sin_port = htons((uint16_t)portNumber);
    
    // binding socket 
    bind(sd, (struct sockaddr*)&socAddr, sizeof(socAddr));

    // lisent to server socket
    listen(sd, MAX_CLIENTS);

    while (1) {
        
        // accept connection
        client = accept(sd, NULL, NULL);

        // child process to handle each client
        if (!fork())
        {
            printf("Client/%d is connected!\n", getpid());
            getConnectedClient(client);
        }

        close(client);
    }
}