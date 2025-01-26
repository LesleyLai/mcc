// RETURN: 226
int main(void)
{
  int x = 1024;
  do
    ;
  while ((x = x - 42) >= 256);

  return x;
}
