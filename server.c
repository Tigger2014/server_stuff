
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h> 
extern char **environ; /* the environment */
#define NUMBER_BASE_DECIMAL 10
#define MAX_PORT_ARGUMENT_LENGTH 5
#define MAX_PORT 65535
#define BUFSIZE 1024
// 256 is somewhat of a magic number.
// 256 = sizeof(char) , 1 byte wide.
#define MAX_COMMAND_LENGTH 256
 
int main(int argc, char **argv)
{
	 FILE *stream;          /* stream version of childfd */
    int childfd;           /* child socket */
    int parentfd;           /* child socket */  
    struct sockaddr_in serveraddr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	char* method=calloc(1,BUFSIZE);  /* request method */
  char* uri=calloc(1,BUFSIZE);     /* request uri */
  char* buf=calloc(1,BUFSIZE);     /* message buffer */
  char* filename=calloc(1,BUFSIZE);     /* request uri */
   char* version=calloc(1,BUFSIZE);     /* message buffer */
    int wait_status;       /* status from wait */
      
  int pid;               /* process id from fork */
    // Check the argument count. we need 2 <port>
    if(argc != 2){
		fprintf(stderr,"Needs 2 params\n");
		exit (EXIT_FAILURE);
	}
    
    // Port Argument Santizating and Paranoia
    // if argv[1]==NULL then chances are the program hasn't been executed
    // via a standard shell. Something fishy could well be going on
    if(argv[1]==NULL){
		fprintf(stderr,"Really! NULL! Really! how the fuck did you manage that\n");
		exit (EXIT_FAILURE);
	}
	// if the length of the string is zero then again this is very fishy indeed
	int port_argument_length  = strnlen(argv[1],MAX_PORT_ARGUMENT_LENGTH);
	if(port_argument_length == 0){
		fprintf(stderr,"Empty String\n");
		exit (EXIT_FAILURE);
	}
	// check the first character of port argument is digit.
	if(!isdigit(argv[1][0])){
		fprintf(stderr,"Port NUMBER - have you got the arguments wrong way round?\n");
		exit (EXIT_FAILURE);
	}
	// Valid port numbers are 0 to 65535 so the length of the string 
	// we are converting should be no greater than 5 characthers long
	if(port_argument_length > MAX_PORT_ARGUMENT_LENGTH){
		fprintf(stderr,"Port Argument must be between 0-65535, It's the fucking internet\n");
		exit(EXIT_FAILURE);
	}
	// Fuck it lets null terminate the string here because we know we only want only 
	// want five or less
	argv[1][port_argument_length]= NULL ;
	
	
	
    // Set up a char with a value of NULL this is the same as char endptr='\0'

	char* endptr;
/********************************** Yeah Baby Time to do some real work ******************/
    fprintf(stderr,"argv[1] %s %d\n",argv[1],strlen(argv[1]));
    // port port unsigned short. 16 bits on every platform. should be safe and portable 
    unsigned short port = (unsigned short)strtoul(argv[1],&endptr,NUMBER_BASE_DECIMAL);
    //fprintf(stderr,"endptr %p",endptr);

    // These endptr should be redundant because we checked the input
    // Yeah and that sort of thinking is what lead to pwnage    
    if(endptr == NULL){
		fprintf(stderr,"FFS We've check this once, strutol has probably been comprisemised\n");
		exit (EXIT_FAILURE);
	}	
	if(endptr == argv[1]){
		fprintf(stderr,"Hmmm zero length string\n");
		exit (EXIT_FAILURE);
	}	
	if(*endptr != '\0'){
		fprintf(stderr,"String terminator missing, buffer overflows aren't my bag i'm afraid '%d'\n",endptr);
		exit (EXIT_FAILURE);
	}
	
	int command_string_length = strnlen(argv[2],MAX_COMMAND_LENGTH) ;
	
	
	if(command_string_length == 0){
		fprintf(stderr,"Empty String\n");
		exit (EXIT_FAILURE);
	}
	if( command_string_length > MAX_COMMAND_LENGTH){
		fprintf(stderr,"Not a fucking chance\n");
		exit (EXIT_FAILURE);
	}
    char *command_string = argv[1];

   
     /* open socket descriptor */
	parentfd = socket(AF_INET, SOCK_STREAM, 0);
	if (parentfd < 0) 
		fprintf(stderr,"ERROR opening socket");
    
 /* allows us to restart server immediately */
  int optval = 1;
  setsockopt(parentfd, SOL_SOCKET, SO_REUSEADDR, 
	     (const void *)&optval , sizeof(int));

  /* bind port to socket */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)port);
  if (bind(parentfd, (struct sockaddr *) &serveraddr, 
	   sizeof(serveraddr)) < 0) 
    fprintf(stderr,"ERROR on binding");

  /* get us ready to accept connection requests */
  if (listen(parentfd, 5) < 0) /* allow 5 requests to queue up */ 
    fprintf(stderr,"ERROR on listen");

  /* 
   * main loop: wait for a connection request, parse HTTP,
   * serve requested content, close connection.
   */
  int clientlen = sizeof(clientaddr);
    while(1)
    {

		childfd = accept(parentfd, (struct sockaddr *) &clientaddr, &clientlen);
    if (childfd < 0) 
      fprintf(stderr,"ERROR on accept");
    

    /* open the child socket descriptor as a stream */
		if ((stream = fdopen(childfd, "r+")) == NULL)
				fprintf(stderr,"ERROR on fdopen");

    /* get the HTTP request line */
    fgets(buf, BUFSIZE, stream);
    printf("%s", buf);
    sscanf(buf, "%s %s %s\n", method, uri, version);
	
	//uri;
	uri++;
	printf("filename: %s\n",uri);
	struct stat sbuf;
    /* make sure the file exists */
    if (stat(uri, &sbuf) < 0) {
	  printf("no file %s\n", uri);
      fclose(stream);
      
      //close(childfd);
    }else {
	
		if (!(S_IFREG & sbuf.st_mode) || !(S_IXUSR & sbuf.st_mode)) {
			printf("no mode %s\n", uri);
			fclose(stream);
			//close(childfd);
		}else{
			

			strcpy(filename, "./");
      strcat(filename, uri);
		 /* create and run the child CGI process so that all child
         output to stdout and stderr goes back to the client via the
         childfd socket descriptor */
      pid = fork();
      if (pid < 0) {
	fprintf(stderr,"ERROR in fork");
	exit(1);
      }
      else if (pid > 0) { /* parent process */
	wait(&wait_status);
      }
      else { /* child  process*/
	close(0); /* close stdin */

	if (execve(filename, NULL, environ) < 0) {
	  fprintf(stderr,"ERROR in execve");
	}
      }
    }
			fclose(stream);
		}
	}
    //close(childfd);
    
  
}
