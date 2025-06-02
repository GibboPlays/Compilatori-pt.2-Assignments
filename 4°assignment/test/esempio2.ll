define void @no_fusione(ptr %A, i32 %N) {
entry:
  br label %loop1

loop1:
  %i1 = phi i32 [ 0, %entry ], [ %i1.next, %loop1 ]
  %i1.next = add i32 %i1, 1
  store i32 %i1, ptr %A
  %cond1 = icmp slt i32 %i1, %N
  br i1 %cond1, label %loop1, label %loop2.preheader

loop2.preheader:
  br label %loop2

loop2:
  %i2 = phi i32 [ 0, %loop2.preheader ], [ %i2.next, %loop2 ]
  %val = load i32, ptr %A
  store i32 %val, ptr %A     ; NO--> dipendenza negativa
  %i2.next = add i32 %i2, 1
  %cond2 = icmp slt i32 %i2, %N
  br i1 %cond2, label %loop2, label %exit

exit:
  ret void
}
