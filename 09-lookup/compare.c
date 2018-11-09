#include <stdio.h>
#include <stdlib.h>

int main(){
	FILE *f1 = fopen("1.txt", "r");
	FILE *f2 = fopen("3.txt", "r");

	char c1 = fgetc(f1);
	char c2 = fgetc(f2);
	while(!feof(f1) && !feof(f2)){
		if(c1 != c2){
			printf("NO\n");
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
