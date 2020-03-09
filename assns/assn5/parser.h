
#ifndef PARSER_H
#define PARSER_H


long getValue(char *input);

void initOpt(options_t * opt);

void handleLeftOverArgs(int argc, char *argv[], options_t * opt);

void getArgs(int argc, char *argv[], options_t * opt);

#endif

