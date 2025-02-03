// RETURN: 28
// On system V ABI, x7 will be passed on stack
int f(int x1, int x2, int x3, int x4, int x5, int x6, int x7)
{
  return x1 + x2 + x3 + x4 + x5 + x6 + x7;
}

int main(void)
{
  return f(1, 2, 3, 4, 5, 6, 7);
}
