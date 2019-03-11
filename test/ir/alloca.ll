; RUN: opt < %s -load ../../build/src/libAddressInterceptorPassModule.so \
; RUN:    -adin -S | FileCheck %s

; ModuleID = 'alloca.c'
source_filename = "alloca.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define void @f() #0 {
  %1 = alloca i32, align 4
  store i32 0, i32* %1, align 4
  %2 = load i32, i32* %1, align 4
  %3 = or i32 %2, 1
  store i32 %3, i32* %1, align 4
  ret void
; CHECK-LABEL: @f(
; CHECK-NEXT: %1 = alloca i32, align 4
; CHECK-NEXT: store i32 0, i32* %1, align 4
; CHECK-NEXT: %2 = load i32, i32* %1, align 4
; CHECK-NEXT: %3 = or i32 %2, 1
; CHECK-NEXT: store i32 %3, i32* %1, align 4
}



; Function Attrs: noinline nounwind optnone uwtable
define i32 @v() #0 {
  %1 = alloca i32, align 4
  store i32 1, i32* %1, align 4
  %2 = load i32, i32* %1, align 4
  ret i32 %2
; CHECK-LABEL: @v(
; CHECK-NEXT: %1 = alloca i32, align 4
; CHECK-NEXT: store i32 1, i32* %1, align 4
; CHECK-NEXT: %2 = load i32, i32* %1, align 4
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
