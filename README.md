# ADIN LLVM Pass

![logo](img/logo.png)


## Introduction:
The **ADIN LLVM pass** is Transform LLVM pass for Runtime Hooking of Memory Operations is a crucial component within the [**ADIN LLVM fork**](https://github.com/remotemcu/adin-llvm). Designed to enhance the capabilities of the LLVM compiler infrastructure, this pass(plugin) facilitates the dynamic modification of memory operations, such as store and load operations, by replacing them with custom hook functions at runtime. By integrating this powerful plugin(pass) into your development workflow, you can gain fine-grained control over memory access and inject custom logic into your programs.


## How to Build
See [**ADIN LLVM fork doc**](https://github.com/remotemcu/adin-llvm)



## Usage
To utilize the memory operation hooking capabilities of the **ADIN LLVM plugin**, you can modify LLVM IR compiled code using the `opt` tool of the [**ADIN LLVM fork**](https://github.com/remotemcu/adin-llvm) with the `-adin` flag. Here's an example to help you understand the process:

Let's assume you have a simple C code file named `example.c`.
```c
int var = 0;

void f(){
	*(int*)0x100 = 1;
	var = *(int*)0x100;
}
```

 To compile it into LLVM IR code using Clang, execute the following command:

```shell
clang -S -emit-llvm example.c -o example.ll
```

This command will generate the LLVM IR code file `example.ll` based on your C code.
```llvm
; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @f() #0 {
  store i32 1, i32* inttoptr (i64 256 to i32*), align 4
  %1 = load i32, i32* inttoptr (i64 256 to i32*), align 4
  store i32 %1, i32* @b, align 4
  ret void
}

```

Now, you can use the **ADIN LLVM plugin** to modify the LLVM IR code and add memory operation hooks. Run the following command:

```shell
opt -adin -S example.ll-o adin_modified_example.ll
```

the `-adin` flag indicates that you want to perform memory operation hooking on the input LLVM IR code. The modified LLVM IR code will be written to the `modified.ll` file.

```llvm
define dso_local void @f() #0 {
  call void @__adin_store_(i8* inttoptr (i64 256 to i8*), i64 1, i32 32, i32 4)
  %load_i32_ = call i64 @__adin_load_(i8* inttoptr (i64 256 to i8*), i32 32, i32 4)
  %truncated_i32_ = trunc i64 %load_i32_ to i32
  store i32 %truncated_i32_, i32* @b, align 4
  ret void
}

```

In the modified LLVM IR code (`modified.ll`), the original store and load operations have been replaced with the `__adin_store_` and `__adin_load_` functions. These functions are the hook functions provided by the ADIN LLVM Fork, which allow you to intercept and modify the behavior of memory operations.

You can define and implement these hook functions in C/C++ code to perform any desired modifications or additional actions before or after the memory operations.

 * `__adin_store_` function will be called instead of a regular store operation,
 * `__adin_load_` function will be called instead of a regular load operation.

To implement the **__adin_store_** and **__adin_load_** hook functions in your C/C++ code for performing desired modifications or additional actions before memory operations, you can follow a similar approach to what is done in the [Address Interceptor Lib]. Here's an example:

```c
extern "C" void __adin_store_(llvm_pass_addr pointer, llvm_value_type value, llvm_pass_arg TypeSizeArg, llvm_pass_arg AlignmentArg)
{
   //...
}

extern "C" llvm_value_type __adin_load_(const llvm_pass_addr pointer, llvm_pass_arg TypeSizeArg, llvm_pass_arg AlignmentArg)
{
   //...
}
```

Finally, you can use the LLVM IR code to continue with the compilation process, linking, and generating the final executable or library as needed.

Yes, the `opt` utility provided by the [**ADIN LLVM fork**](https://github.com/remotemcu/adin-llvm) also allows you to hook `memmove`, `memcpy`, and `memset` operations in addition to store and load operations. You can enable the hooking of these memory operations using specific options provided by `opt`. Here are the options you can use:

```sh
$ opt --help | grep adin
  -adin-alloca-address-skip                         - Skip intercept address on alloca frame (Stack var)
  -adin-check-normal-address-aligment               - Checks normal alignment of address attempt
  -adin-mem-function-instructions                   - if equal true - intercept memmove/memcpy/memset function, else skip
  -adin-name-callback-load=<string>                 - Set name callback of load operation. Default __adin_load_
  -adin-name-callback-memcpy=<string>               - Set name callback of memcpy operation. Default __adin_memcpy_
  -adin-name-callback-memmove=<string>              - Set name callback of memmove operation. Default __adin_memmove_
  -adin-name-callback-memset=<string>               - Set name callback of memset operation. Default __adin_memset_
  -adin-name-callback-store=<string>                - Set name callback of store operation. Default __adin_store_
  -adin-simple-global-skip                          - Skip intercept address of SIMPLE global var
  -adin-skip-unsupported-instructions               - if equal true - skip this unsupported instruction, else throw error
  -adin-verbose-level=<int>                         - Set Level of verbose for AddressIntercept Pass

```
