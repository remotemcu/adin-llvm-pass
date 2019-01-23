; RUN: opt %s -load ../../build/AddressInterceptPass/libAddressInterceptPass.so \
; RUN: 	-adin -S | FileCheck %s
; ModuleID = 'global-load.c'
source_filename = "global-load.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@a = global i32 0, align 4

; Function Attrs: noinline nounwind optnone uwtable
define void @f() #0 {
  store i32 1, i32* @a, align 4
; CHECK-LABEL: @f(
; CHECK-NEXT: call void @__adin_store_(i8* bitcast (i32* @a to i8*), i32 1, i32 32, i32 4)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define i32 @v() #0 {
  %1 = alloca i32, align 4
  %2 = load i32, i32* @a, align 4
  store i32 %2, i32* %1, align 4
  %3 = load i32, i32* @a, align 4
; CHECK-LABEL: @v(
; CHECK-NEXT: %1 = alloca i32, align 4
; CHECK-NEXT: %2 = call i64 @__adin_load_(i8* bitcast (i32* @a to i8*), i32 32, i32 4)
; CHECK-NEXT:  %3 = trunc i64 %2 to i32
; CHECK-NEXT:  store i32 %3, i32* %1, align 4
; CHECK-NEXT:  %4 = call i64 @__adin_load_(i8* bitcast (i32* @a to i8*), i32 32, i32 4)
; CHECK-NEXT:  %5 = trunc i64 %4 to i32
  ret i32 %3
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
