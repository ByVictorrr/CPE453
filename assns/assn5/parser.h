
#ifndef PARSER_H
#define PARSER_H

/* In Main function */
void usage(int doExit, char *argv[]);
void help(char *argv[]);
void handleLeftOverArgs(int argc, char *argv[], options_t * opt);

/* In Parser */
//long getValue(char *input);
void initOpt(options_t * opt);
void getArgs(int argc, char *argv[], options_t * opt);

#endif

