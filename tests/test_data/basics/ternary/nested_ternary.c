// RETURN: 0
int main(void)
{
  int year = 1900;
  return (year % 100 == 0) ? ((year % 400 == 0) ? 1 : 0)
                           : ((year % 4 == 0) ? 1 : 0);
}
