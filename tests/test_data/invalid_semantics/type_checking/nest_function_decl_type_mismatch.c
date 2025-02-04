int main(void)
{
  int foo(void);

  return foo();
}

int foo(int x)
{
  return x + 42;
}
