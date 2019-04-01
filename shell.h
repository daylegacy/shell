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

struct shell{
    shell(){}
    ~shell(){}
    typedef vector<char *> cmd;
    typedef vector<const char *> mod;
    void run(){
        while(1){
            vector<cmd> commands;
            vector<mod> modifiers;
            //printf("$> ");
            int res = parse_input(commands, modifiers);
            print_commands(commands, modifiers);
            if(commands.getsize()>0 && 
                !strcmp(commands[0][0], "quit")){
                for(int i = 0; i<commands.getsize(); i++){
                    for(int k =0;k<commands[i].getsize();k++){
                        free(commands[i][k]);
                    }
                }
                break;
            }
            if(commands[0].gp())execute(commands, modifiers);
            for(int i = 0; i<commands.getsize(); i++){
                for(int k =0;k<commands[i].getsize();k++){
                    free(commands[i][k]);
                }
            }
            for(int i = 0; i<modifiers.getsize(); i++){
                for(int k =0;k<modifiers[i].getsize();k++){
                }
            }
            if (res==-1) break;
        }
        
    }

    int execute(vector<cmd>& commands, vector<mod>& modifiers){
        pid_t pid;
        pid_t cur_pid;
        int pipe_out[2];
        int pipe_in[2];
        int wait_pid=0;
        int end_pipe=0;
        int last_rv=-100;
        int to_execute=1;
        vector<pid_t> pids;// pids for forks
        vector<char * > coms;
        vector<int> fd_to_close;
        auto shell_pid = getpid();
        printD("shell -- %d\n", shell_pid);
        int last = commands.getsize()-1;
        for(int i=0; i<commands.getsize();i++){
            cur_pid = getpid();
            int fd=0;
            if(cur_pid==shell_pid){
                printD(" i= %d\n", i);
                if(i>0 && !is_mod("|", modifiers[i-1])){//not pipe need to check rvs
                    vector<int> rvs;
                    rvs.reallocate(pids.getsize());
                    printD("waiting for %d procs:\n", pids.getsize());
                    for(int i=0;i< pids.getsize();i++){
                        printD("%d \n", pids[i]);
                    }
                    while(1){// check if all forks exited
                        int n_of_finished=0;
                        for(int i =0;i<pids.getsize();i++){
                            if(pids[i]==0) {n_of_finished++;continue;}
                            int status=-5;
                            printD("SHELL: waiting for %d(%s)\n",pids[i], coms[i]);
                            pid_t return_pid = waitpid(pids[i], &status, WNOHANG); 
                            if (return_pid == -1) {
                                printD("ERROR child%d(%s)", pids[i], coms[i]);
                            } else if (return_pid == 0) {
                                printD("child%d(%s) still running\n",pids[i], coms[i]);
                            } else if (return_pid == pids[i]) {
                                rvs[i] = WEXITSTATUS(status);
                                printD("SHELL: child%d(%s) finished status = %d childs rv:%d\n", pids[i], coms[i], status, WEXITSTATUS(status));
                                pids[i] =0;
                            }
                        }
                        if(n_of_finished==pids.getsize()) break;
                        //usleep(30);
                    }
                    //assert(pids.getsize());
                    if(pids.getsize())last_rv = rvs[pids.getsize()-1];//care
                    coms.setsize(0);
                    pids.setsize(0);
                    if(i>0 && is_mod("&&", modifiers[i-1])){
                        printD("last rv = %d\n", last_rv);
                        if(last_rv==0) to_execute=1;
                        else to_execute=0;
                    }
                    if(i>0 && is_mod("||", modifiers[i-1])){
                        printD("last rv = %d\n", last_rv);
                        if(last_rv==0) to_execute=0;
                        else to_execute=1;
                    }
                }
                if(to_execute)
                {
                    if((is_mod("|", modifiers[i])&&commands.getsize()!=1) || (i>0 && is_mod("|", modifiers[i-1]))){ //pipe
                        //printf("mod = |\n");
                        if(is_mod("|", modifiers[i]) && !(i>0 && is_mod("|", modifiers[i-1]))){ //pipe begins
                            pipe(pipe_out);
                        }
                        else if(is_mod("|", modifiers[i]) && (i>0 && is_mod("|", modifiers[i-1]))){ //pipe continues
                            pipe_in[0] = pipe_out[0];pipe_in[1] = pipe_out[1];
                            pipe(pipe_out);
                        }
                        else if((i>0 && is_mod("|", modifiers[i-1])) && !is_mod("|", modifiers[i])){ //pipe ends
                            pipe_in[0] = pipe_out[0];
                            pipe_in[1] = pipe_out[1];
                        }
                    }
                    switch(pid=fork()) {
                    case -1:
                        perror("fork"); 
                        exit(1); 
                    case 0: //fork process
                        if((is_mod("|", modifiers[i])&&commands.getsize()!=1) || (i>0 && is_mod("|", modifiers[i-1]))){ //pipe
                            //printf("mod = |\n");
                            if(is_mod("|", modifiers[i]) && !(i>0 && is_mod("|", modifiers[i-1]))){ //pipe begins
                                close(pipe_out[0]); dup2(pipe_out[1], STDOUT_FILENO);
                            }
                            else if(is_mod("|", modifiers[i]) && (i>0 && is_mod("|", modifiers[i-1]))){ //pipe continues
                                close(pipe_in[1]);  close(pipe_out[0]);
                                dup2(pipe_in[0], STDIN_FILENO);
                                dup2(pipe_out[1], STDOUT_FILENO);
                            }
                            else if((i>0 && is_mod("|", modifiers[i-1])) && !is_mod("|", modifiers[i])){ //pipe ends
                                close(pipe_in[1]);  dup2(pipe_in[0], STDIN_FILENO);
                            }
                        }
                        printD("gs = %d\n",commands[i].getsize() );
                        if(commands[i].getsize()>2){//>/>>
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
                        exit(0);
                    default: //shell
                        pids.push_back(pid);
                        coms.push_back(commands[i][0]);
                        printD("SHELL: PID -- %d PID child %d\n", getpid(),pid);
                        if((is_mod("|", modifiers[i])&&commands.getsize()!=1) || (i>0 && is_mod("|", modifiers[i-1]))){ //pipe
                            //printf("mod = |\n");
                            if(is_mod("|", modifiers[i]) && !(i>0 && is_mod("|", modifiers[i-1]))){ //pipe begins
                                close(pipe_out[1]);
                            }
                            else if(is_mod("|", modifiers[i]) && (i>0 && is_mod("|", modifiers[i-1]))){ //pipe continues
                                close(pipe_out[1]);
                                fd_to_close.push_back(pipe_in[0]);
                            }
                            else if((i>0 && is_mod("|", modifiers[i-1])) && !is_mod("|", modifiers[i])){ //pipe ends
                                fd_to_close.push_back(pipe_in[0]);
                            }
                        }
                        if(strcmp(commands[i][0], "cd")==0){
                            chdir(commands[i][1]);
                            to_execute=0;
                        }
                    }
                    if(fd){ //>/>>
                        close(fd);
                        fd =0;
                    }

                }
                
            }
        }
        for(int i=0;i<fd_to_close.getsize();i++){
            close(fd_to_close[i]);
        }
        usleep(3000);
        printD("absolutely last chechk waiting for %d procs\n", pids.getsize());
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
            usleep(3000);
        }
        
        return 0;
    }

    int parse_input(vector<cmd>& commands, vector<mod> & modifiers){
        int cur_command=0;
        while(1){
            int res=0;
            parser parser;
            commands.push_back(std::move(parser.split_to_tokens(stdin, &res, modifiers)));
            if(commands[cur_command].getsize()==0) {
                commands.pop();cur_command--;
                modifiers.pop();
            }
            if(res==0)break;
            if(res==-1) return -1;
            cur_command++;
        }
        return cur_command+1;
    }
    void print_commands(vector<cmd>& commands, vector<mod> & modifiers){
        printD("print commands s=%d: \n", commands.getsize());
        for(int i=0; i<commands.getsize(); i++){
            printD("com%d: ", i);
            for(int k=0; k<commands[i].getsize(); k++){
                printD("(%s) ", commands[i][k]);
            }
            
            printD("mods%d: ", i);
            for(int k=0; k<modifiers[i].getsize(); k++){
                printD("(%s) ", modifiers[i][k]);
            }
            printD("\n");
        }
    }
    int is_mod(const char * mod, vector<const char*> modifiers){
        for(int k=0; k<modifiers.getsize(); k++){
            if(strcmp(mod, modifiers[k])==0){
                return 1;
            }
        }
        return 0;
    }
    void print_command(vector<char *> &command){
        printD("name = %s, argc = %d\n", command[0], command.getsize());
        for(int i=0;i<command.getsize();i++){
            printD("\targv[%d] = %s\n", i, command[i]);
        }
    }
    // void clear_stdin(){
    //     char buf[1000];
    //     char * bufp = &buf[1];

    //     bufp = fgets(buf, 1000, stdin);
    //     if(!bufp){;}
    // }
};