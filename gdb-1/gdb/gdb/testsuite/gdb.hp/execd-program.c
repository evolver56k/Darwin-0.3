#include <stdio.h>

/* There is a global_i in follow_exec, which exec's us.  We
   should not be able to see that other definition of global_i
   after we are exec'd.
   */
int  global_i = 0;

main (argc, argv)
  int  argc;
  char *  argv[];
{
  /* There is a local_j in follow_exec, which exec's us.  We
     should not be able to see that other definition of local_j
     after we are exec'd.
     */
  int  local_j = argc;
  char *  s;

  printf ("Hello from execd_program...\n");
  if (argc != 2)
    {
      printf ("expected one string argument\n");
      exit (-1);
    }
  s = argv[1];
  printf ("argument received: %s\n", s);
}
