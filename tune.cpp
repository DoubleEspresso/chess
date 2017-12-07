#include <cstring>
#include <iostream>

#include "tune.h"

void parse_args(int argc, char ** argv);


int main(int argc, char ** argv)
{
  printf("..hello from tune\n");
}


void parse_args(int argc, char* argv[])
{
 for (int j = 0; j<argc; ++j)
   {
     if (!strcmp(argv[j], "-test")) {}
   }
}
