// RETURN: 2
int main(void)
{
  int x = 0;
  {
    int y = 4;
    x = y;
  }
  {
    int y = 2;
    x -= y;
  }
  return x;
}
