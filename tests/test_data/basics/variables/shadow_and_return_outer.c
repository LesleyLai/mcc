// RETURN: 2
int main(void)
{
  int x = 2;
  {
    int x = 1;
  }
  return x;
}
