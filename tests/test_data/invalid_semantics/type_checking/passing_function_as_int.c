int f(int x, int y)
{
  return x + y;
}

int main(void)
{
  return f(42, f);
}
