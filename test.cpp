#include <stdio.h>
#include "vector.h"
#include <ctime>
int main(){
    double t= clock();
    vector<char *> a;
    for(int i=0; i<50000000;i++){
        a.push_back("loelf");
    }
    for(int i=0; i<a.getsize();i++){
        //printf("%d: %s\n", i, a[i]);
    }
    printf("Elapsed = %lf\n", (clock() - t)/CLOCKS_PER_SEC);
    
    return 0;
}