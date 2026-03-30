#ifndef PARSER_H
#define PARSER_H


#include <iostream>
#include <vector>
#include <map>
#include <set>
#include<fstream>
#include "Dqbf.h"


class Parser
{
private:
public:
    int argc;
    char** argv;
    Parser(int argc, char** argv);
    Dqbf* ParseDqbf();
    
};

#endif // PARSER_H
