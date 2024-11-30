/*
	$Revision: 1.8 $ $Date: 2022/11/02 10:18:16 $

	R interface

	Pass data between R and internally-defined data structures 


# /////////////////////////////////////////////
# AUTHOR: Adrian Baddeley, CWI, Amsterdam, 1991.
# 
# MODIFIED BY: Adrian Baddeley, Perth 2009, 2022
# 
# This software is distributed free
# under the conditions that
# 	(1) it shall not be incorporated
# 	in software that is subsequently sold
# 	(2) the authorship of the software shall
# 	be acknowledged in any publication that 
# 	uses results generated by the software
# 	(3) this notice shall remain in place
# 	in each file.
# //////////////////////////////////////////////


*/

#include <R.h>
#include "geom3.h"
#include "functable.h"

#undef DEBUG 

#ifdef DEBUG
#define DEBUGMESSAGE(S) Rprintf(S);
#else 
#define DEBUGMESSAGE(S) 
#endif

void g3one(Point *p, int n, Box *b, Ftable *g);
void g3three(Point *p, int n, Box *b, Ftable *g);
void g3cen(Point *p, int n, Box *b, H4table *count);
void k3trans(Point *p, int n, Box *b, Ftable *k);
void k3isot(Point *p, int n, Box *b, Ftable *k);
void pcf3trans(Point *p, int n, Box *b, Ftable *pcf, double delta);
void pcf3isot(Point *p, int n, Box *b, Ftable *pcf, double delta);
void phatminus(Point *p, int n, Box *b, double vside, Itable *count);
void phatnaive(Point *p, int n, Box *b, double vside, Itable *count);
void p3hat4(Point *p, int n, Box *b, double vside, H4table *count);

/*
	ALLOCATION OF SPACE FOR STRUCTURES/ARRAYS

	We have defined an alloc() and free() function for each type.

	However, the free() functions currently do nothing,
	because we use R_alloc to allocate transient space,
	which is freed automatically by R.

*/

Ftable *
allocFtable(int n)		/* allocate function table of size n */
{
  Ftable *x;
  x = (Ftable *) R_alloc(1, sizeof(Ftable));
  x->n = n;
  x->f 	   = (double *) R_alloc(n, sizeof(double));
  x->num   = (double *) R_alloc(n, sizeof(double));
  x->denom = (double *) R_alloc(n, sizeof(double));
  return(x);
}

void freeFtable(Ftable *x) { }

Itable	*
allocItable(int n)
{
  Itable *x;
  x = (Itable *) R_alloc(1, sizeof(Itable));
  x->n     = n;
  x->num   = (int *) R_alloc(n, sizeof(int));
  x->denom = (int *) R_alloc(n, sizeof(int));
  return(x);
}

void freeItable(Itable *x) { }

H4table	*
allocH4table(int n)
{
  H4table *x;
  x = (H4table *) R_alloc(1, sizeof(H4table));
  x->n     = n;
  x->obs   = (int *) R_alloc(n, sizeof(int));
  x->nco   = (int *) R_alloc(n, sizeof(int));
  x->cen   = (int *) R_alloc(n, sizeof(int));
  x->ncc   = (int *) R_alloc(n, sizeof(int));
  return(x);
}

void freeH4table(H4table *x) { }

Box	*
allocBox(void)		/* I know this is ridiculous but it's consistent. */
{
  Box *b;
  b = (Box *) R_alloc(1, sizeof(Box));
  return(b);
}

void freeBox(Box *x) { }


Point	*
allocParray(int n)		/* allocate array of n Points */
{
  Point *p;
  p = (Point *) R_alloc(n, sizeof(Point));
  return(p);
}

void freeParray(Point *x) { }

/*
	CREATE AND INITIALISE DATA STORAGE

*/

Ftable *
MakeFtable(double *t0, double *t1, int *n)
{
  Ftable	*tab;
  int	i, nn;

  nn = *n;
  tab = allocFtable(nn);

  tab->t0 = *t0;
  tab->t1 = *t1;
  
  for(i = 0; i < nn; i++) {
    tab->f[i] = 0.0;
    tab->num[i] = 0;
    tab->denom[i] = 0;
  }
  return(tab);
}
	
Itable	*
MakeItable(double *t0, double *t1, int *n)
{
  Itable *tab;
  int i, nn;

  nn = *n;
  tab = allocItable(nn);

  tab->t0 = *t0;
  tab->t1 = *t1;

  for(i = 0; i < nn; i++) {
    tab->num[i] = 0;
    tab->denom[i] = 0;
  }
  return(tab);
}

H4table	*
MakeH4table(double *t0, double *t1, int *n)
{
  H4table *tab;
  int i, nn;

  nn = *n;
  tab = allocH4table(nn);

  tab->t0 = *t0;
  tab->t1 = *t1;

  for(i = 0; i < nn; i++) {
    tab->obs[i] = 0;
    tab->nco[i] = 0;
    tab->cen[i] = 0;
    tab->ncc[i] = 0;
  }
  tab->upperobs = 0;
  tab->uppercen = 0;

  return(tab);
}

/*
	CONVERSION OF DATA TYPES 

		R -> internal

	including allocation of internal data types as needed
*/

Point	*
RtoPointarray(double *x, double *y, double *z, int *n)
{
  int	i, nn;
  Point	*p;

  nn = *n;
  p = allocParray(nn);
	
  for(i = 0; i < nn; i++) {
    p[i].x = x[i];
    p[i].y = y[i];
    p[i].z = z[i];
  }
  return(p);
}

Box *
RtoBox(double *x0, double *x1, double *y0, double *y1, double *z0, double *z1)
{
  Box *b;
  b = allocBox();

  b->x0 = *x0;
  b->x1 = *x1;
  b->y0 = *y0;
  b->y1 = *y1;
  b->z0 = *z0;
  b->z1 = *z1;
  return(b);
}

/*
	CONVERSION OF DATA TYPES 

		internal -> R

	Note: it can generally be assumed that the R arguments
	are already allocated vectors of correct length,
	so we do not allocate them.


*/

void
FtabletoR(
  /* internal */
  Ftable	*tab,
  /* R representation */
  double       *t0,
  double       *t1,
  int	       *n,
  double	*f,
  double      *num,
  double     *denom
)  {
  int	i;

  *t0 = tab->t0;
  *t1 = tab->t1;
  *n = tab->n;
	
  for(i = 0; i < tab->n; i++) {
    f[i] = tab->f[i];
    num[i] = tab->num[i];
    denom[i] = tab->denom[i];
  }

  freeFtable(tab);
}

void
ItabletoR(
     /* internal */
  Itable *tab,
     /* R representation */
  double *t0,
  double *t1,
  int  *m,
  int  *num,
  int *denom
) {
  int	i;
  
  *t0 = tab->t0;
  *t1 = tab->t1;
  *m  = tab->n;

  for(i = 0; i < tab->n; i++) {
    num[i] = tab->num[i];
    denom[i] = tab->denom[i];
  }
  freeItable(tab);
}
	
void
H4tabletoR(
  /* internal */
  H4table	*tab,
  /* R representation */
  double *t0,
  double *t1,
  int *m,
  int *obs,
  int *nco,
  int *cen,
  int *ncc,
  int *upperobs,
  int *uppercen
) {
  int	i;
  
  *t0 = tab->t0;
  *t1 = tab->t1;
  *m  = tab->n;

  *upperobs = tab->upperobs;
  *uppercen = tab->uppercen;

  for(i = 0; i < tab->n; i++) {
    obs[i] = tab->obs[i];
    nco[i] = tab->nco[i];
    cen[i] = tab->cen[i];
    ncc[i] = tab->ncc[i];
  }

  freeH4table(tab);
}
	
		
/*
	R CALLING INTERFACE 

	These routines are called from R by 
	> .C("routine-name", ....)
*/

void
RcallK3(
  /* points */	
  double *x,
  double *y,
  double *z,	
  int    *n,
  /* box */
  double *x0,
  double *x1,
  double *y0,
  double *y1, 
  double *z0,
  double *z1,
  /* Ftable */
  double *t0,
  double *t1,
  int    *m,
  double *f,
  double *num,
  double *denom,
  /* edge correction */
  int    *method
) {
  Point	*p;
  Box 	*b;
  Ftable	*tab;
	
  p = RtoPointarray(x, y, z, n);
  b = RtoBox(x0, x1, y0, y1, z0, z1);
  tab = MakeFtable(t0, t1, m);	

  switch((int) *method) {	
  case 0:
    k3trans(p, (int) *n, b, tab); break;
  case 1:
    k3isot(p, (int) *n, b, tab); break;
  default:
    Rprintf("Method %d not implemented: defaults to 0\n", *method);
    k3trans(p, (int) *n, b, tab); break;
  }
  FtabletoR(tab, t0, t1, m, f, num, denom);
}

void
RcallG3(
  /* points */
  double *x,
  double *y,
  double *z,
  int    *n,
  /* box */
  double *x0,
  double *x1,
  double *y0,
  double *y1, 
  double *z0,
  double *z1,
  /* Ftable */
  double *t0,
  double *t1,	/* Ftable */
  int    *m,
  double *f,
  double *num,
  double *denom,
  /* edge correction */
  int    *method
) {
  Point	*p;
  Box 	*b;
  Ftable	*tab;
	
  p = RtoPointarray(x, y, z, n);
  b = RtoBox(x0, x1, y0, y1, z0, z1);
  tab = MakeFtable(t0, t1, m);	

  switch(*method) {
  case 1:
    g3one(p, (int) *n, b, tab); 
    break;
  case 3:
    g3three(p, (int) *n, b, tab); 
    break;
  default:
    Rprintf("Method %d not implemented: defaults to 3\n", *method);
    g3three(p, (int) *n, b, tab); 
  }
  FtabletoR(tab, t0, t1, m, f, num, denom);
}

void
RcallG3cen(
  /* points */     
  double *x,
  double *y,
  double *z,
  int    *n,
  /* box */
  double *x0,
  double *x1,
  double *y0,
  double *y1, 
  double *z0,
  double *z1,
  /* H4table */
  double *t0,
  double *t1,
  int    *m,
  int    *obs,
  int    *nco,
  int    *cen,
  int    *ncc,
  int    *upperobs,
  int    *uppercen
) {
  Point	*p;
  Box 	*b;
  H4table *count;
	
  DEBUGMESSAGE("Inside RcallG3cen\n")
  p = RtoPointarray(x, y, z, n);
  b = RtoBox(x0, x1, y0, y1, z0, z1);
  count = MakeH4table(t0, t1, m);
  g3cen(p, (int) *n, b, count);
  H4tabletoR(count, t0, t1, m, obs, nco, cen, ncc, upperobs, uppercen);
  DEBUGMESSAGE("Leaving RcallG3cen\n")
}

void
RcallF3(
  /* points */
  double *x,
  double *y,
  double *z,	
  int    *n,
  /* box */
  double *x0,
  double *x1,
  double *y0,
  double *y1, 
  double *z0,
  double *z1,
  /* voxel size */
  double *vside,
  /* Itable */
  double *t0,
  double *t1,
  int    *m,		
  int    *num,
  int    *denom,
  /* edge correction */
  int    *method
) {
  Point	*p;
  Box 	*b;
  Itable *count;
	
  DEBUGMESSAGE("Inside Rcall_f3\n")
  p = RtoPointarray(x, y, z, n);
  b = RtoBox(x0, x1, y0, y1, z0, z1);
  count = MakeItable(t0, t1, m);	

  switch((int) *method) {
  case 0:
    phatnaive(p, (int) *n, b, *vside, count);
    break;
  case 1:
    phatminus(p, (int) *n, b, *vside, count);
    break;
  default:
    Rprintf("Method %d not recognised: defaults to 1\n", *method);
    phatminus(p, (int) *n, b, *vside, count);
  }

  ItabletoR(count, t0, t1, m, num, denom);
  DEBUGMESSAGE("Leaving Rcall_f3\n")
}

void
RcallF3cen(
  /* points */     
  double *x,
  double *y,
  double *z,
  int    *n,
  /* box */
  double *x0,
  double *x1, 	
  double *y0,
  double *y1, 
  double *z0,
  double *z1,
  /* voxel size */
  double *vside,
  /* H4table */
  double *t0,
  double *t1,
  int    *m,	
  int    *obs,
  int    *nco,
  int    *cen,
  int    *ncc,
  int    *upperobs,
  int    *uppercen
) {
  Point	*p;
  Box 	*b;
  H4table *count;
	
  DEBUGMESSAGE("Inside Rcallf3cen\n")
  p = RtoPointarray(x, y, z, n);
  b = RtoBox(x0, x1, y0, y1, z0, z1);
  count = MakeH4table(t0, t1, m);
  p3hat4(p, (int) *n, b, *vside, count);
  H4tabletoR(count, t0, t1, m, obs, nco, cen, ncc, upperobs, uppercen);
  DEBUGMESSAGE("Leaving Rcallf3cen\n")
}

void
Rcallpcf3(
  /* points */
  double *x,
  double *y,
  double *z,
  int    *n,
  /* box */
  double *x0,
  double *x1,
  double *y0,
  double *y1, 
  double *z0,
  double *z1,
  /* Ftable */
  double *t0,
  double *t1,
  int    *m,
  double *f,
  double *num,
  double *denom,
  /* edge correction */
  int    *method,
  /* Epanechnikov kernel halfwidth */
  double *delta
) {
  Point	*p;
  Box 	*b;
  Ftable	*tab;

  p = RtoPointarray(x, y, z, n);
  b = RtoBox(x0, x1, y0, y1, z0, z1);
  tab = MakeFtable(t0, t1, m);	

  switch((int) *method) {	
  case 0:
    pcf3trans(p, (int) *n, b, tab, (double) *delta); break;
  case 1:
    pcf3isot(p, (int) *n, b, tab, (double) *delta); break;
  default:
    Rprintf("Method %d not implemented: defaults to 0\n", *method);
    pcf3trans(p, (int) *n, b, tab, (double) *delta); break;
  }
  FtabletoR(tab, t0, t1, m, f, num, denom);
}

