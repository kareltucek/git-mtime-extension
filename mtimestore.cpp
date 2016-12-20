#include <time.h>
#include <utime.h>
#include <sys/stat.h>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <string>
#include <cerrno>
#include <cstring>
#include <sys/types.h>
#include <ctime>
#include <map>
#include <ctype>
#include <stdio>

#ifdef __BORLANDC__
#define popen _popen
#endif


void changedate(int time, const char* filename)
{
  try
  {
    struct utimbuf new_times;
    struct stat foo;
    stat(filename, &foo);

    new_times.actime = foo.st_atime;
    new_times.modtime = time;
    utime(filename, &new_times);
  }
  catch(...)
  {}
}

bool parsenum(int& num, char*& ptr)
{
  num = 0;
  if(!isdigit(*ptr))
    return false;
  while(isdigit(*ptr))
  {
    num = num*10 + (int)(*ptr) - 48;
    ptr++;
  }
  return true;
}

//splits the 'line' string into a numeric part(->time) and sets ptr to the beginning of the filename
bool parseline(const char* line, int& time, char*& ptr)
{
  if(*line == '\n' || *line == '\r')
    return false;
  time = 0;
  ptr = (char*)line;
  if( parsenum(time, ptr))
  { 
    ptr++;
    return true;
  }
  else
    return false;
}

//get rid of endlines (otherwise these are interpretted as a part of the filename)
void trim(char* string)
{
  char* ptr = string;
  while(*ptr != '\0')
  {
    if(*ptr == '\n' || *ptr == '\r')
      *ptr = '\0';
    ptr++;
  }
}


void help()
{
  std::cout << "version: 1.6" << std::endl;
  std::cout << "usage: mtimestore <switch>" << std::endl;
  std::cout << "options:" << std::endl;
  std::cout << "  -a  saves mtimes of all git-versed files into .mtimes file (meant to be done on intialization of mtime fixes)" << std::endl;
  std::cout << "  -s  saves mtimes of modified staged files into .mtimes file(meant to be put into pre-commit hook)" << std::endl;
  std::cout << "  -r  restores mtimes from .mtimes file (that is meant to be stored in repository server-side and to be called in post-checkout hook)" << std::endl;
  std::cout << "  -h  show this help" << std::endl;
}

void load_file(const char* file, std::map<std::string,int>& mapa)
{

  std::string line;
  std::ifstream myfile (file, std::ifstream::in);

  if(myfile.is_open())
  {
      while ( myfile.good() )
      {
        getline (myfile,line);
        int time;
        char* ptr;
        if( parseline(line.c_str(), time, ptr))
        {
          if(std::string(ptr) != std::string(".mtimes"))
            mapa[std::string(ptr)] = time;
        }
      }
    myfile.close();
  }

}

void update(std::map<std::string, int>& mapa, bool all)
{
  char path[2048];
  FILE *fp;
  if(all)
    fp = popen("git ls-files", "r");
  else
    fp = popen("git diff --name-only --staged", "r");

  while(fgets(path, 2048, fp) != NULL)
  {
    trim(path);
    struct stat foo;
    int err = stat(path, &foo);
    if(std::string(path) != std::string(".mtimes"))
      mapa[std::string(path)]=foo.st_mtime;
  }
}

void write(const char * file, std::map<std::string, int>& mapa)
{
  std::ofstream outputfile;
  outputfile.open(".mtimes", std::ios::out);
  for(std::map<std::string, int>::iterator itr = mapa.begin(); itr != mapa.end(); ++itr)
  {
    if(*(itr->first.c_str()) != '\0')
    {
      outputfile << itr->second << "|" << itr->first << std::endl;   
    }
  }
  outputfile.close();
}

int main(int argc, char *argv[])
{
  //parse options in unix-like manner
  if(argc >= 2 && argv[1][0] == '-')
  {
    switch(argv[1][1])
    {
      case 'r':
        {
          std::cout << "restoring modification dates" << std::endl;
          std::string line;
          std::ifstream myfile (".mtimes");
          if (myfile.is_open())
          {
            while ( myfile.good() )
            {
              getline (myfile,line);
              int time, time2;
              char* ptr;
              parseline(line.c_str(), time, ptr);
              changedate(time, ptr);
            }
            myfile.close();
          }
        }
        break;
      case 'a':
      case 's':
        {
          std::cout << "saving modification times" << std::endl;

          std::map<std::string, int> mapa;
          load_file(".mtimes", mapa); 
          update(mapa, argv[1][1] == 'a');
          write(".mtimes", mapa);
        }
        break;
      default:
        help();
        return 0;
    }
  }
  else
  {
    help();
    return 0;
  }

  return 0;
}


