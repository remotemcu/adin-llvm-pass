; RUN: opt < %s -load ../../build/src/libAddressInterceptorPassModule.so \
; RUN:    -adin -S | FileCheck %s
; ModuleID = '../src/skip_global_var.c'
source_filename = "../src/skip_global_var.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@load.b = internal global i32 0, align 4
@a = common global i32 0, align 4
@store.s = internal global i32 0, align 4

; Function Attrs: noinline nounwind optnone uwtable
define i32 @load() #0 {
  %1 = alloca i32, align 4
  %2 = load i32, i32* @a, align 4
  %3 = load i32, i32* @load.b, align 4
  %4 = add nsw i32 %2, %3
  store i32 %4, i32* %1, align 4
  %5 = load i32, i32* %1, align 4
; CHECK-LABEL: @load(
; CHECK: %2 = load i32, i32* @a, align 4
; CHECK: %3 = load i32, i32* @load.b, align 4
  ret i32 %5
}

; Function Attrs: noinline nounwind optnone uwtable
define void @store(i32) #0 {
  %2 = alloca i32, align 4
  store i32 %0, i32* %2, align 4
  %3 = load i32, i32* %2, align 4
  store i32 %3, i32* @store.s, align 4
  %4 = load i32, i32* %2, align 4
  store i32 %4, i32* @a, align 4
; CHECK-LABEL: @store(
; CHECK: store i32 %3, i32* @store.s, align 4
; CHECK: store i32 %4, i32* @a, align 4
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
