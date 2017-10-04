#include "stdio.h"
#include "time.h"
long Akm(int m,int n)  
{
    long a[m+1][n+1];
    int i,j;  
    for(j=0;j<n+1;j++)  
        a[0][j]=j+1;
    for(i=1;i<=m;i++)  
    {  
        a[i][0]=a[i-1][1];  
        for(j=1;j<n+1;j++)  
            a[i][j]=a[i-1][a[i][j-1]];  
    }  
    return a[m][n];  
}  

int main()
{
    printf("caculate Akm\n");
    printf("input loop nums\n");
    int loopnum;
    scanf("%d", &loopnum);
    printf("input m and n\n");
    int m, n;
    scanf("%d %d", &m, &n);

    double totaltime = 0.0;
    


    int _;

    for(_ = 0; _ < loopnum; _++){
        double startsec = time(0);
        Akm(m, n);
        double endsec = time(0);
        totaltime += endsec - startsec;
    }


    if (totaltime <= 0) {
        printf("Insufficient duration- Increase the LOOP count\n");
        return -1;
    }

    printf("%lf\n", totaltime);
    //KIPS = (100.0*loopnum)/totaltime;
    return 0;
}