# Rust vs C++ loop vectorization

This is a tiny experiment that demonstrates a behavior I found when working on a simple matrix multiplication loop in Rust. As I was investigating a performance issue, I found that letting the stride be a runtime constant created a large performance penalty when compared to specializing the loop to row major (i.e. column stride being known to be 1 at compile time). I also include a similar experiment but in C++ (via g++). 

When the runtime stride is known to be 1, the compiler can emit SIMD instructions, increasing performance. The performance penalty is not observed in the C++ version of the experiment, seemingly due to the compiler performing loop versioning for the stride = 1 case in the runtime version. 

This demonstrates an interesting code-generation difference between the compiler configurations used in this experiment. It shouldn't necessarily be interpreted as a general performance difference between Rust and C++.

## Code

The code is designed to be simple and minimal. It performs some warm up runs and then times the runtime and compile time stride. One can make it more precise by taking the median, making the order vary, and sampling more runs. It accepts a runtime argument of the stride but it only really makes sense when it is 1. This is to mirror more closely a real world behavior. One could also use a blackbox (see comments in the code). The code is not really meant to be run outside the shell script that performs the experiment. 

## Running It

From this directory
```bash
bash scripts/asm_rust.sh
bash scripts/asm_cpp.sh
```

It will build, run and generate the assembly. It looks for SIMD instructions in the assembly of the `runtime` vs `comptime` functions.

## Assembly

[the exact assembly will depend on the machine, this is what I obtained]

In the C++ version, the compiler creates a branch to check if the runtime stride is 1 or not. 

```assembly
cmp	r9, 1
jne	.L308
cmp	r8, 1
jne	.L308
```

We can see scalar instructions a bit down from the label we jump into:
```assembly
.L310:
	movss	xmm0, DWORD PTR [rcx]
	add	rcx, rdi
	mulss	xmm0, xmm1
	addss	xmm0, DWORD PTR [rax]
	movss	DWORD PTR [rax], xmm0
```

If there is no jump to the scalar path, we get to a block that uses SIMD instructions. 
```assembly
.L315:
	movups	xmm0, XMMWORD PTR [rcx+rax]
	movups	xmm2, XMMWORD PTR [rdx+rax]
	mulps	xmm0, xmm1
	addps	xmm0, xmm2
	movups	XMMWORD PTR [rdx+rax], xmm0
```

The Rust runtime implementation does not use packed SIMD instructions (such as mulps and addps) in the runtime version.

