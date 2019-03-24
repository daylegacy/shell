#include "sql_parser.h"
//#include <sys/types>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "vector.h"
#define COMMANDS_SIZE 10


struct cmd{
    vector<char *> argv;
    int argc;
};

struct shell{
    vector<cmd> commands;
    int commands_n;

    shell(){}
    ~shell(){}

    void run(){
        while(1){
            
            printf(">");
            
            commands_n=parse_input();
            print_commands();
            if(commands_n>0 && !strcmp(commands[0].argv[0], "quit")){
                break;
            }//проблема в том что экзеку нужен null
            if(commands[0].argc)execute_command();
            //clear_stdin();
        }
        
    }

    int execute_command(){
        const char * name = commands[0].argv[0];
        pid_t pid;
        int status=0;
        int rv;
        print_command(commands[0]);

        switch(pid=fork()) {
        case -1:
                perror("fork"); /* произошла ошибка */
                exit(1); /*выход из родительского процесса*/
        case 0:
                printf(" CHILD: created!\n");
                printf(" CHILD: my PID -- %d\n", getpid());
                printf(" CHILD: PID my parent -- %d\n", getppid());
                //printf("CHILD:: %s\n", *(commands[0].argv+1));
                execvp(name, (char * const *)(&commands[0].argv[0]));
                printf(" CHILD: exit!\n");
                //exit(0);
        default:
                printf("PARENT: PID -- %d\n", getpid());
                printf("PARENT: PID child %d\n",pid);
                printf("PARENT: wait until exit()...\n");

                wait(&status);
                printf("PARENT: childs rv:%d\n", WEXITSTATUS(status));
        }
    }

    int parse_input(){
        int cur_command=0;
        while(1){
            int res=0;
            parser parser(commands[cur_command].argv, ARRAY_SIZE);
            commands[cur_command].argc = parser.split_to_tokens(stdin, &res);
            //printf("split_to_tokens ended, argc=%d \n", commands[cur_command].argc);
            if(commands[cur_command].argc==0) {cur_command--;break;}
            if(res==0)break;
            cur_command++;
        }
        //clear_stdin();


        return cur_command+1;

    }
    void print_commands(){
        for(int i=0; i<commands_n; i++){
            printf("com%d: ", i);
            for(int k=0; k<commands[i].argc; k++){
                printf("\"%s\" ", commands[i].argv[k]);
            }
            printf("\n");
        }
    }
    void print_command(cmd command){
        printf("name = %s, argc = %d\n", command.argv[0], command.argc);
        for(int i=0;i<command.argc;i++){
            printf("\targv[%d] = %s\n", i, command.argv[i]);
        }
    }
    void clear_stdin(){
        char buf[1000];
        char * bufp = &buf[1];

        bufp = fgets(buf, 1000, stdin);
        if(!bufp){;}
    }
};