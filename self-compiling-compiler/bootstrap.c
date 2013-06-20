#include <stdio.h>
#include <stdlib.h>

/* expected characters:
     LF ... SPACE ... # ... 0-9 ... a-f
   2 characters define a byte.
*/
int convert(int acc, int comment) {
  int byte = getchar();

  /* parse a character */
  if (EOF == byte) { exit(0); }
  if (';' == byte) { return convert(acc, 1); }
  if ('\n' == byte) { return convert(acc, 0); }
  if (1 == comment) { return convert(acc, 1); }
  if (' ' == byte || '\t' == byte) { return convert(acc, 0); }

  /* convert character to nibble */
  byte -= (byte < 'a') ? '0' : 'a' - 10;

  /* read 2nd nibble */
  if (EOF == acc) { return convert(byte << 4, 0); } 
  putchar(acc + byte);

  /* read next byte */
  return convert(EOF, 0);
}

int main(void) {
  return convert(EOF, 0);
}
