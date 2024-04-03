#include <unistd.h>
#include <stdio.h>

void touch(const char* fname)
{
  FILE* fh = fopen(fname,"w");
  if( fh )
    fclose(fh);
  else
    fprintf(stderr,"Unable to create file \"%s\".",fname);
}


int main(int,char**)
{
  remove("test_proc_started");
  remove("test_proc_ended");
  touch("test_proc_started");
  sleep(3);
  touch("test_proc_ended");
  return 0;
}

// Local Variables:
// compile-command: "make -C ../ testbin"
// End:
