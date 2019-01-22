int b = 0;

void f(){
	*(int*)0x100 = 1;
	int a = *(int*)0x100;
	b = *(int*)0x100;
}