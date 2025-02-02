// RETURN: 6
int a(void)
{
  return 1;
}

int b(int a, int b)
{
  return a + b;
}

int main(void)
{
  return a() + b(2, 3);
}
