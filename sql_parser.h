#ifndef SQL_PARSER_H
#define SQL_PARSER_H
#include <cstring>
#include <cstdlib>
#include <cassert>
#include <cstdio>
#define	TOKEN_SIZE (50)
#define ARRAY_SIZE (20)

class parser {
	char **tokens;
	char cur_token[TOKEN_SIZE+1];
	int token_n = 0;
	int tokens_size = 0;
	int smb_n = 0;
public:
	parser(){;}
	explicit parser(char ** array, int size) {
		tokens = array;
		tokens_size = size;
		nullify_token();
		nullify_tokens();
	}
	int split_to_tokens(FILE * input, int*res) {
		this->token_n = 0;
		this->smb_n = 0;
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
				return token_n;
			}
			if(cmd_end==1){ 
				add_token();
				ungetc(next_smb, input);
				*res=1;
				return token_n;
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
		//printf("LLLL\n");
		
		return token_n;
	}
	
	void add_token() {
		assert(token_n < ARRAY_SIZE);
		if (cur_token && cur_token[0]!=0) {
			assert(cur_token);
			//assert(tokens[token_n]);
			strcpy(tokens[token_n], cur_token);
			token_n++;
			nullify_token();
		}
	}
	void add_smb(int c) {
		if(c==0)return;
		assert(smb_n < TOKEN_SIZE);
		cur_token[smb_n] = c;
		smb_n++;
	}
	void nullify_token() {
		memset((void *)cur_token, 0, (TOKEN_SIZE + 1) * sizeof(char));
		smb_n = 0;
	}
	void nullify_tokens() {
		for(int i =0;i<tokens_size;i++){
			memset((void *)tokens[i], 0, (TOKEN_SIZE + 1) * sizeof(char));
		}
	}

};

#endif // !SQL_PARSER_H