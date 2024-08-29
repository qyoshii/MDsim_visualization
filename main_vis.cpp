#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <cfloat>
#include <time.h>

#define Np 1024
#define L 40.0
#define teq 100
#define tmax 30
#define dtmd 0.001
#define dtbd 0.01
#define temp 0.9
#define dim 2
#define cut 3.0
#define polydispersity 0.2

double unif_rand(double left, double right)
{
  return left + (right - left)*rand()/RAND_MAX;
}

double gaussian_rand(void)
{
  static double iset = 0;
  static double gset;
  double fac, rsq, v1, v2;

  if (iset == 0) {
    do {
      v1 = unif_rand(-1, 1);
      v2 = unif_rand(-1, 1);
      rsq = v1*v1 + v2*v2;
    } while (rsq >= 1.0 || rsq == 0.0);
    fac = sqrt(-2.0*log(rsq)/rsq);

    gset = v1*fac;
    iset = 0.50;
    return v2*fac;
  } else {
    iset = 0;
    return gset;
  }
}

void ini_coord_square(double (*x)[dim]){
  int num_x = (int)sqrt(Np)+1;
  int num_y = (int)sqrt(Np)+1;
  int i,j,k=0;
  for(j=0;j<num_y;j++){
    for(i=0;i<num_x;i++){
      x[i+num_x*j][0] = i*L/(double)num_x;
      x[i+num_x*j][1] = j*L/(double)num_y;
      k++;
      if(k>=Np)
        break;
    }
    if(k>=Np)
      break;
  }
}

void set_diameter(double *a){
  for(int i=0;i<Np;i++)
    a[i]=1.0+polydispersity*unif_rand(-1.,1.);
}

void p_boundary(double (*x)[dim]){
  for(int i=0;i<Np;i++)
    for(int j=0;j<dim;j++)
      x[i][j]-=L*floor(x[i][j]/L);
}

void ini_array(double (*x)[dim]){
  for(int i=0;i<Np;i++)
    for(int j=0;j<dim;j++)
      x[i][j]=0.0;
}

void calc_force(double (*x)[dim],double (*f)[dim],double *a,double *U){
  double dx,dy,dr2,dUr,w2,w6,w12,aij;
  double Ucut=1./pow(cut,12);
  ini_array(f);
  *U=0;
  for(int i=0;i<Np;i++)
    for(int j=0;j<Np;j++){
      if(i<j){
	dx=x[i][0]-x[j][0];
	dy=x[i][1]-x[j][1];
	dx-=L*floor((dx+0.5*L)/L);
	dy-=L*floor((dy+0.5*L)/L);
	dr2=dx*dx+dy*dy;
	if(dr2<cut*cut){
	  aij=0.5*(a[i]+a[j]);
	  w2=aij*aij/dr2;
	  w6=w2*w2*w2;
	  w12=w6*w6;  
	  dUr=-12.*w12/dr2;
	  f[i][0]-=dUr*dx;
	  f[j][0]+=dUr*dx;
	  f[i][1]-=dUr*dy;
	  f[j][1]+=dUr*dy;
	  *U+=w12-Ucut;
	}
      }
    }
}

void eom_langevin(double (*v)[dim],double (*x)[dim],double (*f)[dim],double *a,double *U,double dt,double temp0){
  double zeta=1.0;
  double fluc=sqrt(2.*zeta*temp0*dt);

  calc_force(x,f,a,&(*U));
  for(int i=0;i<Np;i++)
    for(int j=0;j<dim;j++){
      v[i][j]+=-zeta*v[i][j]*dt+f[i][j]*dt+fluc*gaussian_rand();
      x[i][j]+=v[i][j]*dt;
    }
  p_boundary(x);
}

void eom_md(double (*v)[dim],double (*x)[dim],double (*f)[dim],double *a,double *U,double dt){
  for(int i=0;i<Np;i++)
    for(int j=0;j<dim;j++){
      x[i][j]+=v[i][j]*dt+0.5*f[i][j]*dt*dt;
      v[i][j]+=0.5*f[i][j]*dt;
    }
  calc_force(x,f,a,&(*U));
  for(int i=0;i<Np;i++)
    for(int j=0;j<dim;j++){
      v[i][j]+=0.5*f[i][j]*dt;
    }
  p_boundary(x);
}

void output(int k,double (*x)[dim],double (*v)[dim],double *a,double U){
  char filename[128];
  double K=0.0;
  std::ofstream file;

  // ovito用
  sprintf(filename,"ovito_rep_coord_T_%.2f.dat",temp);
  file.open(filename,std::ios::app); //append
  file<< std::setprecision(6)<<"ITEM: TIMESTEP"<<std::endl;
  file<< std::setprecision(6)<<int(k*dtmd)<<std::endl;
  file<< std::setprecision(6)<<"ITEM: NUMBER OF ATOMS"<<std::endl;
  file<< std::setprecision(6)<<Np<<std::endl;
  file<< std::setprecision(6)<<"ITEM: BOX BOUNDS pp pp pp"<<std::endl;
  file<< std::setprecision(16)<<0<<"\t"<<L<<"\t"<<std::endl;
  file<< std::setprecision(16)<<0<<"\t"<<L<<"\t"<<std::endl;
  file<< std::setprecision(16)<<-1.0000000000000000e+00<<"\t"<<1.0000000000000000e+00<<"\t"<<std::endl;
  file<< std::setprecision(6)<<"ITEM: ATOMS type id x y z vx vy vz radius"<<std::endl;
  //file<< std::setprecision(6)<<k*dtmd<<"\t"<<i<<"\t"<<x[i][0]<<"\t"<<x[i][1]<<"\t"<<a[i]<<std::endl;
  file.close();

  for(int i=0;i<Np;i++){
    sprintf(filename,"ovito_rep_coord_T_%.2f.dat",temp);
    file.open(filename,std::ios::app); //append
    //file<< std::setprecision(6)<<k*dtmd<<"\t"<<i<<"\t"<<x[i][0]<<"\t"<<x[i][1]<<"\t"<<a[i]<<std::endl;
    file<< std::setprecision(6)<<1<<"\t"<<(i+1)<<"\t"<<x[i][0]<<"\t"<<x[i][1]<<"\t"<<0<<"\t"<<v[i][0]<<"\t"<<v[i][1]<<"\t"<<0<<"\t"<<a[i]<<std::endl;
    file.close();
  }

  for(int i=0;i<Np;i++){
    sprintf(filename,"rep_coord_T_%.2f.dat",temp);
    file.open(filename,std::ios::app); //append
    file<< std::setprecision(6)<<k*dtmd<<"\t"<<i<<"\t"<<x[i][0]<<"\t"<<x[i][1]<<"\t"<<a[i]<<std::endl;
    file.close();
  }

  sprintf(filename,"energy_T_%.2f.dat",temp);
  file.open(filename,std::ios::app); //append
  for(int i=0;i<Np;i++)
    for(int j=0;j<dim;j++)
      K+=0.5*v[i][j]*v[i][j];
  
  std::cout<< std::setprecision(6)<<k*dtmd<<"\t"<<K/Np<<"\t"<<U/Np<<"\t"<<(K+U)/Np<<std::endl;  
  file<< std::setprecision(6)<<k*dtmd<<"\t"<<K/Np<<"\t"<<U/Np<<"\t"<<(K+U)/Np<<std::endl;
  file.close();
}

int main(){
  double x[Np][dim],v[Np][dim],f[Np][dim],a[Np];
  double tout=0.0,U;
  int j=0;
  set_diameter(a);
  ini_coord_square(x);
  ini_array(v);
  
  while(j*dtbd < 10.){
    j++;
    eom_langevin(v,x,f,a,&U,dtbd,5.0);
  }
  
  j=0;
  while(j*dtbd < teq){
    j++;
    eom_langevin(v,x,f,a,&U,dtbd,temp);
  }
  j=0;
  while(j*dtmd < tmax){
    j++;
    eom_md(v,x,f,a,&U,dtmd);
    if(j*dtmd >= tout){
      output(j,x,v,a,U);
      tout+=1.;
    }
  }
  
  return 0;
}