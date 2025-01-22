// RETURN: 1
int main(void)
{
  int a = 0;
  int b = 1;
  if (a) b = 2;
  else if (!b) {
    b = 3;
  }
  return b;
}
