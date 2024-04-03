#include <unistd.h>
#include <stdio.h>

void touch(const char* fname)
{
  FILE* fh = fopen(fname,"w");
  if( fh )
    fclose(fh);
  else
    fprintf(stderr,"Unable to create file \"%s\".\n",fname);
}


int main(int argc,char** argv)
{
  char s_start[1024];
  s_start[1023] = 0;
  snprintf(s_start, 1023, "test_proc%s_started", (argc>1)?(argv[1]):(""));
  char s_end[1024];
  s_end[1023] = 0;
  snprintf(s_end, 1023, "test_proc%s_ended", (argc>1)?(argv[1]):(""));
  remove(s_start);
  remove(s_end);
  touch(s_start);
  sleep(3);
  touch(s_end);
  return 0;
}

// Local Variables:
// compile-command: "make -C ../ testbin"
// End:
