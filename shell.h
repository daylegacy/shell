#include "parser.h"
//#include <sys/types>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vector.h"
#define COMMANDS_SIZE 10
#ifdef DEBUG
#define printD(...) \
	do{ \
		printf(__VA_ARGS__); \
	}while(0); 
#else
#define printD(...) \
	do{ \
	}while(0); 
#endif
struct cmd{
    vector<char *> argv;
    cmd(vector<char *> argv){
        this->argv = argv;
    }
};

struct shell{
    shell(){}
    ~shell(){}
    typedef vector<char *> cmd;
    void run(){
        while(1){
            vector<cmd> commands;
            printf(">");
            parse_input(commands);
            print_commands(commands);
            if(commands.getsize()>0 && 
                !strcmp(commands[0][0], "quit")){
                for(int i = 0; i<commands.getsize(); i++){
                    for(int k =0;k<commands[i].getsize();k++){
                        free(commands[i][k]);
                    }
                    //free(commands[i].gp());
                }
                break;
            }
            if(commands[0].gp())execute(commands);
            for(int i = 0; i<commands.getsize(); i++){
                for(int k =0;k<commands[i].getsize();k++){
                    free(commands[i][k]);
                }
                //free(commands[i].gp());
            }
        }
        
    }

    int execute(vector<cmd>& commands){
        pid_t pid;
        pid_t cur_pid;
        int pipe_out[2];
        int pipe_in[2];
        vector<pid_t> pids;// pids for forks
        auto shell_pid = getpid();
        printD("shell -- %d\n", shell_pid);
        int last = commands.getsize()-1;
        for(int i=0; i<commands.getsize();i++){
            cur_pid = getpid();
            int fd=0;
            if(cur_pid==shell_pid){
                printD(" i= %d\n", i);
                if(commands.getsize()!=1){ //initialising pipes
                    if(i==last || i==0){
                        if(i==0){
                            pipe(pipe_out);
                        }
                        if(i==last){
                            pipe_in[0] = pipe_out[0];
                            pipe_in[1] = pipe_out[1];
                        }
                    }
                    else{
                        pipe_in[0] = pipe_out[0];
                        pipe_in[1] = pipe_out[1];
                        pipe(pipe_out);
                    }
                }
                
                switch(pid=fork()) {
                case -1:
                    perror("fork"); 
                    exit(1); 
                case 0: //fork process
                    if(commands.getsize()!=1){
						if(i==0){
							close(pipe_out[0]);
                            dup2(pipe_out[1], STDOUT_FILENO);
						}
                        else if(i==last){
							close(pipe_in[1]);
							dup2(pipe_in[0], STDIN_FILENO);
                        }
                        else{
                            close(pipe_in[1]);
                            close(pipe_out[0]);
                            dup2(pipe_in[0], STDIN_FILENO);
                            dup2(pipe_out[1], STDOUT_FILENO);
                        }
                    }
                    if(commands[i].getsize()>2){
                        if(strcmp(commands[i][commands[i].getsize()-2], ">")==0){
                            printD("%s: detected > \n", commands[i][0]);
                            fd = open(commands[i][commands[i].getsize()-1], O_WRONLY|O_CREAT|O_TRUNC);
                            dup2(fd, STDOUT_FILENO);
                            commands[i][commands[i].getsize()-2]=NULL;
                        }
                        else if(strcmp(commands[i][commands[i].getsize()-2], ">>")==0){
                            printD("%s: detected >> \n", commands[i][0]);
                            fd = open(commands[i][commands[i].getsize()-1], O_WRONLY|O_APPEND|O_CREAT);
                            dup2(fd, STDOUT_FILENO);
                            commands[i][commands[i].getsize()-2]=NULL;
                        }
                        else{
                            commands[i].push_back(NULL);
                        }
                    }
                    else{
                        commands[i].push_back(NULL);
                    }
                    execvp(commands[i][0], commands[i].gp());
                    exit(-100);
                default: //shell
                    pids.push_back(pid);
                    printD("SHELL: PID -- %d PID child %d\n", getpid(),pid);
                }
                if(fd){
                    close(fd);
                }
				if(commands.getsize()!=1){ //close all descriptors after process finished
					if(i==0){
						close(pipe_out[1]);
					}
					else if(i==last){
						close(pipe_in[0]);
					}
					else{
						close(pipe_out[1]);
						close(pipe_in[0]);
					}
                }
            }
        }
        while(1){// check if all forks exited
            int n_of_finished=0;
            for(int i =0;i<pids.getsize();i++){
                if(pids[i]==0) {n_of_finished++;continue;}
                auto cur_pid=getpid();
                if(cur_pid == shell_pid){
                    int status=-5;
                    printD("SHELL: waiting for %d(%s)\n",pids[i], commands[i][0]);
                    pid_t return_pid = waitpid(pids[i], &status, WNOHANG); 
                    if (return_pid == -1) {
                        printD("ERROR child%d(%s)", pids[i], commands[i][0]);
                    } else if (return_pid == 0) {
                        printD("child%d(%s) still running\n",pids[i], commands[i][0]);
                    } else if (return_pid == pids[i]) {
                        printD("SHELL: child%d(%s) finished status = %d childs rv:%d\n", pids[i], commands[i][0], status, WEXITSTATUS(status));
                        pids[i] =0;
                    }
                }
            }
            if(n_of_finished==pids.getsize()) break;
            usleep(30000);
        }
        return 0;
    }

    int parse_input(vector<cmd>& commands){
        int cur_command=0;
        while(1){
            int res=0;
            parser parser;
            commands.push_back(std::move(parser.split_to_tokens(stdin, &res)));
            if(commands[cur_command].getsize()==0) {commands.pop();cur_command--;break;}
            if(res==0)break;
            cur_command++;
        }
        return cur_command+1;
    }
    void print_commands(vector<cmd>& commands){
        printD("print commands s=%d: \n", commands.getsize());
        for(int i=0; i<commands.getsize(); i++){
            printD("com%d: ", i);
            for(int k=0; k<commands[i].getsize(); k++){
                printD("\"%s\" ", commands[i][k]);
            }
            printD("\n");
        }
    }
    void print_command(vector<char *> &command){
        printD("name = %s, argc = %d\n", command[0], command.getsize());
        for(int i=0;i<command.getsize();i++){
            printD("\targv[%d] = %s\n", i, command[i]);
        }
    }
    void clear_stdin(){
        char buf[1000];
        char * bufp = &buf[1];

        bufp = fgets(buf, 1000, stdin);
        if(!bufp){;}
    }
};