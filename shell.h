#include "parser.h"
//#include <sys/types>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "vector.h"
#define COMMANDS_SIZE 10


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
            if(commands.getsize()>0 && !strcmp(commands[0][0], "quit")){
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
        
        vector<int *> pipes;
        vector<pid_t> pids;
        for(int i =0; i< commands.getsize()-1;i++){
            auto pipefd = new int[2];
            pipe(pipefd);
            pipes.push_back(pipefd);
        }
        int pipes_n=commands.getsize()-1;
        auto shell_pid = getpid();
        printf("shell -- %d\n", shell_pid);
        int last = commands.getsize()-1;
        for(int i=0; i<commands.getsize();i++){
            cur_pid = getpid();
            if(cur_pid==shell_pid){
                printf(" i= %d\n", i);
                switch(pid=fork()) {
                case -1:
                    perror("fork"); 
                    exit(1); 
                case 0: //fork process
                    if(pipes_n){
                        if(i==0 || i==last){
                            if(i==0){
                                dup2(pipes[i][1], STDOUT_FILENO);//stdout->pipes[i][1]
                                //close(pipes[i][0]);//close unused read end for first
                                // close(pipes[i][0]);
                            }
                            if(i==last){
                                dup2(pipes[i-1][0], STDIN_FILENO);//stdin->pipes[i-1][0]
                                //close(pipes[i-1][1]);
                            }
                        }
                        else{
                            dup2(pipes[i][1], STDOUT_FILENO);
                            dup2(pipes[i-1][0], STDIN_FILENO);
                            //close(pipes[i][0]);
                            //close(pipes[i-1][1]);
                        }
                    }
                    commands[i].push_back(NULL);
                    execvp(commands[i][0], commands[i].gp());

                    if(pipes_n){// if commands[i][0] didnt exited
                        if(i==0 || i==last){
                            if(i==0){
                                close(pipes[i][0]);//close write end for first
                            }
                            if(i==last){
                                close(pipes[i-1][0]); //close read end for last
                            }
                        }
                        else{
                            close(pipes[i][1]);//close write end
                            close(pipes[i-1][0]);//close read end
                        }
                    }
                    
                    exit(0);
                default: //shell
                    pids.push_back(pid);
                    printf("SHELL: PID -- %d PID child %d\n", getpid(),pid);
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
                    printf("SHELL: waiting for %d(%s)\n",pids[i], commands[i][0]);
                    pid_t return_pid = waitpid(pids[i], &status, WNOHANG); 
                    if (return_pid == -1) {
                        printf("ERROR child%d(%s)", pids[i], commands[i][0]);
                    } else if (return_pid == 0) {
                        printf("child%d(%s) still running\n",pids[i], commands[i][0]);
                    } else if (return_pid == pids[i]) {
                        printf("SHELL: child%d(%s) finished status = %d childs rv:%d\n", pids[i], commands[i][0], status, WEXITSTATUS(status));
                        pids[i] =0;
                    }
                }
            }
            if(n_of_finished==pids.getsize()) break;
            usleep(1000000);
        }
        

        for(int i =0; i< commands.getsize()-1;i++){
            delete [] pipes[i];
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
        printf("print commands: \n");
        for(int i=0; i<commands.getsize(); i++){
            printf("com%d: ", i);
            for(int k=0; k<commands[i].getsize(); k++){
                printf("\"%s\" ", commands[i][k]);
            }
            printf("\n");
        }
    }
    void print_command(vector<char *> &command){
        printf("name = %s, argc = %d\n", command[0], command.getsize());
        for(int i=0;i<command.getsize();i++){
            printf("\targv[%d] = %s\n", i, command[i]);
        }
    }
    void clear_stdin(){
        char buf[1000];
        char * bufp = &buf[1];

        bufp = fgets(buf, 1000, stdin);
        if(!bufp){;}
    }
};