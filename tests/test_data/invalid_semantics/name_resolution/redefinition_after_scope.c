int main(void)
{
  int x = 42;
  {
    x = 42;
  }
  int x = 2;
  return x;
}
