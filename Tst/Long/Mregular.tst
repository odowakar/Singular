LIB "tst.lib";
LIB "Mregular.lib";
ring r=0,(x,y,z,t),dp; 
ideal j=x17y14-y31, x20y13, x60-y36z24-x20z20t20;
int l=reg_CM(j);l;
l-1;
ring rr=0,(t,x,y,z),dp; 
ideal j=imap(r,j);
reg_CM(j);
ring r1=0,(x,y,z,t),dp;
ideal i=y4-t3z, x3t-y2z2, x3y2-t2z3, x6-tz5;
reg_CM(i); 
ring r2=0,(x,y,z,t,w),dp;
ideal i=xy-zw,x3-yw2,x2z-y2w,y3-xz2,-y2z3+xw4+tw4+w5,-yz4+x2w3+xtw3+xw4,
  -z5+x2tw2+x2w3+yw4;
reg_CM(i); 
ring r3=0,(x,y,z,t,w,u),dp;
ideal i=imap(r2,i);
reg_CM(i);
ring r4=0,(a,b,c,d,x(0..9)),dp;
ideal i= x(0)-ab,x(1)-ac,x(2)-ad,x(3)-bc,x(4)-bd,x(5)-cd,
         x(6)-a2,x(7)-b2,x(8)-c2,x(9)-d2;
ideal ei=eliminate(i,abcd);
ring r5=0,x(0..9),dp;
ideal i=imap(r4,ei);
reg_CM(i);
ideal mi=intersect(i,maxideal(3));
size(mi);
reg_CM(mi);
ring s = 0,(x,y,z,t),dp;
ideal i  = x17y14-y31, x20y13, x60-y36z24-x20z20t20;
reg_curve(i);
int k=43;
ideal j=x17y14-y31,x20y13,x60-y36z24-x20z20t20,y41*z^k-y40*z^(k+1);
reg_curve(j);
k=14;
j=x17y14-y31,x20y13,x60-y36z24-x20z20t20,y41*z^k-y40*z^(k+1);  
reg_curve(j);  
k=22;
j=x17y14-y31,x20y13,x60-y36z24-x20z20t20,y41*z^k-y40*z^(k+1);  
reg_curve(j);  
k=315;
j=x17y14-y31,x20y13,x60-y36z24-x20z20t20,y41*z^k-y40*z^(k+1);  
reg_curve(j);  
ring s1=0,(a,b,c,d,x(0..9)),dp;
ideal i= x(0)-ab,x(1)-ac,x(2)-ad,x(3)-bc,x(4)-bd,x(5)-cd,
         x(6)-a2,x(7)-b2,x(8)-c2,x(9)-d2;
ideal ei=eliminate(i,abcd);
ring s2=0,x(0..9),dp;
ideal i=imap(s1,ei);
reg_curve(i);
ring s3=0,(t,x,y,z),dp;
ideal j=imap(s,j);
reg_curve(j);
setring s;
ideal h=x2-3xy+5xt,xy-3y2+5yt,xz-3yz,2xt-yt,y2-yz-2yt;
reg_curve(h);
reg_curve(lead(std(h)));
ring s4=0,(x,y,z,t,u,a,b),dp;
ideal i=u-b40,t-a40,x-a23b17,y-a22b18+ab39,z-a25b15;
ideal ei=eliminate(i,ab); 
ring s5=0,(x,y,z,t,u),dp;
ideal i=imap(s4,ei);
reg_curve(i);  
ring s6=0,(x(0..8),s,t),dp;
ideal i=x(0)-st24,x(1)-s2t23,x(2)-s3t22,x(3)-s9t16,x(4)-s11t14,x(5)-s18t7,
        x(6)-s24t,x(7)-t25,x(8)-s25;
ideal ei=eliminate(i,st);
ring s7=0,x(0..8),dp;
ideal i=imap(s6,ei);
reg_curve(i);
reg_moncurve(0,1,2,3);
reg_moncurve(0,1,3,5,6);
reg_moncurve(1,4,6,9);
reg_moncurve(0,3,8,5,23);
reg_moncurve(0,4,7,7,9);
reg_moncurve(0,2,12,15);
reg_moncurve(0,5,9,11,20);
reg_moncurve(0,1,2,3,9,11,18,24,25); 
reg_moncurve(0,1,2,7,16,17,25,27,28,30,36,37);
ring q=0,(s,t,x(0..11)),dp;
ideal i=x(0)-st36,x(1)-s2t35,x(2)-s7t30,x(3)-s16t21,x(4)-s17t20,x(5)-s25t12,
        x(6)-s27t10,x(7)-s28t9,x(8)-s30t7,x(9)-s36t,x(10)-s37,x(11)-t37;
ideal ei=eliminate(i,st);
ring qq=0,x(0..11),dp;
ideal i=imap(q,ei);
reg_curve(i,1);
reg_moncurve(0,1,2,7,16,17,25,27,28,30,36,37,40,53,55);
tst_status(1);$
