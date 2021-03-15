#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/shm.h>
#include <sys/mman.h>

int req,resp;
int shm_fd;
char *shm, *file_data;

char input[20];

void connect(){
    char con[8]={7, 'C', 'O', 'N', 'N', 'E', 'C', 'T'};
    write(resp,con,sizeof(con));
}

void ping() {
	char ping[10] ={4,'P','I','N','G',4,'P','O','N','G'};
	unsigned int value=51655;
	write(resp, ping ,sizeof(ping));
	write(resp, &value, sizeof(value));
}

void create_shm()
{
    unsigned int size;
    
    read(req, &size, sizeof(size));
    shm_unlink ("/uHLKUN");
    shm_fd = shm_open ("/uHLKUN", O_CREAT | O_RDWR, 0664); // open
    ftruncate(shm_fd, size); //set size

    shm = (char*)mmap(
            0, 
            size, 
            PROT_READ |
            PROT_WRITE,
            MAP_SHARED, 
            shm_fd, 
            0);
        if(shm == MAP_FAILED){
            perror("Could not map the shared memory");
            return;
        }

    char create_shm[11]={10, 'C', 'R', 'E', 'A', 'T', 'E', '_', 'S', 'H', 'M'};
	write(resp,create_shm,sizeof(create_shm));

    if(shm_fd<0)
    {
        char error[6]={5, 'E', 'R', 'R', 'O', 'R'};
        write(resp,error,sizeof(error));
    }
    else
    {
        char success[8]={7,'S','U','C','C','E','S','S'};
        write(resp,success,sizeof(success));
    }
}

void write_to_shm()
{
    unsigned int offset;
    unsigned int value;

    read(req,&offset,sizeof(offset));
    read(req,&value,sizeof(value));

    char write_shm[13]={12, 'W', 'R', 'I', 'T', 'E', '_', 'T', 'O', '_', 'S', 'H', 'M'};
    write(resp,write_shm,sizeof(write_shm));

    if(offset<0 || offset+4>2774660) // + 4 because the unsinged int uses 4 bytes
    {
        char error[6]={5, 'E', 'R', 'R', 'O', 'R'};
        write(resp,error,sizeof(error));
    }
    else
    {
        //write byte by byte
        shm[offset + 3] = (value>>24&0xff);
        shm[offset + 2] = (value>>16&0xff);
        shm[offset + 1] = (value>>8&0xff);
        shm[offset + 0] = (value&0xff);
        
        char success[8]={7,'S','U','C','C','E','S','S'};
        write(resp,success,sizeof(success));

    }
}

unsigned int file_size;
void map_file(){
    char map[255];
    unsigned int map_size = 0;

    read(req, &map_size, 1);
    read(req, &map, map_size);
    map[map_size] = '\0';

    char map_string[9]={8, 'M', 'A', 'P', '_', 'F', 'I', 'L', 'E'};
    write(resp,map_string,sizeof(map_string));

    int fd = open(map, O_RDONLY);
    file_size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    file_data = (char*)mmap(
        NULL, 
        file_size, 
        PROT_READ, 
        MAP_SHARED, 
        fd, 
        0);
    if(file_data == (void*)-1){
	    char error[6]={5, 'E', 'R', 'R', 'O', 'R'};
        write(resp,error,sizeof(error));
    }
    else{
        char success[8]={7,'S','U','C','C','E','S','S'};
        write(resp,success,sizeof(success));
    }
}

void read_from_file_offset(){
    unsigned int offset;
    unsigned int no_of_bytes;

    read(req, &offset, sizeof(unsigned int));
    read(req, &no_of_bytes, sizeof(unsigned int));

    char read_string[22]={21, 'R', 'E', 'A', 'D' ,'_', 'F', 'R', 'O', 'M', '_', 'F', 'I', 'L', 'E', '_', 'O', 'F', 'F', 'S', 'E', 'T'};
    write(resp,read_string,sizeof(read_string));
    
    char value[no_of_bytes];
    int index = 0;
    if(shm != (void*)-1){
        if(file_data != (char*)-1){
            if(no_of_bytes + offset <= file_size) //check if to be read data is inside the file
            {
                for(int i = offset; i < offset + no_of_bytes; i++)
                {
                    value[index++] = file_data[i];
                }

                memcpy(shm, &value, no_of_bytes); //write data to shm

		        char success[8]={7,'S','U','C','C','E','S','S'};
		        write(resp,success,sizeof(success));
            }
            else
            {
                char error[6]={5, 'E', 'R', 'R', 'O', 'R'};
                write(resp,error,sizeof(error));
		    }
        }
        else
            {
                char error[6]={5, 'E', 'R', 'R', 'O', 'R'};
                write(resp,error,sizeof(error));
		    }
    }
    else
            {
                char error[6]={5, 'E', 'R', 'R', 'O', 'R'};
                write(resp,error,sizeof(error));
		    }
}

int main()
{
    char exit = 1;
    char input[20];
    unsigned int length;

    unlink("RESP_PIPE_51655");
    if(mkfifo("RESP_PIPE_51655",0600)!=0){
        perror("Error creating pipe");
        return 1;
    }
    req=open("REQ_PIPE_51655",O_RDONLY);
    if(req<0){
        perror("ERROR Cannot open the request pipe");
        return 1;
    }
    resp=open("RESP_PIPE_51655",O_WRONLY);
    connect();

    while(exit){
        read(req,&length,1);
        read(req,&input,length);
        input[length + 1]='\0';
        if(strstr(input, "PING"))
        {
            ping();
        }
        else if(strstr(input, "CREATE_SHM"))
        {
            create_shm();
        }
        else if(strstr(input, "WRITE_TO_SHM"))
        {
            write_to_shm();
        }
        else if(strstr(input, "MAP_FILE"))
        {
            map_file();
        }
        else if(strstr(input, "READ_FROM_FILE_OFFSET"))
        {
            read_from_file_offset();
        }
        else if(strstr(input, "EXIT"))
        {
            close(req);
            close(resp);
            unlink("RESP_PIPE_51655");
            exit = 0;
        }
        else
            //not implemented
            return -1;
    }
	return 0;
}
