#include <stdio.h>
#include <stdlib.h>

int main(){
	char *name1 = "1.txt";
	char *name2 = "11.txt";
	FILE *f1 = fopen(name1, "r");
	FILE *f2 = fopen(name2, "r");
	printf("compare %s and %s\n", name1, name2);

	char c1 = fgetc(f1);
	char c2 = fgetc(f2);
	while(!feof(f1) && !feof(f2)){
		if(c1 != c2){
			system("pause");
			return 0;
		}
		c1 = fgetc(f1);
		c2 = fgetc(f2);
	}
	if(c1 == EOF && c2 == EOF){
		printf("YES\n");
	} else {
		printf("NO\n");
	}
	fclose(f1);
	fclose(f2);
}
