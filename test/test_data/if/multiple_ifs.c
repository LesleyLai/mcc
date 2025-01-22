// RETURN: 7
int main(void)
{
  int a = 0;
  int b = 0;

  if (a) {
    a = 2;
  } else {
    a = 3;
  }

  if (b == 0) {
    b = 4;
  } else {
    b = 5;
  }

  return a + b;
}
