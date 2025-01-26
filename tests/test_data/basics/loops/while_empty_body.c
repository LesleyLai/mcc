// RETURN: 32
int main(void)
{
  int x = 200;
  while ((x -= 42) > 42) {}
  return x;
}
