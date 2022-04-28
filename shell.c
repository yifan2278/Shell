#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <setjmp.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#define BUFFER_SIZE 80
#define LIST_SIZE 10
 
int count = 0;
int begin = 0;
int end = 0;
static char list[LIST_SIZE][BUFFER_SIZE];
static sigjmp_buf env;
struct sigaction handler;
int fd = 0;
 
 
/* the signal handler function */
void handle_SIGINT(){
   int j = 0;
   int len = count<10? count:10;
   sigaddset(&handler.sa_mask, SIGINT);
   fflush(stdout);
   printf("\n");
   printf("The most recent ten commands are: \n");

   for(j = 0; j < len; j++){
       printf("%i. %s\n", j+1, list[(begin+j)%10]);
   }
   siglongjmp(env, 40);
}

/*reading content from saved file */
void readHistory(){
   int ret = 1;
   if(access("zhu2278.history", F_OK) != -1) {
       fd = open("zhu2278.history", O_RDWR, 00777);/*this gaves r/w permission*/
       if (fd < 0) {
           close(fd);
           perror("failed to open");
           exit(1);
       }
       while(ret != 0) {
           ret = read(fd, list[count], BUFFER_SIZE);
           printf("readed content is %s", list[count]);
	   printf(", store in position %i\n", end);
           count++;  
	   end++;
           if (count > 10){
			begin++;
			begin = begin%10;
			end = end%10;
	  }
          /*printf("count is %s\n", list[count-1]);*/
       }
      printf("ending at %i", end);
   } else { /* create new file if first time*/
       fd = open("zhu2278.history", O_WRONLY|O_CREAT|O_TRUNC, 00777);
       if (fd < 0) {
           close(fd);
           perror("failed to create");
           exit(1);
       }
   }
}
/* writes content in array to file*/
void writeHistory(){
   write(fd, (void *)list[end%10], BUFFER_SIZE); 
   printf("Currently writing %s\n", list[end%10]);
}

void setup(char inputBuffer[], char *args[],int *background)
{
   int length, /* # of characters in the command line */
       i,  /* loop index for accessing inputBuffer array */
       start,  /* index where beginning of next command parameter is */
           ct; /* index of where to place the next parameter into args[] */
       int m = 0;
       ct= 0;
       start = -1;
       length = read(STDIN_FILENO, inputBuffer, BUFFER_SIZE);
   /* read what the user enters on the command line */
      
       if(errno == EINTR){
           fflush(0);
           length = read(STDIN_FILENO, inputBuffer, BUFFER_SIZE);
           fflush(0);
       }
/* It fillters out r x and r command, and normal command, and store them */
       if(inputBuffer[0]=='r' && (inputBuffer[1]=='\n' || inputBuffer[1] ==' ')){
           if(inputBuffer[1]==' '){
               /* find the most recent command */
               for(m = 0; m < count && list[(m+begin)%10][0] != inputBuffer[2]; m++);
               if (m == begin+count){
                   printf("No matched commands in the history\n");
                   fflush(stdout);
               }
               else{
                   strcpy(inputBuffer, list[m]);
               }
           }
           else{
               if(count==0){
                   printf("Commands list is empty\n");
                   fflush(stdout);             
               }
               else{
                   strcpy(inputBuffer, list[(10+end-1)%10]);
               }
           }
       }
       /* the command list is not full */     
       if(count<10){
           strcpy(list[end],inputBuffer);
	   writeHistory();
	   end++;
           count++;
       } else {/* move command forward to the buffer*/
           /*put the last command into the inputBuffer */
           strcpy(list[end%10],inputBuffer);
           writeHistory();
	   begin++;
	   begin = begin%10;
           end++;
	   begin = begin%10;
	   count++;
       }
   
       length = strlen(inputBuffer);
  
   /* examine every character in the inputBuffer */
       for (i = 0; i < length; i++) {
           switch (inputBuffer[i]){
               case ' ':
               case '\t':  /* argument separators */
                   if(start != -1){
                       args[ct] = &inputBuffer[start];  /* set up pointer */
                       ct++;
                   }
                   inputBuffer[i] = '\0'; /* add a null char; make a C string */
                   start = -1;
                   break;
          
               case '\n':  /* should be the final char examined */
                   if (start != -1){
                       args[ct] = &inputBuffer[start];    
                       ct++;
                       }
                   inputBuffer[i] = '\0';
                   args[ct] = NULL; /* no more arguments to this command */
                   break;
 
               case '&':
                   *background = 1;
                   inputBuffer[i] = '\0';
                   break;
          
               default :     /* some other character */
                   if (start == -1)
                   start = i;
       }
   }   
   args[ct] = NULL; /* just in case the input line was > 80 */
}
int main(void){
 
   char inputBuffer[BUFFER_SIZE]; /* buffer to hold the command entered */
   int background;             /* equals 1 if a command is followed by '&' */
   pid_t pid;
   char *args[BUFFER_SIZE/2+1];/* command line (of 80) has max of 40 arguments */
  
   readHistory();
   sigsetjmp(env,1);
   sigemptyset(&handler.sa_mask);
   handler.sa_flags = SA_RESTART;
   handler.sa_handler =handle_SIGINT;
   sigaction(SIGINT, &handler, NULL);
  
 
   while (1){          
       background = 0;
       printf("COMMAND->");
           fflush(stdout);
           setup(inputBuffer, args, &background);      /* get next command */
       pid = fork();
       if(pid<0){ /* If fork failed, print error message */
               fprintf(stderr,"FORK FAILED");
               fflush(stdout);
               exit(-1);
       }
       if(pid==0){/*the child process will invoke execvp()*/
               execvp(inputBuffer, args);
               fflush(0);
               exit(0);
              
       }
       if(background==0){/*If backgroud == 0, the parent will wait */
               wait(NULL);
       }
   }
exit(0);
}
 
 


