#include <iostream>
#include <vector>
#include <map>
#include <set>

#include "Dqbf.h"


class Parser
{
private:
    int argc;
    char* argv[];
public:
    Parser(int argc, char* argv[]);
    Dqbf* ParseDqbf();
};
