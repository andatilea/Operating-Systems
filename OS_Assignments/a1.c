#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

void print_list_directory(char * path, int recursive, int has_perm_execute, int end_name_on, char*end_name){
	DIR * dir1;
	struct dirent *file;

	dir1=opendir(path);
	if(path[0]=='.'){
		strcpy(path,path+2);
	}
	if(dir1){
		

		file=readdir(dir1);
		
		while(file){
				if(strcmp(file->d_name,".")!=0&&strcmp(file->d_name,"..")!=0){
					struct stat fileStat;
					char file1[401];
					int ok=0;

					if(recursive==1&&file->d_type==DT_DIR){
						strcpy(file1,path);
						strcat(file1,"/");
						strcat(file1,file->d_name);
						print_list_directory(file1,recursive,has_perm_execute,end_name_on,end_name);
					}
				
					if(has_perm_execute==0 && end_name_on==0){
						ok=1;

					}else if(has_perm_execute==1){
						strcpy(file1,path);
						strcat(file1,"/");
						strcat(file1,file->d_name);
						stat(file1,&fileStat);
						
						if((fileStat.st_mode & S_IXUSR) ){
							ok=1;
						}
					}else if(end_name_on==1){
						ok=1;
						int len_name=strlen(end_name);
						if(len_name>strlen(file->d_name)){
							ok=0;
						}
						int len_max=strlen(file->d_name);
						for(int k=len_name-1;k>=0;k--){
							
								if(file->d_name[len_max-1-k]!=end_name[len_name-1-k]){
									ok=0;
								}
						}
					}

					if(ok==1){
						printf("%s",path);
						printf("/");
						printf("%s",file->d_name);
						printf("\n");
					}


				}

			



			file=readdir(dir1);
		}

		closedir(dir1);


		
			
	}else{
		perror("Could not open directory");
	}
}

char path[300];

void list_directory(char **argv,int nr){
	
	int i,has_perm_execute=0,end_name_on=0,recursive=0;
	char end_name[40];
	strcpy(path,"./");

    for(i=2;i<nr;i++){
    	
        if(strncmp(argv[i],"path=",5)==0){
     	
        	strcat(path,argv[i]+5);
        }else if(strncmp(argv[i],"name_ends_with=",15)==0) {
        	
        	strcpy(end_name,argv[i]+15);
        	end_name_on=1;
        }else if(strcmp(argv[i],"has_perm_execute")==0){
        	has_perm_execute=1;

        }else if(strcmp(argv[i],"recursive")==0){
        	recursive=1;
        }
    }
    printf("SUCCESS\n");
    print_list_directory(path,recursive,has_perm_execute,end_name_on,end_name);
   


	

}

void parse_path(char * path1){
	
	int fd;
	char file[150];
	strcpy(file,"./");
	strcat(file,path1+5);
	
	fd=open(file,O_RDONLY);
	if(fd==-1){
		perror("Can't open file");
		return;
	}
	lseek(fd,-4,SEEK_END);
	char magic[4];
	read(fd,magic,4);

	if(strncmp(magic,"SiEu",4)==0){
		
		short headSize=0;
		lseek(fd,-6,SEEK_END);
		read(fd,&headSize,2);
		lseek(fd,-headSize,SEEK_END);
		
		int nr;
		int allowed_types[]={89,88,24,90,48,74};
		read(fd,&nr,4);
		
		
		if(nr>=123&&nr<=162){
			
			int nr_sect=0;
			read(fd,&nr_sect,1);
			if(nr_sect>=6&&nr_sect<=15){
				
				int i;
				char buff[28];
				int ok=1;
				for(i=0;i<=nr_sect;i++){
					read(fd,buff,28);
					int j;
					int ok1=0;
					for(j=0;j<6;j++){
						if((int)allowed_types[j]==(int)buff[19]){
							ok1=1;
						}

					}
					if(ok1!=1){
						ok=0;
					}
				}
				if(ok==1){
					printf("SUCCESS\n");
					printf("version=%d\n",nr);
					printf("nr_sections=%d\n",nr_sect );
					
					lseek(fd,-headSize+5,SEEK_END);
					char name[19];
					char type1;
					int sizeSect;

					for(i=1;i<=nr_sect;i++){
						lseek(fd,-headSize+5+28*(i-1),SEEK_END);
						printf("section%d: ",i);
						
						read(fd,name,19);
						
						read(fd,&type1,1);
						lseek(fd,4,SEEK_CUR);
						
						read(fd,&sizeSect,4);
						printf("%s %d %d\n",name,type1,sizeSect);

					}

					return;
				}else{
					printf("ERROR\nwrong sect_types");
				}


			}else{
				printf("ERROR\nwrong sect_nr");
			}
		}else{
			printf("ERROR\nwrong version\n");
		}

	}else{
		printf("ERROR\nwrong magic\n");
	}

	
	


}


void extract_section(char **data, int data_length){
	int section_nr,line_nr;
	char tmp[100];
	for(int i=2;i<data_length;i++){
		if(strncmp(data[i],"path=",5)==0){
			strcpy(path,"./");
			strcat(path,data[i]+5);
		}else if(strncmp(data[i],"section=",8)==0){
			strcpy(tmp,data[i]+8);
			section_nr=atoi(tmp);
		}else if(strncmp(data[i],"line=",5)==0){
			strcpy(tmp,data[i]+5);
			line_nr=atoi(tmp);
		}
	}
	printf("\nSection_nr: %d \n",section_nr);
	printf("\nSection_nr: %d \n",line_nr);
	printf("\nPath:%s\n",path);

}







int main(int argc, char **argv)
{
    if(argc >= 2){
        if(strcmp(argv[1], "variant") == 0){
            printf("51655\n");
        }
        else if(strcmp(argv[1],"list")==0){
        	list_directory(argv,argc);
        }
        else if(strcmp(argv[1],"parse")==0){
        	parse_path(argv[2]);
        }
        else if(strcmp(argv[1],"extract")==0){
        	extract_section(argv,argc);
        }


        
    }
    return 0;
}