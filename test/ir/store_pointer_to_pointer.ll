; RUN: opt < %s -load ../../build/src/libAddressInterceptorPassModule.so \
; RUN:    -adin -S | FileCheck %s

; ModuleID = '../src/store_pointer_to_pointer.c.c'
source_filename = "../src/pointer_to_pointer.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.test_struct = type { i8* }

@pointer_int = common global i32** null, align 8
@test_ex = internal global %struct.test_struct zeroinitializer, align 8

; Function Attrs: noinline nounwind optnone uwtable
define i32* @q(i32*) #0 {
  %2 = alloca i32*, align 8
  store i32* %0, i32** %2, align 8
  %3 = load i32*, i32** %2, align 8
  %4 = load i32**, i32*** @pointer_int, align 8
  %5 = getelementptr inbounds i32*, i32** %4, i64 0
  store i32* %3, i32** %5, align 8
  %6 = load i32*, i32** %2, align 8
  %7 = bitcast i32* %6 to i8*
  store i8* %7, i8** getelementptr inbounds (%struct.test_struct, %struct.test_struct* @test_ex, i32 0, i32 0), align 8
  %8 = load i8*, i8** getelementptr inbounds (%struct.test_struct, %struct.test_struct* @test_ex, i32 0, i32 0), align 8
  %9 = bitcast i8* %8 to i32*
; CHECK-LABEL: @q(
; CHECK: %"cast_Ptr_i32*_p_to_i8p_" = bitcast i32** %5 to i8*
; CHECK-NEXT: %cast_Val_i32_p__to_i64_ = ptrtoint i32* %3 to i64
; CHECK-NEXT: call void @__adin_store_(i8* %"cast_Ptr_i32*_p_to_i8p_", i64 %cast_Val_i32_p__to_i64_, i32 64, i32 8)

; CHECK: %cast_Val_i8_p__to_i64_ = ptrtoint i8* %7 to i64
; CHECK-NEXT: call void @__adin_store_(i8* bitcast (%struct.test_struct* @test_ex to i8*), i64 %cast_Val_i8_p__to_i64_, i32 64, i32 8)
; CHECK-NEXT: %load_i8_p_ = call i64 @__adin_load_(i8* bitcast (%struct.test_struct* @test_ex to i8*), i32 64, i32 8)
; CHECK-NEXT: %convertedi8_p__to_prt = inttoptr i64 %load_i8_p_ to i8*
  ret i32* %9
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
