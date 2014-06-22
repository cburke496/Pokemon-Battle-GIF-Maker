#include <stdio.h>
#include "mat4.h"


Mat4 *multiply(Mat4 *a, Mat4 *b) {
  int i, j, k;
  double total = 0;
  Mat4 *result = mat4_create(mat4_columns(b)); 
  for(i = 0; i < 4; i++) {
    for(j = 0; j < b->cols; j++) {
      for(k = 0; k < 4; k++) {
	total += a->cells[k*4 + i] * b->cells[j*4 + k];
      }
      mat4_set(result, i, j, total);
      total = 0;
    }
  }


  return result;
}


//This should be used when the result of the multiplication is going to be
//stored in b. This frees the memory that was used for the old b.
Mat4 *premultiply(Mat4 *a, Mat4 *b) {
  Mat4 *result = multiply(a, b);
  
  Mat4 *deleter = b;
  mat4_delete(deleter);

  return result;
}


//This should be used when the result of the multiplication is going to be
//stored in a. This frees the memory that was used for the old a.
Mat4 *postmultiply(Mat4 *a, Mat4 *b) {
  Mat4 *result = multiply(a, b);
  
  Mat4 *deleter = a;
  mat4_delete(deleter);

  return result;
}
