; RUN: opt %s -load ../../build/AddressInterceptPass/libAddressInterceptPass.so \
; RUN: 	-adin -S | FileCheck %s
; ModuleID = 'pointer.c'
source_filename = "pointer.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@b = global i32 0, align 4

; Function Attrs: noinline nounwind optnone uwtable
define void @f() #0 {
  %1 = alloca i32, align 4
  store i32 1, i32* inttoptr (i64 256 to i32*), align 4
  %2 = load i32, i32* inttoptr (i64 256 to i32*), align 4
  store i32 %2, i32* %1, align 4
  %3 = load i32, i32* inttoptr (i64 256 to i32*), align 4
  store i32 %3, i32* @b, align 4
; CHECK-LABEL: @f(
; CHECK-NEXT: %1 = alloca i32, align 4
; CHECK-NEXT:  call void @__adin_store_(i8* inttoptr (i64 256 to i8*), i32 1, i32 32, i32 4)
; CHECK-NEXT:  %2 = call i32 @__adin_load_(i8* inttoptr (i64 256 to i8*), i32 32, i32 4)
; CHECK-NEXT:  store i32 %2, i32* %1, align 4
; CHECK-NEXT:  %3 = call i32 @__adin_load_(i8* inttoptr (i64 256 to i8*), i32 32, i32 4)
; CHECK-NEXT:  call void @__adin_store_(i8* bitcast (i32* @b to i8*), i32 %3, i32 32, i32 4)
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
