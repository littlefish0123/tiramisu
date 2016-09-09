#include <isl/set.h>
#include <isl/union_map.h>
#include <isl/union_set.h>
#include <isl/ast_build.h>
#include <isl/schedule.h>
#include <isl/schedule_node.h>

#include <coli/debug.h>
#include <coli/core.h>

#include <string.h>
#include <Halide.h>


int main(int argc, char **argv)
{
	// Set default coli options.
	coli::global::set_default_coli_options();

	// Declare a function.
	coli::function fct("function0");
	coli::buffer buf0("buf0", 2, {10,10}, coli::type::primitive::uint8, NULL, true, coli::type::argument::output, &fct);

	// Declare the invariants of the function.  An invariant can be a symbolic
	// constant or a variable that does not change value during the
	// execution of the function.
	coli::invariant p0("N", coli::expr::make((int32_t) 10), &fct);

	// Declare the expressions that will be associated with the
	// computations.
	coli::expr *e1 = coli::expr::make(coli::type::op::add, coli::expr::make((uint8_t) 3), coli::expr::make((uint8_t) 4));

	// Declare the computations of the function fct.
	// To declare a computation, you need to provide:
	// (1) an isl set representing the iteration space of the computation, and
	// (2) a coli expression that represents the computation,
	// (3) the function in which the computation will be declared.
	coli::computation computation0("[N]->{S0[i,j]: 0<=i<N and 0<=j<N}", e1, &fct);

	// Map the computations to a buffer (i.e. where each computation
	// should be stored in the buffer).
	// This mapping will be updated automaticall when the schedule
	// is applied.  To disable automatic data mapping updates use
	// coli::global::set_auto_data_mapping(false).
	computation0.set_access("{S0[i,j]->buf0[i,j]}");

	// Dump the iteration domain (input) for the function.
	fct.dump_iteration_domain();

	// Set the schedule of each computation.
	// The identity schedule means that the program order is not modified
	// (i.e. no optimization is applied).
	computation0.tile(0,1,2,2);
	computation0.tag_parallel_dimension(0);

	// Add buf0 as an argument to the function.
	fct.set_arguments({&buf0});

	// Generate the time-processor domain of the computation
	// and dump it on stdout.
	fct.gen_time_processor_domain();
	fct.dump_time_processor_domain();

	// Generate an AST (abstract Syntax Tree)
	fct.gen_isl_ast();

	// Generate Halide statement for the function.
	fct.gen_halide_stmt();

	// If you want to get the generated halide statements, call
	// fct.get_halide_stmts().

	// Dump the Halide stmt generated by gen_halide_stmt()
	// for the function.
	fct.dump_halide_stmt();

	// Dump all the fields of fct.
	fct.dump(true);

	// Generate an object file from the function.
	fct.gen_halide_obj("build/generated_lib_tutorial_01.o");

	return 0;
}
