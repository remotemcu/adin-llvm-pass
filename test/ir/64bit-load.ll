; RUN: opt < %s -load ../../build/src/libAddressInterceptorPassModule.so \
; RUN:    -adin -S -adin-simple-global-skip=false | FileCheck %s
; ModuleID = '../src/64bit-load.c'
source_filename = "../src/64bit-load.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@a = global i64 0, align 8

; Function Attrs: noinline nounwind optnone uwtable
define void @f() #0 {
  store i64 1, i64* @a, align 8
  ret void
; CHECK-LABEL: @f(
; CHECK-NEXT:  call void @__adin_store_(i8* bitcast (i64* @a to i8*), i64 1, i32 64, i32 8)
; CHECK-NEXT:  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define i64 @v() #0 {
  %1 = alloca i64, align 8
  %2 = load i64, i64* @a, align 8
  store i64 %2, i64* %1, align 8
  %3 = load i64, i64* @a, align 8
  ret i64 %3
; CHECK-LABEL: @v(
; CHECK:  %load_i64_ = call i64 @__adin_load_(i8* bitcast (i64* @a to i8*), i32 64, i32 8)
; CHECK:  store i64 %load_i64_, i64* %1, align 8
; CHECK:  %load_i64_1 = call i64 @__adin_load_(i8* bitcast (i64* @a to i8*), i32 64, i32 8)
; CHECK-NEXT:  ret i64 %load_i64_1

}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
