#ifndef SQL_PARSER_H
#define SQL_PARSER_H
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <memory>
#include "vector.h"
#define	TOKEN_SIZE (50)
#define ARRAY_SIZE (20)

class parser {
	vector<char*> tokens;
	vector<char> cur_token;
public:
	parser(){
		vector<char*> tokens;
		vector<char> cur_token;
	}
	~parser(){}
	explicit parser(vector<char *>* array) {
		tokens = *array;
		nullify_token();
		nullify_tokens();
	}
	vector<char*> &split_to_tokens(FILE * input, int*res) {
		int k=0;
		int literal=0;
		int smb=0;
		int next_smb = 0;
		int prev_smb=0;
		int cmd_end=0;
		int jmp=0;
		while (!(smb=='\n' && prev_smb!='\\')) {
			next_smb=fgetc(input);

			if(literal){
				switch (smb)
				{
				case '\"':
					if(prev_smb!='\\'){
						literal=0;
						add_token();
					}
					else{
						add_smb(smb);
					}
					break;
				default:	add_smb(smb);
					break;
				}
			}
			else{
				switch (smb)
				{
				case '\n': 
					if(prev_smb!='\\'){
						add_token(); cmd_end=2; //end of command
					}
					break;
				case ' ':
					if(prev_smb!='\\'){
						add_token();
					}
					else{
						add_smb(smb);
					}
					break;
				case '&':	add_token(); add_smb('&');
					if (next_smb == '&')		{ add_smb('&');jmp=1;}
					add_token(); break;
				case '|':	add_token();
					if (next_smb == '|'){
						add_smb('|');add_smb('|');add_token();jmp=1;
					}
					else{
						cmd_end=1;
					}
					break;
				case '>':	add_token(); add_smb('>');
					if (next_smb == '>')		{ add_smb('>'); jmp=1;}
					add_token(); break;	
				case '<':	add_token(); add_smb('<');
					if (next_smb == '<')		{ add_smb('<'); jmp=1;}
					add_token(); break;
				case '\"':
					if(prev_smb!='\\'){
						literal=1;add_token();
					}
					else{
						add_smb(smb);
					}
					break;
				case '\\':
					break;
				default:	add_smb(smb);
					break;
				}
			}
			if(cmd_end==2){ 
				add_token();
				*res=0;
				return tokens;
			}
			if(cmd_end==1){ 
				add_token();
				ungetc(next_smb, input);
				*res=1;
				return tokens;
			}
			
			if(!jmp){
				prev_smb = smb;
				smb = next_smb;
			}
			else{
				prev_smb = next_smb;
				smb = fgetc(input);
				jmp=0;
			}
			//printf("k=%d smb=%c int=%d\n", k, smb, smb);
			//if(smb==EOF)printf("pisdets\n");
			
			//printf("k=%d next_smb=%c int=%d\n", k, next_smb, next_smb);
			k++;
		}
		add_token();
		//ungetc(next_smb, input);
		
		return tokens;
	}
	
	void add_token() {
		cur_token.push_back('\0');
		if (cur_token.gp() && cur_token[0]!=0) {
			char * new_token = (char *)malloc(sizeof(char)*(strlen(cur_token.gp())+1));
			strcpy(new_token, cur_token.gp());
			tokens.push_back((new_token));
		}
		nullify_token();
	}
	void add_smb(int c) {
		if(c==0)return;
		cur_token.push_back(c);
	}
	void nullify_token() {
		cur_token.setsize(0);
	}
	void nullify_tokens() {}

};

#endif // !SQL_PARSER_H