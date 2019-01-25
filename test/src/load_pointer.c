struct ST {
  int f1;
  int f2;
};

struct ST *P = (struct ST *)0x100;

void f() {
  P[0].f1 = P[1].f1 + P[2].f2;
}

void v(long a){
	struct ST *P = (struct ST *)a;
	P[0].f1 = 0;
}

int g = 0;
void w(struct ST * P){
	g = (int)P;
	g++;
}
