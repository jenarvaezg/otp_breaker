#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <err.h>
#include <errno.h>
#include <signal.h>


#define MESSAGES_FILE "ctexts.txt" //important that last line of file has EOL EOF
									 //instead of only EOF
#define KEY_FILE "possible_key.txt"

#define MESSAGE_LENGTH 31



int key[MESSAGE_LENGTH];




char *
get_first_word(char *buf){
	memset(buf, 0, 1024);
	fgets(buf, 1024, stdin);
	return(strtok(buf, " \n"));

}

static void catch_function(int signo) {
	char buf[1024];
	int i;
    printf("\nAre you sure you want to exit? (y/n) ");
    get_first_word(buf);
    if(strcmp(buf, "y") != 0){

    	return;
    }
    printf("Do you want to save your key progress? (y/n) ");
    get_first_word(buf);
    if(strcmp(buf, "y") != 0)
    	exit(EXIT_SUCCESS);
    FILE *fp = fopen(KEY_FILE, "w");
    if(fp == NULL)
    	err(1, "Error opening file");
    for(i = 0; i < MESSAGE_LENGTH; i++){
    	if(key[i] != -1)
    		fprintf(fp, "%02X", key[i]);
    	else
    		fprintf(fp, "-1");
    }
    fclose(fp);
    exit(EXIT_SUCCESS);

}

int
hex_to_int(char c){
		 if(c == 'f' || c == 'F') return 15;
	else if(c == 'e' || c == 'E') return 14;
	else if(c == 'd' || c == 'D') return 13;
	else if(c == 'c' || c == 'C') return 12;
	else if(c == 'b' || c == 'B') return 11;
	else if(c == 'a' || c == 'A') return 10;
	else return c - '0';
}


int
hex_to_ascii(char c, char d){
	if(c == '-' && d == '1')
		return -1;
	int high = hex_to_int(c) * 16;
	int low = hex_to_int(d);
	return high+low;
}

int
sum_n_minus_one(int n){
	if(n == 1)
		return 1;
	return n + sum_n_minus_one(n-1);
}

void
print_array(int *arr){
	int i;
	fprintf(stderr, "=========================\n");
	for(i = 0; i < MESSAGE_LENGTH; i++)
		fprintf(stderr, "|%d: %d| ", i, arr[i]);
	fprintf(stderr, "\n=========================\n");
}

int
get_lines(char *file){
	int fd[2];
	char *argv[] = {"wc", "-l", MESSAGES_FILE, NULL};
	int nr, i, total;
	if(pipe(fd) < 0)
		err(1, "error pipe");
	switch(fork()){
		case -1:
			err(1, "error fork");
		case 0:
			
			close(fd[0]);
			dup2(fd[1], 1);
			execvp("wc", argv);
			err(1, "error execvp");
		default:
			nr = 1;
			total = 0;
			char buf[1024];
			close(fd[1]);
			while(nr > 0){
				nr = read(fd[0], buf + total, 1024 - total);
				total += nr;
			}
			close(fd[0]);
			if(nr < 0 )
				err(1, "error read");
			total = 0;
			for(i = 0; buf[i] != ' '; i++){
				total = total * 10 + buf[i] - '0';

			}
			return total;
	}
}

int **
get_messages(char *filename, int lines){
	int i, j;
	int **messages = malloc(sizeof(int *) * lines);
	char *msg_hex = malloc(sizeof(char) * (MESSAGE_LENGTH * 2 + 2)); 
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
		err(1, "fopen");
	for(i = 0; i < lines; i++){
		messages[i] = malloc(sizeof(int *) * MESSAGE_LENGTH);
		fgets(msg_hex, MESSAGE_LENGTH * 2 + 2, fp);
		for(j = 0; j < MESSAGE_LENGTH*2; j+=2)
			messages[i][j/2] = hex_to_ascii(msg_hex[j], msg_hex[j+1]);
	}
	return messages;
}

int
is_acceptable_value(int val){
	return (val >= 32 && val <= 33) || (val >= 44 && val <= 46)
		|| (val >= 63 && val <= 90) || (val >= 97 && val <= 122);
}

int **
get_xors(int **arrs, int lines){
	int i, j, k;
	int n = 0;
	int **result = malloc(sizeof(int *) * lines * lines);
	/*we could have less arrays but since we will need 
	  our array to look like this later*/
	for(i = 0; i < lines ; i++)
		for(j = 0 ; j < lines; j++){
			result[n] = malloc(sizeof(int) * MESSAGE_LENGTH);
			for(k = 0; k < MESSAGE_LENGTH; k++)
				result[n][k] = arrs[i][k] ^ arrs[j][k];
			n++;
		}

	return result;
}

void
get_key(char *keyfile){
	int i;
	char key_hex[MESSAGE_LENGTH*2 + 2];
	FILE *fp = fopen(keyfile, "r");
	if(fp == NULL){
		if(errno != ENOENT)
			err(1, "error fopen");
		for(i = 0; i < MESSAGE_LENGTH; i++)
			key[i] = -1;
	}
	fgets(key_hex, MESSAGE_LENGTH * 2 + 2, fp);
	for(i = 0; i < MESSAGE_LENGTH*2; i+=2)
		key[i/2] = hex_to_ascii(key_hex[i], key_hex[i+1]);
	
}

void
print_status(int **messages, int lines){
	int line, pos;
	printf("   ");
	for(line = 0; line < MESSAGE_LENGTH; line++)
		printf("%d", line % 10);
	printf("\n");
	for(line = 0; line < lines; line++){
		printf("%d: ", line);
		for(pos = 0; pos < MESSAGE_LENGTH; pos++){
			if(key[pos] == -1)
				printf("_");
			else
				printf("%c", key[pos] ^messages[line][pos]);
		}
		printf("\n");
	}
}

int
get_position(char *buf){
	printf("Which position do you want to check? ");
	get_first_word(buf);
	return (int) strtol(buf, NULL, 10);
}

int
get_most_likely_space(int lines, int pos, int **xored_messages){
	int i, j, max;
	int times[lines];
	j = 0;
	for(i = 0; i < lines; i++)
		times[i] = 0;
	for(i = 0; i < lines * lines; i++){
		if(i != 0 && i % lines == 0)
			j++;
		/*letter xor letter < 64
		  letter xor space > 64
		  space xor space < 64 but
		  is less likely*/
		if(xored_messages[i][pos] > 64)
			times[j]++;
	}
	max = times[0];
	for(i = 1; i < lines; i++){
		if(times[i] > max)
			max = times[i];
	}
	if(max == 0){
		printf("Either all the characters are spaces or none is!\n");
		return 0;
	}
	j = 0;
	for(i = 0; i < lines; i++){
		if(max == times[i]){			
			if(j == 0)
				printf("Character at line %d is the most likely to be a space ", i);
			else
				printf("and %d too ", i);
			j++;
		}
	}
	printf("\n");
	return 0;
}

int
get_line(char *buf){
	printf("In which line do you want to guess? ");
	get_first_word(buf);
	return (int) strtol(buf, NULL, 10);
}

char
get_char_guess(char *buf){
	printf("Which character do you want to guess? (If you want to guess space write space) ");
	get_first_word(buf);
	if(strcmp(buf, "space") == 0)
		return ' ';
	return buf[0];
}

int main(){
	int line, pos;
	char buf[1024];
	char ch;
	if (signal(SIGINT, catch_function) == SIG_ERR) {
        err(1, "An error occurred while setting a signal handler\n");
    }

	int lines = get_lines(MESSAGES_FILE);
	int **messages = get_messages(MESSAGES_FILE, lines);
	int **xored_messages = get_xors(messages, lines);
	get_key(KEY_FILE);

	printf("press ctrl + C anytime to exit and save progress!\n");
	while(1){
		print_status(messages, lines);
		pos = get_position(buf);
		get_most_likely_space(lines, pos, xored_messages);
		printf("Do you want to guess a character in position %d? (y/n)", pos); 
		get_first_word(buf);
		if(strcmp(buf, "y") != 0 || strcmp(buf, "yes") != 0)
			continue;
		line = get_line(buf);
		ch = get_char_guess(buf);		
		key[pos] = messages[line][pos] ^ ch;
		fprintf(stderr, "set key at pos %d to be %d (0x%02X)\n", pos, key[pos], key[pos]);
	}	
	exit(EXIT_SUCCESS);	
}