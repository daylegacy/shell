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
	vector<char*> modifiers;
	vector<char> cur_token;
public:
	parser(){
	}
	~parser(){}
	explicit parser(vector<char *>* array) {
		tokens = *array;
		nullify_token();
		nullify_tokens();
	}
	vector<char*> &split_to_tokens(FILE * input, int*res, vector<vector<const char*>> & modifiers) {
		vector<const char*> mod;
		int k=0;
		int literal1=0;
		int literal2=0;
		int smb=0;
		int next_smb = 0;
		int prev_smb=0;
		int cmd_end=0;
		int jmp=0;
		int is_protected=0;
		while (!(smb=='\n' && !is_protected && !literal1 && !literal2)) {
			next_smb=fgetc(input);
			assert(!(literal1 && literal2));
			if(literal1){
				switch (smb)
				{
				case '\n': is_protected=0;
					break;
				case '\"':
					if(!is_protected){
						literal1=0;
						add_token();
					}
					else{
						is_protected=0;
						add_smb(smb);
					}
					break;
				case '\\':
					if(!is_protected){
						is_protected=1;
					}
					else{
						is_protected=0;
						add_smb(smb);
					}
					break;
				default:	add_smb(smb);
					is_protected=0;
					break;
				}
			}
			else if(literal2){
				//assert(false);
				switch (smb)
				{
				case '\n': is_protected=0;
					break;
				case '\'':
					literal2=0;
					add_token();
					is_protected=0;
					break;
				default:	add_smb(smb);
					is_protected=0;
					break;
				}
			}
			else{
				switch (smb)
				{
				case '\n': 
					if(!is_protected){
						add_token(); cmd_end=2; //end of command
					}
					is_protected=0;
					break;
				case ' ':
					if(!is_protected){
						add_token();
					}
					else{
						add_smb(smb);
					}
					is_protected=0;
					break;
				case '&':	add_token();
					if (next_smb == '&'){
						mod.push_back("&&");
						cmd_end=1;
					}
					else{
						mod.push_back("&");
						cmd_end=1;
					}
					is_protected=0;
					break;	
				case '|':	add_token();
					if (next_smb == '|'){
						cmd_end=1;
						mod.push_back("||");
						//add_smb('|');add_smb('|');add_token();jmp=1;
					}
					else{
						mod.push_back("|");
						cmd_end=1;
					}
					is_protected=0;
					break;
				case '>':	add_token(); add_smb('>');
					if (next_smb == '>')		{ add_smb('>'); jmp=1;}
					add_token(); is_protected=0;break;	
				case '<':	add_token(); add_smb('<');
					if (next_smb == '<')		{ add_smb('<'); jmp=1;}
					add_token(); is_protected=0;break;
				case '\"':
					if(!is_protected){
						literal1=1;add_token();
					}
					else{
						add_smb(smb);
					}
					is_protected=0;
					break;
				case '\'':
					if(!is_protected){
						literal2=1;add_token();
					}
					else{is_protected=0;
						add_smb(smb);
					}
					is_protected=0;
					break;
				case '\\':
					if(!is_protected){
						is_protected=1;
					}
					else{
						add_smb(smb);
						is_protected=0;
					}
					break;
				case '#':
					while(fgetc(input)!='\n'){;}
					cmd_end=2;
					break;
				default:	add_smb(smb);
					is_protected=0;
					break;
				}
			}
			if(cmd_end==2){ 
				add_token();
				*res=0;
				
				modifiers.push_back(mod);
				return tokens;
			}
			if(cmd_end==1){ 
				add_token();
				ungetc(next_smb, input);
				*res=1;
				modifiers.push_back(mod);		
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
			k++;
		}
		add_token();
		//ungetc(next_smb, input);
		modifiers.push_back(mod);
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




// while (!(smb=='\n' && (prev_smb!='\\' && !literal1 && !literal2))) {
// 			next_smb=fgetc(input);
// 			if(literal1){
// 				switch (smb)
// 				{
// 				case '\n': 
// 					break;
// 				case '\"':
// 					if(prev_smb!='\\'){
// 						literal1=0;
// 						add_token();
// 					}
// 					else{
// 						add_smb(smb);
// 					}
// 					break;
// 				default:	add_smb(smb);
// 					break;
// 				}
// 			}
// 			else if(literal2){
// 				switch (smb)
// 				{
// 				case '\n': 
// 					break;
// 				case '\'':
// 					if(prev_smb!='\\'){
// 						literal2=0;
// 						add_token();
// 					}
// 					else{
// 						add_smb(smb);
// 					}
// 					break;
// 				default:	add_smb(smb);
// 					break;
// 				}
// 			}
// 			else{
// 				switch (smb)
// 				{
// 				case '\n': 
// 					if(prev_smb!='\\'){
// 						add_token(); cmd_end=2; //end of command
// 					}
// 					break;
// 				case ' ':
// 					if(prev_smb!='\\'){
// 						add_token();
// 					}
// 					else{
// 						add_smb(smb);
// 					}
// 					break;
// 				case '&':	add_token();
// 					if (next_smb == '&'){
// 						mod.push_back("&&");
// 						cmd_end=1;
// 					}
// 					else{
// 						mod.push_back("&");
// 						cmd_end=1;
// 					}
// 					break;	
// 				case '|':	add_token();
// 					if (next_smb == '|'){
// 						cmd_end=1;
// 						mod.push_back("||");
// 						//add_smb('|');add_smb('|');add_token();jmp=1;
// 					}
// 					else{
// 						mod.push_back("|");
// 						cmd_end=1;
// 					}
// 					break;
// 				case '>':	add_token(); add_smb('>');
// 					if (next_smb == '>')		{ add_smb('>'); jmp=1;}
// 					add_token(); break;	
// 				case '<':	add_token(); add_smb('<');
// 					if (next_smb == '<')		{ add_smb('<'); jmp=1;}
// 					add_token(); break;
// 				case '\"':
// 					if(prev_smb!='\\'){
// 						literal1=1;add_token();
// 					}
// 					else{
// 						add_smb(smb);
// 					}
// 					break;
// 				case '\'':
// 					if(prev_smb!='\\'){
// 						literal2=1;add_token();
// 					}
// 					else{
// 						add_smb(smb);
// 					}
// 					break;
// 				case '\\':
// 					break;
// 				default:	add_smb(smb);
// 					break;
// 				}
// 			}
// 			if(cmd_end==2){ 
// 				add_token();
// 				*res=0;
				
// 				modifiers.push_back(mod);
// 				return tokens;
// 			}
// 			if(cmd_end==1){ 
// 				add_token();
// 				ungetc(next_smb, input);
// 				*res=1;
// 				modifiers.push_back(mod);		
// 				return tokens;
// 			}
			
// 			if(!jmp){
// 				prev_smb = smb;
// 				smb = next_smb;
// 			}
// 			else{
// 				prev_smb = next_smb;
// 				smb = fgetc(input);
// 				jmp=0;
// 			}
// 			k++;
// 		}
// 		add_token();
// 		//ungetc(next_smb, input);
// 		modifiers.push_back(mod);
// 		return tokens;
// 	}