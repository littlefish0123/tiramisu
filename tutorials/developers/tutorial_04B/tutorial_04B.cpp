/* 
    This tutorial shows how to write two successive matrix multiplications:
	C = A * B
	E = A * D
    We want to perform these two matrix multiplications in the same i, j loop
    (i.e., fuse them).

    for i = 0 .. N
        for j = 0 .. N
            C[i,j] = 0;
            E[i,j] = 0;
            for k = 0 .. N
                C[i,j] = C[i,j] + A[i,k] * B[k,j];
            for k = 0 .. N
                E[i,j] = E[i,j] + A[i,k] * D[k,j];
    
     To run this tutorial
     
     cd build/
     make run_developers_tutorial_04B

*/

#include <tiramisu/tiramisu.h>

#define SIZE0 1000
using namespace tiramisu;

int main(int argc, char **argv)
{
    // Set default tiramisu options.
    tiramisu::init();

    // -------------------------------------------------------
    // Layer I
    // -------------------------------------------------------

    /*
     * Declare a function matmul.
     */
    function matmul("matmul");

    constant p0("N", expr((int32_t) SIZE0), p_int32, true, NULL, 0, &matmul);

    // Declare computations that represents the input buffers (b_A and b_B)
    computation c_A("[N]->{c_A[i,j]: 0<=i<N and 0<=j<N}", expr(), false, p_uint8, &matmul);
    computation c_B("[N]->{c_B[i,j]: 0<=i<N and 0<=j<N}", expr(), false, p_uint8, &matmul);
    computation c_D("[N]->{c_D[i,j]: 0<=i<N and 0<=j<N}", expr(), false, p_uint8, &matmul);

    // Declare loop iterators
    var i("i"), j("j"), k("k"), i0("i0"), j0("j0"), i1("i1"), j1("j1");

    // Declare a computation to initialize the reduction c[i,j] and e[i,j]
    computation C_init("[N]->{C_init[i,j,-1]: 0<=i<N and 0<=j<N}", expr((uint8_t) 0), true, p_uint8, &matmul);
    computation E_init("[N]->{E_init[i,j,-1]: 0<=i<N and 0<=j<N}", expr((uint8_t) 0), true, p_uint8, &matmul);
    
    // Declare the first reduction.
    computation c_C("[N]->{c_C[i,j,k]: 0<=i<N and 0<=j<N and 0<=k<N}", expr(), true, p_uint8, &matmul);
    // Note that the previous computation has an empty expression (because we can only use c_C in an expression after its declaration)
    c_C.set_expression(c_C(i, j, k - 1) + c_A(i, k) * c_B(k, j));

    // Declare the second reduction.
    computation c_E("[N]->{c_E[i,j,k]: 0<=i<N and 0<=j<N and 0<=k<N}", expr(), true, p_uint8, &matmul);
    c_E.set_expression(c_E(i, j, k - 1) + c_A(i, k) * c_D(k, j));


    // -------------------------------------------------------
    // Layer II
    // -------------------------------------------------------

    // Tile all the computations: C_init, c_C, E_init, c_E
    // This tiles the loop levels i and j and produces the loop levels by a 32x32 tile.
    // i0, j0, i1 and j1 where i0 is the outermost loop level and j1 is the innermost.
    C_init.tile(i, j, 32, 32, i0, j0, i1, j1);
    c_C.tile(i, j, 32, 32, i0, j0, i1, j1);
    E_init.tile(i, j, 32, 32, i0, j0, i1, j1);
    c_E.tile(i, j, 32, 32, i0, j0, i1, j1);

    // Parallelize the outermost loop level i0. By parallelizing this loop all
    // the other computations are parallelized too because they all share the
    // same outer loop i0.
    c_C.parallelize(i0);

    // Specify the order between c_C, c_E, C_init and E_init.
    E_init.after(C_init, j1);
    c_C.after(E_init, j1);
    c_E.after(c_C, j1);


    // -------------------------------------------------------
    // Layer III
    // -------------------------------------------------------

    // Declare the buffers.
    buffer b_A("b_A", {expr(SIZE0), expr(SIZE0)}, p_uint8, a_input, &matmul);
    buffer b_B("b_B", {expr(SIZE0), expr(SIZE0)}, p_uint8, a_input, &matmul);
    buffer b_C("b_C", {expr(SIZE0), expr(SIZE0)}, p_uint8, a_output, &matmul);
    buffer b_D("b_D", {expr(SIZE0), expr(SIZE0)}, p_uint8, a_input, &matmul);
    buffer b_E("b_E", {expr(SIZE0), expr(SIZE0)}, p_uint8, a_output, &matmul);


    // Map the computations to a buffer.
    c_A.store_in(&b_A);
    c_B.store_in(&b_B);
    c_D.store_in(&b_D);

    // Store C_init[i,j,k] in b_C[i,j] and E_init[i,j,k] in b_E[i,j]
    C_init.store_in(&b_C, {i,j});
    E_init.store_in(&b_E, {i,j});

    // Store c_C[i,j,k] in b_C[i,j] and c_E[i,j,k] in b_E[i,j]
    c_C.store_in(&b_C, {i,j});
    c_E.store_in(&b_E, {i,j});

    // -------------------------------------------------------
    // Code Generation
    // -------------------------------------------------------

    matmul.codegen({&b_A, &b_B, &b_C, &b_D, &b_E}, "build/generated_fct_developers_tutorial_04B.o");
    
    // Dump the generated Halide statement (just for debugging).
    matmul.dump_halide_stmt();

    /** Generated code

  let N = 1000
  parallel (c1, 0, ((int32(floor_f32(float32(((N + -1)/32)))) + 1) - 0)) {
    for (c3, 0, ((int32(floor_f32(float32(((N + -1)/32)))) + 1) - 0)) {
      for (c5, 0, (min((N - (c1*32)), 32) - 0)) {
        for (c7, 0, (min((N - (c3*32)), 32) - 0)) {
          b_C[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))] = (uint8)0
          b_E[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))] = (uint8)0
          for (c9, 0, (N - 0)) {
            b_C[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))] = (b_C[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))] + (b_A[((0 + int32((int64(c9)*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))]*b_B[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(c9)*(int64)1000)))]))
          }
          for (c9, 0, (N - 0)) {
            b_E[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))] = (b_E[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))] + (b_A[((0 + int32((int64(c9)*(int64)1))) + int32((int64(((32*c1) + c5))*(int64)1000)))]*b_D[((0 + int32((int64(((32*c3) + c7))*(int64)1))) + int32((int64(c9)*(int64)1000)))]))
          }
        }
      }
    }
   }
      */

    return 0;
}
