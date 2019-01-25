; RUN: opt < %s -load ../../build/AddressInterceptPass/libAddressInterceptPass.so -adin -S \
; RUN:  | FileCheck %s
; ModuleID = 'src/load_pointer.c'
source_filename = "src/load_pointer.c"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.ST = type { i64, i64 }

@P = global %struct.ST* inttoptr (i64 256 to %struct.ST*), align 8
@g = global i32 0, align 4

; Function Attrs: noinline nounwind optnone uwtable
define void @f() #0 {
  %1 = load %struct.ST*, %struct.ST** @P, align 8
  %2 = getelementptr inbounds %struct.ST, %struct.ST* %1, i64 1
  %3 = getelementptr inbounds %struct.ST, %struct.ST* %2, i32 0, i32 0
  %4 = load i64, i64* %3, align 8
  %5 = load %struct.ST*, %struct.ST** @P, align 8
  %6 = getelementptr inbounds %struct.ST, %struct.ST* %5, i64 2
  %7 = getelementptr inbounds %struct.ST, %struct.ST* %6, i32 0, i32 1
  %8 = load i64, i64* %7, align 8
  %9 = add nsw i64 %4, %8
  %10 = load %struct.ST*, %struct.ST** @P, align 8
  %11 = getelementptr inbounds %struct.ST, %struct.ST* %10, i64 0
  %12 = getelementptr inbounds %struct.ST, %struct.ST* %11, i32 0, i32 0
  store i64 %9, i64* %12, align 8
; CHECK-LABEL: @f(
; CHECK-NEXT:  %load__struct.ST_p_ = call i64 @__adin_load_(i8* bitcast (%struct.ST** @P to i8*), i32 64, i32 8)
; CHECK-NEXT:  %converted_struct.ST_p__to_prt = inttoptr i64 %load__struct.ST_p_ to %struct.ST*
; CHECK-NEXT:  %1 = getelementptr inbounds %struct.ST, %struct.ST* %converted_struct.ST_p__to_prt, i64 1
; CHECK-NEXT:  %2 = getelementptr inbounds %struct.ST, %struct.ST* %1, i32 0, i32 0
; CHECK-NEXT:  %cast_Ptr_i64_p_to_i8p_ = bitcast i64* %2 to i8*
; CHECK-NEXT:  %load_i64_ = call i64 @__adin_load_(i8* %cast_Ptr_i64_p_to_i8p_, i32 64, i32 8)
; CHECK-NEXT:  %load__struct.ST_p_1 = call i64 @__adin_load_(i8* bitcast (%struct.ST** @P to i8*), i32 64, i32 8)
; CHECK-NEXT:  %converted_struct.ST_p__to_prt2 = inttoptr i64 %load__struct.ST_p_1 to %struct.ST*
; CHECK-NEXT:  %3 = getelementptr inbounds %struct.ST, %struct.ST* %converted_struct.ST_p__to_prt2, i64 2
; CHECK-NEXT:  %4 = getelementptr inbounds %struct.ST, %struct.ST* %3, i32 0, i32 1
; CHECK-NEXT:  %cast_Ptr_i64_p_to_i8p_3 = bitcast i64* %4 to i8*
; CHECK-NEXT:  %load_i64_4 = call i64 @__adin_load_(i8* %cast_Ptr_i64_p_to_i8p_3, i32 64, i32 8)
; CHECK-NEXT:  %5 = add nsw i64 %load_i64_, %load_i64_4
; CHECK-NEXT:  %load__struct.ST_p_5 = call i64 @__adin_load_(i8* bitcast (%struct.ST** @P to i8*), i32 64, i32 8)
; CHECK-NEXT:  %converted_struct.ST_p__to_prt6 = inttoptr i64 %load__struct.ST_p_5 to %struct.ST*
; CHECK-NEXT:  %6 = getelementptr inbounds %struct.ST, %struct.ST* %converted_struct.ST_p__to_prt6, i64 0
; CHECK-NEXT:  %7 = getelementptr inbounds %struct.ST, %struct.ST* %6, i32 0, i32 0
; CHECK-NEXT:  %cast_Ptr_i64_p_to_i8p_7 = bitcast i64* %7 to i8*
; CHECK-NEXT:  call void @__adin_store_(i8* %cast_Ptr_i64_p_to_i8p_7, i64 %5, i32 64, i32 8)
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define void @v(i64) #0 {
  %2 = alloca i64, align 8
  %3 = alloca %struct.ST*, align 8
  store i64 %0, i64* %2, align 8
  %4 = load i64, i64* %2, align 8
  %5 = inttoptr i64 %4 to %struct.ST*
  store %struct.ST* %5, %struct.ST** %3, align 8
  %6 = load %struct.ST*, %struct.ST** %3, align 8
  %7 = getelementptr inbounds %struct.ST, %struct.ST* %6, i64 0
  %8 = getelementptr inbounds %struct.ST, %struct.ST* %7, i32 0, i32 0
  store i64 0, i64* %8, align 8
; CHECK-LABEL: @v(
; CHECK:      %cast_Ptr_i64_p_to_i8p_ = bitcast i64* %8 to i8*
; CHECK-NEXT:  call void @__adin_store_(i8* %cast_Ptr_i64_p_to_i8p_, i64 0, i32 64, i32 8)

  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define void @w(%struct.ST*) #0 {
  %2 = alloca %struct.ST*, align 8
  store %struct.ST* %0, %struct.ST** %2, align 8
  %3 = load %struct.ST*, %struct.ST** %2, align 8
  %4 = ptrtoint %struct.ST* %3 to i32
  store i32 %4, i32* @g, align 4
  %5 = load i32, i32* @g, align 4
  %6 = add nsw i32 %5, 1
  store i32 %6, i32* @g, align 4
; CHECK-LABEL: @w(
; CHECK: %4 = ptrtoint %struct.ST* %3 to i32
; CHECK-NEXT: %cast_Val_i32__to_i64_ = zext i32 %4 to i64
; CHECK-NEXT:  call void @__adin_store_(i8* bitcast (i32* @g to i8*), i64 %cast_Val_i32__to_i64_, i32 32, i32 4)
; CHECK-NEXT:  %load_i32_ = call i64 @__adin_load_(i8* bitcast (i32* @g to i8*), i32 32, i32 4)
; CHECK-NEXT:  %truncated_i32_ = trunc i64 %load_i32_ to i32
; CHECK-NEXT:  %5 = add nsw i32 %truncated_i32_, 1
; CHECK-NEXT:  %cast_Val_i32__to_i64_1 = zext i32 %5 to i64
; CHECK-NEXT:  call void @__adin_store_(i8* bitcast (i32* @g to i8*), i64 %cast_Val_i32__to_i64_1, i32 32, i32 4)
  ret void
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 6.0.0-1ubuntu2~16.04.1 (tags/RELEASE_600/final)"}
