int main(void)
{
   char num[] = {0x12, 0x34, 0x12, 0x34, 0x56, 0x78, 0x56, 0x78};
   int res = 0x34123456;
   int val = 0;
   memcpy(&val, num + 1, sizeof(int));
   printf("val = %d\n", val);
   printf("res = %d\n", res);
   if (val == res)
	printf("Test successful\n");
   else
	printf("Test failed\n");
   return 0;
}

