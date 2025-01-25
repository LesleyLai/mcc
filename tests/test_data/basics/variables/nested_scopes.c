// RETURN: 8
int main(void)
{
  int x = 2;
  {
    int y = x;
    x <<= y;
  }
  return x;
}
